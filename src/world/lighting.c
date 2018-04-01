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
//
// Note: Lighting implementation comes from and builds off of the technical
// blogposts from Seed Of Andromeda.
// https://www.seedofandromeda.com/blogs/
//----------------------------------------------------------------------------

#include <string.h>
#include <stretchy_buffer.h>
#include "world/lighting.h"
#include "world/util.h"

typedef struct Node {
   S32 x;
   S32 y;
   S32 z;
   Chunk *c;
   S32 renderChunkId;

   struct Node *next;
   struct Node *prev;
} Node;

#define LIGHTQUEUE_INITIAL_LENGTH 16

// LightQueue is an internal circular stretchy buffer.
// It will grow as it needs more space, but before it grows it will use
// all slots available.
typedef struct LightQueue {
   //S32 front;
   //S32 nextFree;
   S32 count;
   //S32 length;
   //Node *nodes;
   Node *front;
   Node *back;
} LightQueue;

void lightqueue_init(LightQueue *queue) {
   queue->count = 0;
   queue->front = NULL;
   queue->back = NULL;
}

void lightqueue_pop(LightQueue *queue) {
   assert(queue->count);

   Node *front = queue->front;
   queue->front = front->next;
   free(front);
   queue->count--;
}

void lightqueue_cleanup(LightQueue *queue) {
   while (queue->count)
      lightqueue_pop(queue);
   memset(queue, 0, sizeof(LightQueue));
}

void lightqueue_push(LightQueue *queue, S32 x, S32 y, S32 z, Chunk *c, S32 renderChunkId) {
   Node *n = (Node*)calloc(1, sizeof(Node));
   n->x = x;
   n->y = y;
   n->z = z;
   n->c = c;
   n->renderChunkId = renderChunkId;

   queue->count++;

   if (queue->back)
      queue->back->next = n;
   n->prev = queue->front;
   n->next = NULL;
   queue->back = n;
   if (!queue->front)
      queue->front = n;
}

Node lightqueue_front(LightQueue *queue) {
   assert(queue->count);
   return *queue->front;
}

/*
void lightqueue_init(LightQueue *queue) {
   queue->front = 0;
   queue->nextFree = 0;
   queue->count = 0;
   queue->length = LIGHTQUEUE_INITIAL_LENGTH;
   queue->nodes = (Node*)calloc(LIGHTQUEUE_INITIAL_LENGTH, sizeof(Node));
}

void lightqueue_cleanup(LightQueue *queue) {
   free(queue->nodes);
   memset(queue, 0, sizeof(LightQueue));
}

void lightqueue_push(LightQueue *queue, S32 x, S32 y, S32 z, Chunk *c) {
   if (queue->count == queue->length) {
      // We need to realloc.
      queue->nodes = (Node*)realloc(queue->nodes, queue->length * 2);
      queue->length = queue->length * 2;
   }

   // Wrap next free
   if (queue->nextFree == queue->length) {
      queue->nextFree = 0;
   }

   Node *n = &queue->nodes[queue->nextFree];
   queue->nextFree++;
   queue->count++;
   n->x = x;
   n->y = y;
   n->z = z;
   n->c = c;
}

Node lightqueue_front(LightQueue *queue) {
   assert(queue->count);
   return queue->nodes[queue->front];
}

void lightqueue_pop(LightQueue *queue) {
   assert(queue->count);

   queue->front++;
   queue->count--;

   // wrap
   if (queue->front == queue->length) {
      queue->front = 0;
   }
}
*/

