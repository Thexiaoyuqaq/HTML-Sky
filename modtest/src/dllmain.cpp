#include <windows.h>
#include "imgui.h"
#include "includes/htmod.h"

static HTHandle hKeyTest = nullptr;
static HMODULE hModuleDll = nullptr;
static char gVersionRtName[10] = {0};
static unsigned int gVersionRtNum = 0;
static HTKeyEventFlags gLastKeyEventFlags = 0;
static HTKeyEventPreventFlags gKeyEventPreventFlags = 0;

static void HTTestShowMainWindow(
  float timeElapsed,
  bool *open
) {
  if (!ImGui::Begin("Mod Test", open))
    // Early out if the window is collapsed, as an optimization.
    return ImGui::End();

  ImGui::Text("HT Mod Loader test mod.");
  ImGui::Text("FPS %.1f", 1.0 / timeElapsed);
  ImGui::Text("This mod has benn compiled with HTModLoader SDK v%s (%d).", HTML_VERSION_NAME, HTML_VERSION);
  ImGui::Text("This mod is running on HTModLoader v%s (%d).", gVersionRtName, gVersionRtNum);

  if (ImGui::CollapsingHeader("Help")) {
    ImGui::SeparatorText("ABOUT THIS DEMO:");
    ImGui::BulletText("This mod provides a template on how to write a mod on HTModLoader.");
    ImGui::BulletText("Sections below are demonstrating many aspects of the library.");

    ImGui::SeparatorText("PROGRAMMER GUIDE:");
    ImGui::BulletText("See the codes of this mod.");
    ImGui::BulletText("See comments in htmodloader.h.");
  }

  if (ImGui::CollapsingHeader("Inputs")) {
    ImGui::SeparatorText("Basic key bindings");
    ImGui::BulletText("Test key pressed: %d", HTHotkeyPressed(hKeyTest));
    ImGui::BulletText("Currently bound: %s", HTHotkeyGetName(HTHotkeyBindGet(hKeyTest)));
    ImGui::BulletText("Use \"Settings/Key Bindings\" in HTML Main Menu to change the key binding.");

    static bool preventGame = false;
    static bool preventNext = false;
    ImGui::SeparatorText("HTKeyEventPreventFlags");
    ImGui::Checkbox("HTKeyEventPreventFlags_Game", &preventGame);
    ImGui::Checkbox("HTKeyEventPreventFlags_Next", &preventNext);

    gKeyEventPreventFlags = 0;
    if (preventGame) gKeyEventPreventFlags |= HTKeyEventPreventFlags_Game;
    if (preventNext) gKeyEventPreventFlags |= HTKeyEventPreventFlags_Next;
  }

  if (ImGui::CollapsingHeader("Console")) {
    static char buf[128];

    ImGui::InputText("##ConsoleTest", buf, sizeof(buf));
    ImGui::SameLine();
    if (ImGui::Button("Print"))
      HTTellText("%s", buf);

    ImGui::Text("Use '§' (Alt+0167) to change the color of the text.");
    if (ImGui::Button("Colorful"))
      strcpy(buf, "§aC§bo§cl§do§er§1f§2u§3l");
  }

  ImGui::End();
}

__declspec(dllexport) void HTMLAPI HTModRenderGui(
  float timeElapsed,
  void *
) {
  HTTestShowMainWindow(timeElapsed, nullptr);
}

void HTTestKeyEventCallback(HTKeyEvent *event) {
  gLastKeyEventFlags = event->flags;
  event->preventFlags = gKeyEventPreventFlags;
}

__declspec(dllexport) HTStatus HTMLAPI HTModOnInit(
  void *
) {
  HTGetLoaderVersion(&gVersionRtNum);
  HTGetLoaderVersionName(
    gVersionRtName,
    sizeof(gVersionRtName));

  hKeyTest = HTHotkeyRegister(
    hModuleDll,
    "Test key",
    HTKey_M);
  HTHotkeyListen(hKeyTest, HTTestKeyEventCallback);

  return HT_SUCCESS;
}

__declspec(dllexport) HTStatus HTMLAPI HTModOnEnable(
  void *
) {
  HTTellText("HT Mod Test on enable.");
  return HT_SUCCESS;
}

BOOL APIENTRY DllMain(
  HMODULE hModule,
  DWORD dwReason,
  LPVOID lpReserved
) {
  if (dwReason == DLL_PROCESS_ATTACH)
    hModuleDll = hModule;
  return TRUE;
}
