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

#ifndef _GAME_WORLD_H_
#define _GAME_WORLD_H_

#include <GL/glew.h>
#include "base/types.h"
#include "math/math.h"
#include "world/cube.h"

typedef struct GPUVertex {
   Vec4 position;
   F32 uvx;
   F32 uvy;
} GPUVertex;

typedef U32 GPUIndex;

typedef struct Cube {
   U16 material : 10; // 1024 material types
   U16 light : 4;     // 0-15 light level
   U16 flag1 : 1;     // 1-bit extra flag
   U16 flag2 : 1;     // 1-bit extra flag
} Cube;

// TODO: store a list of pointers of RenderChunk array (RenderChunk**)
// into a Chunk datastructure. That way we can access the RenderChunk
// and update it accordingly when we break a block. We can calculate
// based on the position of the block breaking what position the RenderChunk
// is in.

typedef struct RenderChunk {
   GPUVertex *vertexData; /// stretchy buffer
   GPUIndex *indices;          /// stretchy buffer
   GPUIndex currentIndex;      /// Current index offset
   GPUIndex indiceCount;       /// Indice Size
   S32 vertexCount;       /// VertexData Count 

   GLuint vbo;            /// OpenGL Vertex Buffer Object
   GLuint ibo;            /// OpenGL Index Buffer Object
} RenderChunk;

typedef struct Chunk {
   S32 startX;
   S32 startZ;
   Cube *cubeData;                         /// Cube data for full chunk
   RenderChunk renderChunks[CHUNK_SPLITS]; /// Per-render chunk data.
} Chunk;

void initWorld();
void freeWorld();
F32 getViewDistance();
void renderWorld(F32 dt);

#endif // _GAME_WORLD_H_