//-----------------------------------------------------------------------------
// Copyright 2018 Jeff Hutchinson
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <GL/glew.h>
#include <open-simplex-noise.h>
#include "base/types.h"
#include "platform/window.h"
#include "platform/platform.h"
#include "platform/input.h"
#include "graphics/shader.h"
#include "game/camera.h"
#include "world/world.h"
#include "world/lighting.h"
#include "math/math.h"

extern S32 gVisibleChunks;
extern S32 gTotalChunks;
extern S32 gTotalVisibleChunks;

int main(int argc, char **argv) {
   if (!initPlatform())
      return -1;

#ifdef LIGHTQUEUE_TEST
   LIGHTQUEUE_TEST_FN();
#endif

   WindowCreationData createWindowData;
   memset(&createWindowData, 0, sizeof(WindowCreationData));
   createWindowData.api = OpenGL;
   createWindowData.versionMajor = 2;
   createWindowData.versionMinor = 1;
   createWindowData.fullscreen = false;

   WindowData window = createWindow("JeefCraft", 1440, 900, &createWindowData);

   if (glewInit() != GLEW_OK)
      return -2;
   initCamera();

   printf("OpenGL Driver Information:\n");
   printf("   Vendor:   %s\n", glGetString(GL_VENDOR));
   printf("   Renderer: %s\n", glGetString(GL_RENDERER));
   printf("   Version:  %s\n", glGetString(GL_VERSION));
   printf("   Shading:  %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

   F64 lastTime = getRealTime();
   F64 secondTime = lastTime;

   initWorld();

   S32 fpsCounter = 0;
#define FPS_BUFFER_SIZE 256
   char fpsBuffer[FPS_BUFFER_SIZE];

   // Set initial camera position
   Vec3 cameraPos;
   cameraPos.x = -5.0f;
   cameraPos.y = 80.0f;
   cameraPos.z = 0.0f;
   setCameraPosition(cameraPos);

   // Set projection matrix
   mat4 proj;
   glm_perspective(1.5708f, 1440.0f / 900.0f, 0.01f, getViewDistance(), proj);
   setCameraProjMatrix(proj);

   while (gRunning) {
      // Calculate mouse movement for frame.
      inputCacheMouseMovementForCurrentFrame();

      // Get delta time in milliseconds.
      F64 current = getRealTime();
      F32 delta = (F32)(current - lastTime) * 1000.0f;
      lastTime = current;

      if ((current - secondTime) >= 1.0) { // 1 second.
         Vec3 pos;
         getCameraPosition(&pos);

         memset(fpsBuffer, 0, FPS_BUFFER_SIZE);
         snprintf(fpsBuffer, FPS_BUFFER_SIZE, "JeefCraft - FPS: %d mspf: %f Camerapos: %f %f %f Chunks: %d visible | %d visible total | %d chunk total", fpsCounter, (1000.0f / (F32)fpsCounter), pos.x, pos.y, pos.z, gVisibleChunks, gTotalVisibleChunks, gTotalChunks);
         setWindowTitle(&window, fpsBuffer);

         // Reset
         fpsCounter = 0;
         secondTime = current;
      }

      // Calculate camera and frustum
      calculateFreecamViewMatrix(delta);

      // Perform rendering.
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      renderWorld(delta);

      swapBuffers(&window);
      pollEvents(&window);

      // We completed a frame!
      fpsCounter++;
   }
   
   freeWorld();
   freeWindow(&window);
   shutdownPlatform();

	return 0;
}