#ifdef LIGHTQUEUE_TEST
void LIGHTQUEUE_TEST_FN() {
   LightQueue q;
   lightqueue_init(&q);

   for (S32 i = 0; i < LIGHTQUEUE_INITIAL_LENGTH; ++i)
      lightqueue_push(&q, i, i, i, (Chunk*)i, 9);

   // Get front of queue
   Node n = lightqueue_front(&q);
   assert(n.x == 0);

   // Pop it
   lightqueue_pop(&q);

   // Now insert another item
   lightqueue_push(&q, 69, 69, 69, (Chunk*)69, 69);
   Node next = lightqueue_front(&q);
   assert(next.x == 1);

   // Now remove all the other 'original' ones and check the 69 one.
   for (S32 i = 1; i < LIGHTQUEUE_INITIAL_LENGTH; ++i)
      lightqueue_pop(&q);
   Node another = lightqueue_front(&q);
   assert(another.x == 69);

   printf("All LightQueue tests passed.\n");
   lightqueue_cleanup(&q);
}
#endif

//-----------------------------------------------------------------------------
// Light Map Implementation
//-----------------------------------------------------------------------------

static inline void lightmap_init(LightMap *lightMap) {
   memset(lightMap, (U8)0, sizeof(LightMap));
}

static inline S32 lightmap_getGlobalLight(LightMap *lightMap, S32 x, S32 y, S32 z) {
   return (S32)lightMap->lights[flattenRenderChunkArrayIndex(x, y, z)].global;
}

static inline void lightmap_setGlobalLight(LightMap *lightMap, S32 x, S32 y, S32 z, S32 value) {
   assert(value <= MAX_LIGHT_LEVEL);

   S32 index = flattenRenderChunkArrayIndex(x, y, z);
   lightMap->lights[index].global = (U8)value;
}

static inline S32 lightmap_getBlockLight(LightMap *lightMap, S32 x, S32 y, S32 z) {
   return (S32)lightMap->lights[flattenRenderChunkArrayIndex(x, y, z)].block;
}

static inline void lightmap_setBlockLight(LightMap *lightMap, S32 x, S32 y, S32 z, S32 value) {
   assert(value <= MAX_LIGHT_LEVEL);

   S32 index = flattenRenderChunkArrayIndex(x, y, z);
   lightMap->lights[index].block = (U8)value;
   assert(lightMap->lights[index].block <= MAX_LIGHT_LEVEL);
}

//-----------------------------------------------------------------------------
// Chunk Lighting Implementation
//-----------------------------------------------------------------------------

void chunk_initLightmap(Chunk *chunk) {
   for (S32 i = 0; i < CHUNK_SPLITS; ++i) {
      chunk->lightMap[i] = (LightMap*)malloc(sizeof(LightMap));
      lightmap_init(chunk->lightMap[i]);
   }
}

void chunk_freeLightmap(Chunk *chunk) {
   for (S32 i = 0; i < CHUNK_SPLITS; ++i) {
      free(chunk->lightMap[i]);
   }
   //free(chunk->lightMap);
   //chunk->lightMap = NULL;
}

S32 chunk_getBlockLight(Chunk *chunk, S32 x, S32 y, S32 z) {
   S32 index = y / RENDER_CHUNK_HEIGHT;
   return lightmap_getBlockLight(chunk->lightMap[index], x, y % RENDER_CHUNK_HEIGHT, z);
}

static void updateSurroundingBlock(LightQueue *q, Chunk *chunk, S32 x, S32 y, S32 z, S32 lightLevel) {
   S32 renderChunkId = y / RENDER_CHUNK_HEIGHT;

   if (lightLevel > MIN_LIGHT_LEVEL) {
      if (!isTransparent(chunk->cubeData, x, y, z) && lightmap_getBlockLight(chunk->lightMap[renderChunkId], x, y % RENDER_CHUNK_HEIGHT, z) + MIN_LIGHT_LEVEL <= lightLevel) {
         // lightmap_setBlockLight requires a 'local y'
         lightmap_setBlockLight(chunk->lightMap[renderChunkId], x, y % RENDER_CHUNK_HEIGHT, z, lightLevel - 1);
         lightqueue_push(q, x, y, z, chunk, renderChunkId);
      }
   }
}

typedef struct UpdateLightChunkInfo {
   Chunk *c;
   S32 renderChunkIndex;
} UpdateLightChunkInfo;

