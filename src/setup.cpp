#include <windows.h>
#include <ntstatus.h>
#include <unordered_map>
#include <mutex>
#include <atomic>

#include "MinHook.h"
#include "utils/texts.h"
#include "htinternal.h"

typedef LONG (WINAPI *PFN_RegEnumValueA)(
  HKEY, DWORD, LPSTR, LPDWORD, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef NTSTATUS (WINAPI *PFN_NtQueryKey)(
  HANDLE, u64, PVOID, ULONG, PULONG);
typedef HWND (WINAPI *PFN_CreateWindowExA)(
  DWORD, LPCSTR, LPCSTR, DWORD, i32, i32, i32, i32, HWND, HMENU, HINSTANCE, LPVOID);
typedef HWND (WINAPI *PFN_CreateWindowExW)(
  DWORD, LPCWSTR, LPCWSTR, DWORD, i32, i32, i32, i32, HWND, HMENU, HINSTANCE, LPVOID);

static std::atomic<bool> gGameWindowCatched{false};
static std::unordered_map<HKEY, DWORD> gRegKeys;
static std::mutex gRegKeysMutex
  , gLocalGameStatusMutex;
static PFN_RegEnumValueA fn_RegEnumValueA;
static PFN_CreateWindowExA fn_CreateWindowExA;
static PFN_CreateWindowExW fn_CreateWindowExW;

/**
 * Check if the key name is a Vulkan implicit layer list.
 */
static i32 checkKeyName(HKEY key) {
  HMODULE ntdll = GetModuleHandleA("ntdll.dll");
  PFN_NtQueryKey fn_NtQueryKey;
  DWORD size = 0;
  NTSTATUS result = STATUS_SUCCESS;
  wchar_t *buffer;
  i32 r = 0;

  if (!key || !ntdll)
    return 0;

  fn_NtQueryKey = (PFN_NtQueryKey)GetProcAddress(ntdll, "NtQueryKey");
  if (!fn_NtQueryKey)
    return 0;

  result = fn_NtQueryKey(key, 3, 0, 0, &size);
  if (result == STATUS_BUFFER_TOO_SMALL) {
    buffer = (wchar_t *)malloc(size + 2);
    if (!buffer)
      return 0;

    result = fn_NtQueryKey(key, 3, buffer, size, &size);
    if (result == STATUS_SUCCESS)
      buffer[size / sizeof(wchar_t)] = 0;
    
    r = !wcscmp(buffer + 2, HTTexts_VulkanLayer);
    free(buffer);
  }

  return r;
}

/**
 * Inject HTML layer on index 0.
 */
static LONG WINAPI hook_RegEnumValueA(
  HKEY hKey,
  DWORD dwIndex,
  LPSTR lpValueName,
  LPDWORD lpcchValueName,
  LPDWORD lpReserved,
  LPDWORD lpType,
  LPBYTE lpData,
  LPDWORD lpcbData
) {
  std::lock_guard<std::mutex> lock(gRegKeysMutex);

  LONG result;
  auto it = gRegKeys.find(hKey);
  bool notSaved = it == gRegKeys.end();

  if (notSaved && !dwIndex) {
    // The handle isn't recorded and it's the first call on this key.
    if (checkKeyName(hKey)) {
      // Set the current registry handle as access for Vulkan layer loader.
      gRegKeys[hKey] = 1;

      // Inject the layer.
      if (lpValueName)
        strcpy(lpValueName, gPathLayerConfig);
      if (lpcchValueName)
        *lpcchValueName = strlen(gPathLayerConfig) + 1;
      if (lpType)
        *lpType = REG_DWORD;
      if (lpData)
        *((i32 *)lpData) = 0;
      if (lpcbData)
        *lpcbData = 4;

      return ERROR_SUCCESS;
    } else
      // Set the current registry handle as regular access.
      gRegKeys[hKey] = 2;
  }

  // Return the enumerate result.
  result = fn_RegEnumValueA(
    hKey,
    (!notSaved && gRegKeys[hKey] == 1) ? dwIndex - 1 : dwIndex,
    lpValueName,
    lpcchValueName,
    lpReserved,
    lpType,
    lpData,
    lpcbData);
  if (result == ERROR_NO_MORE_ITEMS)
    // Enumeration ended.
    gRegKeys.erase(hKey);

  return result;
}

static i32 checkWindowAndSetupAW(HWND hWnd) {
  wchar_t buffer[32];
  HTGameEdition edition = HT_EDITION_UNKNOWN;

  //if (gGameWindowCatched.load(std::memory_order_acquire))
    //return 0;
  if (gGameStatus.window)
    return 0;

  // Get the game edition from window name.
  GetWindowTextW(hWnd, buffer, 32);
  buffer[31] = 0;
  if (!wcscmp(buffer, HTTexts_WndNameChW))
    edition = HT_EDITION_CHINESE;
  else if (!wcscmp(buffer, HTTexts_WndNameInW))
    edition = HT_EDITION_INTERNATIONAL;
  else
    return 0;

  // Check the window's class name.
  GetClassNameW(hWnd, buffer, 32);
  buffer[31] = 0;
  if (wcscmp(buffer, HTTexts_WndClassW))
    return 0;

  //bool expected = false;
  //if (!gGameWindowCatched.compare_exchange_strong(
    //expected,
    //true,
    //std::memory_order_release
  //))
    //return 0;

  // Set game edition and hWnd.
  gGameStatus.edition = edition;
  gGameStatus.window = hWnd;

  HTiSetupAll();

  return 1;
}

static HWND WINAPI hook_CreateWindowExA(
  DWORD dwExStyle,
  LPCSTR lpClassName,
  LPCSTR lpWindowName,
  DWORD dwStyle,
  int X,
  int Y,
  int nWidth,
  int nHeight,
  HWND hWndParent,
  HMENU hMenu,
  HINSTANCE hInstance,
  LPVOID lpParam
) {
  HWND result = fn_CreateWindowExA(
    dwExStyle,
    lpClassName,
    lpWindowName,
    dwStyle,
    X,
    Y,
    nWidth,
    nHeight,
    hWndParent,
    hMenu,
    hInstance,
    lpParam);
  DWORD lastError = GetLastError();

  if (!result)
    return result;

  checkWindowAndSetupAW(result);

  SetLastError(lastError);
  return result;
}

static HWND WINAPI hook_CreateWindowExW(
  DWORD dwExStyle,
  LPCWSTR lpClassName,
  LPCWSTR lpWindowName,
  DWORD dwStyle,
  int X,
  int Y,
  int nWidth,
  int nHeight,
  HWND hWndParent,
  HMENU hMenu,
  HINSTANCE hInstance,
  LPVOID lpParam
) {
  HWND result = fn_CreateWindowExW(
    dwExStyle,
    lpClassName,
    lpWindowName,
    dwStyle,
    X,
    Y,
    nWidth,
    nHeight,
    hWndParent,
    hMenu,
    hInstance,
    lpParam);
  DWORD lastError = GetLastError();

  if (!result)
    return result;

  checkWindowAndSetupAW(result);

  SetLastError(lastError);
  return result;
}

void HTiSetupWinHooks() {
  MH_CreateHookApi(
    L"advapi32.dll",
    "RegEnumValueA",
    (void *)hook_RegEnumValueA,
    (void **)&fn_RegEnumValueA
  );
  MH_CreateHookApi(
    L"user32.dll",
    "CreateWindowExA",
    (void *)hook_CreateWindowExA,
    (void **)&fn_CreateWindowExA
  );
  MH_CreateHookApi(
    L"user32.dll",
    "CreateWindowExW",
    (void *)hook_CreateWindowExW,
    (void **)&fn_CreateWindowExW
  );

  MH_EnableHook(MH_ALL_HOOKS);
}

void HTiSetupAll() {
  std::wstring optionsPath(gPathDataWide);
  optionsPath += L"\\options.json";
  HTiOptionsLoadFromFile(optionsPath.c_str());

  HTiLoadMods();
}
