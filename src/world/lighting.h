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

#ifndef _WORLD_LIGHTING_H_
#define _WORLD_LIGHTING_H_

#include "world/world.h"

void chunk_initLightmap(Chunk *chunk);
void chunk_freeLightmap(Chunk *chunk);
S32 chunk_getBlockLight(Chunk *chunk, S32 x, S32 y, S32 z);
void chunk_setBlockLight(Chunk *chunk, S32 x, S32 y, S32 z, S32 value);

#define LIGHTQUEUE_TEST

#ifdef LIGHTQUEUE_TEST
void LIGHTQUEUE_TEST_FN();
#endif

#endif