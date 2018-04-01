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

#include <open-simplex-noise.h>
#include "world/material.h"
#include "world/terrainGen.h"
#include "world/util.h"
#include "world/worldMap.h"

static struct osn_context *osn;

// Our map world
extern ChunkTable gChunkTable;

void initTerrainGen() {
   open_simplex_noise((U64)0xDEADBEEF, &osn);
}

void freeTerrainGen() {
   open_simplex_noise_free(osn);
}

// Worldspace
static bool shouldCave(S32 x, S32 y, S32 z) {
   F64 cave_stretch = 24.0;

   F64 noise = 0.0;
   for (S32 i = 0; i < 6; ++i) {
      F64 factor = cave_stretch * ((F64)((1 << i) / 3) + 1.0);

      noise += (open_simplex_noise3(
         osn,
         (F64)(x) / factor * (F64)(1 << i),
         (F64)y / factor * (F64)(1 << (i + 1)),
         (F64)(z) / factor * (F64)(1 << i)
      ) + 1.0) / (F64)(1 << (i + 1));
   }

   return noise >= 1.33;
}

static S32 solidCubesAroundCubeAt(S32 x, S32 y, S32 z, S32 worldX, S32 worldZ) {
   S32 solidCount = 0;
   solidCount += !isTransparentAtCube(getGlobalCubeAtWorldSpacePosition(x + worldX - 1, y, z + worldZ)) && !shouldCave(x + worldX - 1, y, z + worldZ);
   solidCount += !isTransparentAtCube(getGlobalCubeAtWorldSpacePosition(x + worldX + 1, y, z + worldZ)) && !shouldCave(x + worldX + 1, y, z + worldZ);
   solidCount += !isTransparentAtCube(getGlobalCubeAtWorldSpacePosition(x + worldX, y - 1, z + worldZ)) && !shouldCave(x + worldX, y - 1, z + worldZ);
   solidCount += !isTransparentAtCube(getGlobalCubeAtWorldSpacePosition(x + worldX, y + 1, z + worldZ)) && !shouldCave(x + worldX, y + 1, z + worldZ);
   solidCount += !isTransparentAtCube(getGlobalCubeAtWorldSpacePosition(x + worldX, y, z - 1 + worldZ)) && !shouldCave(x + worldX, y, z - 1 + worldZ);
   solidCount += !isTransparentAtCube(getGlobalCubeAtWorldSpacePosition(x + worldX, y, z + 1 + worldZ)) && !shouldCave(x + worldX, y, z + 1 + worldZ);
   return solidCount;
}

void generateWorld(S32 worldX, S32 worldZ) {
   // Get chunk at this position
   Chunk *chunk = chunktable_getAt(&gChunkTable, worldX, worldZ);
   Cube *cubeData = chunk->cubeData;

   F64 stretchFactor = 20.0;

   for (S32 x = 0; x < CHUNK_WIDTH; ++x) {
      for (S32 z = 0; z < CHUNK_WIDTH; ++z) {
         // calculate height for each cube.
         // Taking absolute value will allow for only 0-1 scaling.
         // also make sure to use the world coordinates

         // Smoothen the noise based on 5 blocks surrounding it.
         F64 noise = (open_simplex_noise2(osn, (F64)(x + worldX) / stretchFactor, (F64)(z + worldZ) / stretchFactor)) * 10.0;
         for (S32 i = -5; i < 5; ++i) {
            for (S32 j = -5; j < 5; ++j) {
               noise += (open_simplex_noise2(osn, (F64)(x + i + worldX) / (stretchFactor + i), (F64)(z + j + worldZ) / (stretchFactor + j)) * (10.0 + j)) / 2.0f;
            }
            noise /= 10.f;
         }
         //F64 noise = fabs(open_simplex_noise2(osn, (F64)(x + worldX) / stretchFactor, (F64)(z + worldZ) / stretchFactor) * 10.0);
         S32 height = (S32)(noise)+70.0f; // 70 as base height.

                                          // Make block at height level grass.
         getCubeAt(cubeData, x, height, z)->material = Material_Grass;

         // Need to make air for anything above height.
         // Anything below height is just block the whole way till last couple rows which
         // are bedrock.
         for (S32 y = height + 1; y < MAX_CHUNK_HEIGHT; ++y) {
            // All air
            getCubeAt(cubeData, x, y, z)->material = Material_Air;
         }
         for (S32 y = 4; y < height; ++y) {
            // All dirt.
            // todo: this is where we do stuff like caves!
            // but of course in a second pass, or third pass...!
            getCubeAt(cubeData, x, y, z)->material = Material_Dirt;
         }
         for (S32 y = 0; y < 4; ++y) {
            // All bedrock
            getCubeAt(cubeData, x, y, z)->material = Material_Bedrock;
         }
      }
   }
}

void generateCavesAndStructures(S32 worldX, S32 worldZ) {
   // Get chunk at this position
   Chunk *chunk = chunktable_getAt(&gChunkTable, worldX, worldZ);
   Cube *cubeData = chunk->cubeData;

   // Generate caves.
   for (S32 x = 0; x < CHUNK_WIDTH; ++x) {
      for (S32 z = 0; z < CHUNK_WIDTH; ++z) {
         for (S32 y = 0; y < MAX_CHUNK_HEIGHT; ++y) {
            Cube *c = getCubeAt(cubeData, x, y, z);
            if (c->material == Material_Bedrock)
               continue;
            if (c->material == Material_Air)
               break;

            if (shouldCave(x + worldX, y, z + worldZ)) {
               // Perform smothing.
               S32 solidCount = solidCubesAroundCubeAt(x, y, z, worldX, worldZ);
               if (solidCount < 4) {
                  // It's a cave, carve out air.
                  c->material = Material_Air;
               }
            }
         }
      }
   }

   // Generate Trees
   for (S32 x = 0; x < CHUNK_WIDTH; ++x) {
      for (S32 z = 0; z < CHUNK_WIDTH; ++z) {
         // Find the height. Skip over anything that isn't the height.
         // and we only care about grass.
         S32 height = MAX_CHUNK_HEIGHT - 1;
         for (; height >= 0; --height) {
            if (getCubeAt(cubeData, x, height, z)->material != Material_Air) {
               break;
            }
         }

         if (getCubeAt(cubeData, x, height, z)->material == Material_Grass) {
            // Lets generate some trees.
            //
            // Also, a tree only has a 1/10 chance of spawning on this block.
            S32 posX = x;
            S32 posZ = z;
            if (open_simplex_noise2(osn, (F64)posX + worldX, (F64)posZ + worldZ) >= 0.8) {
               getCubeAt(cubeData, x, height + 1, z)->material = Material_Wood_Trunk;
               getCubeAt(cubeData, x, height + 2, z)->material = Material_Wood_Trunk;
               getCubeAt(cubeData, x, height + 3, z)->material = Material_Wood_Trunk;
               for (S32 xxx = x - 3; xxx < x + 3; ++xxx) {
                  if (xxx >= CHUNK_WIDTH)
                     break;
                  else if (xxx < 0)
                     continue;
                  for (S32 zzz = z - 3; zzz < z + 3; ++zzz) {
                     if (zzz >= CHUNK_WIDTH)
                        break;
                     else if (zzz < 0)
                        continue;
                     getCubeAt(cubeData, xxx, height + 4, zzz)->material = Material_Leaves;
                  }
               }
            }
         }
      }
   }
}
