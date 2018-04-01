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

#ifndef _WORLD_WORLDMAP_H_
#define _WORLD_WORLDMAP_H_

#include <string.h>
#include "base/types.h"
#include "world/world.h"

// TODO: have a 'free list' so we don't have
// to constantly alloc and free new chunks.
//
// TODO: Tie a 'threadpool' to the chunk table
// and have a worker item to do parallel foreach
// could be useful for stuff like frustum culling?

/*
typedef struct {
Chunk chunk;
ChunkFreeNode *next;
} ChunkFreeNode;
*/

typedef struct {
   S32 count;
   S32 tableSize;
   Chunk *chunkTable;
   //ChunkFreeNode *freeList;
} ChunkTable;

// A callback function
typedef void(*ChunkTableForeach)(const Chunk *chunk);

/**
 * Creates a new ChunkTable and initializes it.
 * @param tblSize The initial size of how many Chunks the table can store.
 *  The table will grow as needed if it runs out of space.
 * @param outTable The returned ChunkTable.
 */
void chunktable_create(S32 tblSize, ChunkTable *outTable);

/**
 * Frees a ChunkTable's memory and all chunks within it.
 * @param table The table that we wish to free.
 */
void chunktable_free(ChunkTable *table);

/**
 * @brief Inserts a chunk into the chunktable at worldspace position x,z.
 * @param table The chunk table to insert the chunk into.
 * @param x The x coordinate in worldspace position.
 * @param z The z coordinate in worldspace position.
 * @unstable The pointers to chunks may become invalidated after calling
 * this function.
 */
void chunktable_insertAt(ChunkTable *table, S32 x, S32 z);

/**
 * @brief Removes a chunk at worldspace position x,z.
 * @param table The chunk table to insert the chunk into.
 * @param x The x coordinate in worldspace position.
 * @param z The z coordinate in worldspace position.
 * @unstable The pointers to chunks may become invalidated after calling
 * this function.
 */
void chunktable_removeAt(ChunkTable *table, S32 x, S32 z);

/**
 * @brief Gets a pointer to the chunk at location x,z in worldspace.
 * @param table The chunk table we are looking up.
 * @param x The x coordinate in worldspace.
 * @param z The z coordinate in worldspace.
 * @return Returns a NON-STABLE pointer to the chunk. You cannot rely on
 *  this pointer being stable. Any change to the table that is marked as
 *  "@unstable" from any method invocoation [from any thread] may invalidate
 *  the returned pointer.
 */
Chunk* chunktable_getAt(ChunkTable *table, S32 x, S32 z);

/**
 * @brief Loops over the sorted chunk array, invoking a callback function.
 * @param table The chunk table we are looking up.
 * @param fn The callback function to handle each chunk. The arg in the callback
 *  is a NON-STABLE pointer to the chunk. You cannot rely on this pointer being stable. 
 *  Any change to the table that is marked as "@unstable" from any method invocoation
 *  [from any thread] may invalidate the returned pointer.
 */
void chunktable_foreach(ChunkTable *table, ChunkTableForeach fn);

#endif