// ----------------------------------------------------------------------------
// Backend dispatcher.
// You must put your backend initialize functions under the function below.
// ----------------------------------------------------------------------------
#include <mutex>

#include "htinternal.h"
#include "includes/htconfig.h"

std::mutex gGraphicInitMutex;

int HTiBackendExpectProcess() {
  int success = 0;

  // Setup all game backends.
#ifdef USE_IMPL_SKY
  // Expect Sky.exe
  extern int HTi_ImplSky_ExpectProcess();
  success |= HTi_ImplSky_ExpectProcess();
#endif
#ifdef USE_IMPL_MCBE
  // Expect Minecraft.Windows.exe
  extern void HTi_ImplSky_ExpectedProcess();
  success |= HTi_ImplSky_ExpectedProcess();
#endif

  return success;
}

int HTiBackendSetupAll() {
  int success = 0;

  // Setup all graphic backends.
#ifdef USE_IMPL_VKLAYER
  // Setup vulkan layer.
  extern int HTi_ImplVkLayer_Init();
  success |= HTi_ImplVkLayer_Init();
#endif
#ifdef USE_IMPL_OPENGL3
  // Setup OpenGL3.
  extern void HTi_ImplOpenGL3_Init();
  success |= HTi_ImplOpenGL3_Init();
#endif

  // Setup all game backends.
#ifdef USE_IMPL_SKY
  // Setup Sky:CotL.
  extern int HTi_ImplSky_Init();
  success |= HTi_ImplSky_Init();
#endif
#ifdef USE_IMPL_MCBE
  // Setup Minecraft:Bedrock.
  extern void HTi_ImplMCBE_Init();
  success |= HTi_ImplMCBE_Init();
#endif

  return success;
}