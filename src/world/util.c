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

#include "world/util.h"
#include "world/material.h"
#include "world/worldMap.h"

extern S32 worldSize;

// Our map world
extern ChunkTable gChunkTable;

Cube* getCubeAt(Cube *cubeData, S32 x, S32 y, S32 z) {
   return &cubeData[flattenWorldArrayIndex(x, y, z)];
}

void worldCordsToChunkCoords(S32 x, S32 z, S32 *chunkX, S32 *chunkZ) {
   /*S32 roundedX = (S32)((F32)x / CHUNK_WIDTH);
   S32 roundedZ = (S32)((F32)z / CHUNK_WIDTH);

   if (x < 0) roundedX--;
   if (z < 0) roundedZ--;

   *chunkX = roundedX * CHUNK_WIDTH;
   *chunkZ = roundedZ * CHUNK_WIDTH; */

   S32 diffX = x % CHUNK_WIDTH;
   S32 diffZ = z % CHUNK_WIDTH;
   *chunkX = x < 0 ? x + diffX : x - diffX;
   *chunkZ = z < 0 ? z + diffZ : z - diffZ;
}

bool isTransparent(Cube *cubeData, S32 x, S32 y, S32 z) {
   assert(cubeData);
   assert(getCubeAt(cubeData, x, y, z));
   return getCubeAt(cubeData, x, y, z)->material == Material_Air;
}

bool isTransparentAtCube(Cube *c) {
   if (c == NULL)
      return false;
   return c->material == Material_Air;
}

Chunk* getChunkAtWorldSpacePosition(S32 x, S32 y, S32 z) {
   S32 chunkX;
   S32 chunkZ;
   worldCordsToChunkCoords(x, z, &chunkX, &chunkZ);
   Chunk *chunk = chunktable_getAt(&gChunkTable, chunkX, chunkZ);
   return chunk;
}

RenderChunk* getRenderChunkAtWorldSpacePosition(S32 x, S32 y, S32 z, S32 *renderChunkIndex) {
   Chunk *chunk = getChunkAtWorldSpacePosition(x, y, z);
   if (chunk == NULL)
      return NULL;
   assert(y >= 0); // Ensure y is >= 0
   assert(y < MAX_CHUNK_HEIGHT); // Ensure chunk is < MAX_CHUNK_HEIGHT

   *renderChunkIndex = y / RENDER_CHUNK_HEIGHT;
   return &chunk->renderChunks[*renderChunkIndex];
}

void globalPosToLocalPos(S32 x, S32 y, S32 z, S32 *localX, S32 *localY, S32 *localZ) {
   S32 chunkY = y / RENDER_CHUNK_HEIGHT;

   if (x >= 0 || x % CHUNK_WIDTH == 0)
      *localX = x % CHUNK_WIDTH;
   else
      *localX = x % CHUNK_WIDTH + CHUNK_WIDTH;
   if (z >= 0 || z % CHUNK_WIDTH == 0)
      *localZ = z % CHUNK_WIDTH;
   else
      *localZ = z % CHUNK_WIDTH + CHUNK_WIDTH;
   *localY = y - (chunkY * RENDER_CHUNK_HEIGHT);

   assert(*localX >= 0);
   assert(*localZ >= 0);
   assert(*localY >= 0);
   assert(*localX < CHUNK_WIDTH);
   assert(*localZ < CHUNK_WIDTH);
   assert(*localY < RENDER_CHUNK_HEIGHT);
}

Cube* getGlobalCubeAtWorldSpacePosition(S32 x, S32 y, S32 z) {
   // first calculate chunk based upon position.
   S32 chunkX;
   S32 chunkZ;
   worldCordsToChunkCoords(x, z, &chunkX, &chunkZ);

   Chunk *chunk = chunktable_getAt(&gChunkTable, chunkX, chunkZ);
   if (chunk == NULL)
      return NULL;

   // LocalY isn't used because we are in full chunk space, not render chunk space.
   S32 localX, localY, localZ;
   globalPosToLocalPos(x, y, z, &localX, &localY, &localZ);
   assert(getCubeAt(chunk->cubeData, localX, y, localZ));
   return getCubeAt(chunk->cubeData, localX, y, localZ);
}