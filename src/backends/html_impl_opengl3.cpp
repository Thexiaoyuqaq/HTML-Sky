// ----------------------------------------------------------------------------
// OpenGL3 implementation of HT's Mod Loader.
// ----------------------------------------------------------------------------

#include <windows.h>
#include "MinHook.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl3.h"

#include "htinternal.h"
#include "includes/htconfig.h"

#ifdef USE_IMPL_OPENGL3

typedef WINBOOL (WINAPI *PFN_SwapBuffers)(
  HDC);

static PFN_SwapBuffers fn_SwapBuffers;
static bool gInit = false;

static WINBOOL WINAPI hook_SwapBuffers(
  HDC hDC
) {
  if (!gInit) {
    gInit = true;
    HTiInitGUI();
  }

  if (HTiBackendGLEnterCritical()) {
    ImGui_ImplOpenGL3_Init();
    // Set the gui inited event.
    HTiBackendGLInitComplete();
  }
  HTiBackendGLLeaveCritical();

  // Create new frame.
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  // Render ImGui.
  HTiUpdateGUI();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  return fn_SwapBuffers(hDC);
}

/**
 * Setup the vulkan layer injection.
 */
int HTi_ImplOpenGL3_Init() {
  int success = 0;
  MH_STATUS s;

  // We don't want to create a new thread and wait, so we load gdi32.dll
  // directly.
  LoadLibraryW(L"gdi32.dll");
  s = MH_CreateHookApi(
    L"gdi32.dll",
    "SwapBuffers",
    (void *)hook_SwapBuffers,
    (void **)&fn_SwapBuffers
  );
  success |= (s == MH_OK);

  return success;
}

#endif