void chunk_setBlockLight(Chunk *chunk, S32 x, S32 y, S32 z, S32 value) {
   LightQueue q;
   lightqueue_init(&q);

   lightmap_setBlockLight(chunk->lightMap[y / RENDER_CHUNK_HEIGHT], x, y % RENDER_CHUNK_HEIGHT, z, value);
   lightqueue_push(&q, x, y, z, chunk, y / RENDER_CHUNK_HEIGHT);

   UpdateLightChunkInfo *chunkList = NULL;

   while (q.count) {
      Node n = lightqueue_front(&q);
      lightqueue_pop(&q);

      // Insert chunk into list if it doesn't exist.
      bool found = false;
      for (S32 i = 0; i < sb_count(chunkList); ++i) {
         if (chunkList[i].c == n.c && chunkList[i].renderChunkIndex == n.renderChunkId) {
            found = true;
            break;
         }
      }
      if (!found) {
         UpdateLightChunkInfo ch;
         ch.c = n.c;
         ch.renderChunkIndex = n.renderChunkId;
         sb_push(chunkList, ch);
      }

      // Get block light level for our current node
      S32 lightLevel = lightmap_getBlockLight(n.c->lightMap[n.renderChunkId], n.x, n.y, n.z);

      // Go around each block and for each block that isn't opaque, subtract a light level - 1
      // Boudsn checking is needed on x and z axes.
      // TODO: We need relight as chunks come into view / loaded / generated.
      // This might be out of scope of this method, but we need to still do it anyways.
      if (n.x == 0) {
         Chunk *nextChunk = getChunkAtWorldSpacePosition(n.c->startX - CHUNK_WIDTH, n.y, n.c->startZ);
         if (nextChunk) {
            updateSurroundingBlock(&q, nextChunk, CHUNK_WIDTH - 1, n.y, n.z, lightLevel);
         }
      } else {
         updateSurroundingBlock(&q, n.c, n.x - 1, n.y, n.z, lightLevel);
      }
      if (n.x == CHUNK_WIDTH - 1) {
         Chunk *nextChunk = getChunkAtWorldSpacePosition(n.c->startX + CHUNK_WIDTH, n.y, n.c->startZ);
         if (nextChunk) {
            updateSurroundingBlock(&q, nextChunk, 0, n.y, n.z, lightLevel);
         }
      } else {
         updateSurroundingBlock(&q, n.c, n.x + 1, n.y, n.z, lightLevel);
      }
      if (n.z == 0) {
         Chunk *nextChunk = getChunkAtWorldSpacePosition(n.c->startX, n.y, n.c->startZ - CHUNK_WIDTH);
         if (nextChunk) {
            updateSurroundingBlock(&q, nextChunk, n.x, n.y, CHUNK_WIDTH - 1, lightLevel);
         }
      } else {
         updateSurroundingBlock(&q, n.c, n.x, n.y, n.z - 1, lightLevel);
      }
      if (n.z == CHUNK_WIDTH - 1) {
         Chunk *nextChunk = getChunkAtWorldSpacePosition(n.c->startX, n.y, n.c->startZ + CHUNK_WIDTH);
         if (nextChunk) {
            updateSurroundingBlock(&q, nextChunk, n.x, n.y, 0, lightLevel);
         }
      } else {
         updateSurroundingBlock(&q, n.c, n.x, n.y, n.z + 1, lightLevel);
      }

      if (n.y > 0)
         updateSurroundingBlock(&q, n.c, n.x, n.y - 1, n.z, lightLevel);
      if (n.y < MAX_CHUNK_HEIGHT - 1)
         updateSurroundingBlock(&q, n.c, n.x, n.y + 1, n.z, lightLevel);
   }

   // These are the RenderChunks that need their lightmaps regenerated.
   for (S32 i = 0; i < sb_count(chunkList); ++i) {
      Chunk *c = chunkList[i].c;
      S32 index = chunkList[i].renderChunkIndex;
      RenderChunk *r = &c->renderChunks[index];
      freeGenerateUpdate(c, r, index);
   }

   sb_free(chunkList);
   lightqueue_cleanup(&q);
}
