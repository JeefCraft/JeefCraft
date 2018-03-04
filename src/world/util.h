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

#ifndef _WORLD_UTIL_H_
#define _WORLD_UTIL_H_

#include "world/world.h"

Chunk* getChunkAt(S32 x, S32 z);

Cube* getCubeAt(Cube *cubeData, S32 x, S32 y, S32 z);

bool isTransparent(Cube *cubeData, S32 x, S32 y, S32 z);

bool isTransparentAtCube(Cube *c);

Chunk* getChunkAtWorldSpacePosition(S32 x, S32 y, S32 z);

RenderChunk* getRenderChunkAtWorldSpacePosition(S32 x, S32 y, S32 z, S32 *renderChunkIndex);

void globalPosToLocalPos(S32 x, S32 y, S32 z, S32 *localX, S32 *localY, S32 *localZ);

Cube* getGlobalCubeAtWorldSpacePosition(S32 x, S32 y, S32 z);

#endif