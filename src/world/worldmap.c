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

#include "world/worldMap.h"
#include "world/lighting.h"

void chunktable_create(S32 tblSize, ChunkTable *outTable) {
   assert(tblSize > 0);

   outTable->tableSize = tblSize;
   outTable->count = 0;
   outTable->chunkTable = (Chunk*)malloc(sizeof(Chunk) * tblSize);
   //table->freeList = NULL;
}

void chunktable_free(ChunkTable *table) {
   // Free all cubedata and lightmap data on chunk
   for (S32 i = 0; i < table->count; ++i) {
      free(table->chunkTable[i].cubeData);
      chunk_freeLightmap(&table->chunkTable[i]);
   }
   free(table->chunkTable);

   // TODO: free "free list"

   memset(table, 0, sizeof(ChunkTable));
}

void chunktable_insertAt(ChunkTable *table, S32 x, S32 z) {
   if (table->count == table->tableSize) {
      // Need more space within our flat array, increase it by 1.5X tableSize
      S32 sz = (S32)((F32)table->tableSize * 1.5f);
      table->chunkTable = (Chunk*)realloc(table->chunkTable, sizeof(Chunk) * sz);
      table->tableSize = sz;
   }
   S32 index = table->count;
   ++table->count;

   // Make new chunk.
   // TODO: free list
   Chunk chunk;
   memset(&chunk, 0, sizeof(Chunk));
   chunk.startX = x;
   chunk.startZ = z;
   chunk.cubeData = (Cube*)calloc(CHUNK_SIZE, sizeof(Cube));
   chunk_initLightmap(&chunk);

   // Append it to the table.
   memcpy(&table->chunkTable[index], &chunk, sizeof(Chunk));
}

void chunktable_removeAt(ChunkTable *table, S32 x, S32 z) {
   for (S32 i = 0; i < table->count; ++i) {
      Chunk *c = &table->chunkTable[i];
      if (c->startX == x && c->startZ == z) {
         // TODO: free list

         free(c->cubeData);
         chunk_freeLightmap(c);

         // shift elements down by one.
         memmove(
            &table->chunkTable[i], 
            &table->chunkTable[i + 1], 
            sizeof(Chunk) * (table->count - (i + 1))
         );
         table->count--;

         return;
      }
   }

   // This should never happen. Code bug if we hit here.
   assert(false);
}

Chunk* chunktable_getAt(ChunkTable *table, S32 x, S32 z) {
   // Get the chunk [O(n) lookup]
   for (S32 i = 0; i < table->count; ++i) {
      Chunk *c = &table->chunkTable[i];
      if (c->startX == x && c->startZ == z) {
         return c;
      }
   }
   return NULL;
}

void chunktable_foreach(ChunkTable *table, ChunkTableForeach fn) {
   for (S32 i = 0; i < table->count; ++i)
      fn(&table->chunkTable[i]);
}
