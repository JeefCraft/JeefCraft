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

#include <math.h>
#include <assert.h>
#include <string.h>
#include <stretchy_buffer.h>
#include <open-simplex-noise.h>
#include "game/camera.h"
#include "graphics/shader.h"
#include "graphics/texture2d.h"
#include "math/frustum.h"
#include "math/ray.h"
#include "platform/input.h"
#include "math/screenWorld.h"
#include "math/aabb.h"
#include "platform/input.h"
#include "world/material.h"
#include "world/util.h"
#include "world/terrainGen.h"
#include "world/worldMap.h"
#include "world/lighting.h"

// Our map world
ChunkTable gChunkTable;

// Grid size but should be variable. This is the 'chunk distance'.
S32 worldSize = 4;

GLuint projMatrixLoc;
GLuint modelMatrixLoc;
GLuint textureLoc;
U32 program;
Texture2D textureAtlas;

#define TEXTURE_ATLAS_COUNT_I 32
#define TEXTURE_ATLAS_COUNT_F 32.0f

static void buildFace(Chunk *chunk, S32 index, S32 side, F32 light, S32 material, Vec3 localPos) {
   // Vertex data first, then index data.

   RenderChunk *renderChunk = &chunk->renderChunks[index];

   for (S32 i = 0; i < 4; ++i) {
      GPUVertex v;
      v.position.x = cubes[side][i][0] + localPos.x;
      v.position.y = cubes[side][i][1] + localPos.y;
      v.position.z = cubes[side][i][2] + localPos.z;
      v.position.w = cubes[side][i][3];
      v.uvx = (F32)(cubeUVs[side][i][0] + ((F32)(material % TEXTURE_ATLAS_COUNT_I))) / TEXTURE_ATLAS_COUNT_F;
      v.uvy = (F32)(cubeUVs[side][i][1] + ((F32)(material / TEXTURE_ATLAS_COUNT_I))) / TEXTURE_ATLAS_COUNT_F;
      v.light = light;
      v.padding = 1.0f;
      sb_push(renderChunk->vertexData, v);
   }
   renderChunk->vertexCount += 4;

   GPUIndex in = renderChunk->currentIndex;
   sb_push(renderChunk->indices, in);
   sb_push(renderChunk->indices, in + 2);
   sb_push(renderChunk->indices, in + 1);
   sb_push(renderChunk->indices, in);
   sb_push(renderChunk->indices, in + 3);
   sb_push(renderChunk->indices, in + 2);
   renderChunk->currentIndex += 4;
   renderChunk->indiceCount += 6;
}

void generateGeometryForRenderChunk(Chunk *chunk, S32 renderChunkId) {
   Cube *cubeData = chunk->cubeData;
   S32 chunkX = chunk->startX;
   S32 chunkZ = chunk->startZ;

   for (S32 x = 0; x < CHUNK_WIDTH; ++x) {
      for (S32 z = 0; z < CHUNK_WIDTH; ++z) {
         for (S32 j = 0; j < RENDER_CHUNK_HEIGHT; ++j) {
            S32 y = (RENDER_CHUNK_HEIGHT * renderChunkId) + j;
            Vec3 localPos;
            localPos.x = (F32)x;
            localPos.y = (F32)y;
            localPos.z = (F32)z;

            // skip if current block is transparent.
            if (isTransparent(cubeData, x, y, z))
               continue;

            // Cross chunk checking. Only need to check x and z axes.
            // If the next *chunk* over is is transparent then ya we have
            // to render regardless.
            bool isOpaqueNegativeX = false;
            bool isOpaquePositiveX = false;
            bool isOpaqueNegativeZ = false;
            bool isOpaquePositiveZ = false;

            Chunk *c = chunktable_getAt(&gChunkTable, chunkX - CHUNK_WIDTH, chunkZ);
            if (x == 0 && c != NULL) {
               Cube *behindData = c->cubeData;
               //Cube *behindData = getChunkAt(chunkX - 1, chunkZ)->cubeData;
               if (!isTransparent(behindData, CHUNK_WIDTH - 1, y, z)) {
                  // The cube behind us on the previous chunk is in fact
                  // transparent. We need to render this face.
                  isOpaqueNegativeX = true;
               }
            }
            c = chunktable_getAt(&gChunkTable, chunkX + CHUNK_WIDTH, chunkZ);
            if (x == (CHUNK_WIDTH - 1) && c != NULL) {
               Cube *behindData = c->cubeData;
               //Cube *behindData = getChunkAt(chunkX + 1, chunkZ)->cubeData;
               if (!isTransparent(behindData, 0, y, z)) {
                  // The cube behind us on the previous chunk is in fact
                  // transparent. We need to render this face.
                  isOpaquePositiveX = true;
               }
            }
            c = chunktable_getAt(&gChunkTable, chunkX, chunkZ - CHUNK_WIDTH);
            if (z == 0 && c != NULL) {
               Cube *behindData = c->cubeData;
               //Cube *behindData = getChunkAt(chunkX, chunkZ - 1)->cubeData;
               if (!isTransparent(behindData, x, y, CHUNK_WIDTH - 1)) {
                  // The cube behind us on the previous chunk is in fact
                  // transparent. We need to render this face.
                  isOpaqueNegativeZ = true;
               }
            }
            c = chunktable_getAt(&gChunkTable, chunkX, chunkZ + CHUNK_WIDTH);
            if (z == (CHUNK_WIDTH - 1) && c != NULL) {
               Cube *behindData = c->cubeData;
               //Cube *behindData = getChunkAt(chunkX, chunkZ + 1)->cubeData;
               if (!isTransparent(behindData, x, y, 0)) {
                  // The cube behind us on the previous chunk is in fact
                  // transparent. We need to render this face.
                  isOpaquePositiveZ = true;
               }
            }

            // check all 6 directions to see if the cube is exposed.
            // If the cube is exposed in that direction, render that face.

            S32 material = getCubeAt(cubeData, x, y, z)->material;
            F32 light = (F32)chunk_getBlockLight(chunk, x, y, z);

            if (y >= (MAX_CHUNK_HEIGHT - 1) || isTransparent(cubeData, x, y + 1, z))
               buildFace(chunk, renderChunkId, CubeSides_Up, light, material, localPos);

            // If this is grass, bottom has to be dirt.

            if (y == 0 || isTransparent(cubeData, x, y - 1, z))
               buildFace(chunk, renderChunkId, CubeSides_Down, light, (material == Material_Grass ? Material_Dirt : material), localPos);

            // After we built the top, this is a special case for grass.
            // If we are actually building grass sides it has to be special.
            if (material == Material_Grass)
               material = Material_Grass_Side;

            if ((!isOpaqueNegativeX && x == 0) || (x > 0 && isTransparent(cubeData, x - 1, y, z)))
               buildFace(chunk, renderChunkId, CubeSides_West, light, material, localPos);

            if ((!isOpaquePositiveX && x >= (CHUNK_WIDTH - 1)) || (x < (CHUNK_WIDTH - 1) && isTransparent(cubeData, x + 1, y, z)))
               buildFace(chunk, renderChunkId, CubeSides_East, light, material, localPos);

            if ((!isOpaqueNegativeZ && z == 0) || (z > 0 && isTransparent(cubeData, x, y, z - 1)))
               buildFace(chunk, renderChunkId, CubeSides_South, light, material, localPos);

            if ((!isOpaquePositiveZ && z >= (CHUNK_WIDTH - 1)) || (z < (CHUNK_WIDTH - 1) && isTransparent(cubeData, x, y, z + 1)))
               buildFace(chunk, renderChunkId, CubeSides_North, light, material, localPos);
         }
      }
   }
}

F32 getViewDistance() {
   // Give 1 chunk 'padding' looking forward.
   return worldSize * CHUNK_WIDTH + CHUNK_WIDTH;
}

void generateGeometry(Chunk *chunk) {
   for (S32 i = 0; i < CHUNK_SPLITS; ++i) {
      generateGeometryForRenderChunk(chunk, i);
   }
}

GLuint singleBufferCubeVBO;
GLuint singleBufferCubeIBO;

void uploadRenderChunkToGL(RenderChunk *r) {
   if (r->vertexCount > 0) {
      glGenBuffers(1, &r->vbo);
      glGenBuffers(1, &r->ibo);

      glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(GPUVertex) * r->vertexCount, r->vertexData, GL_STATIC_DRAW);

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r->ibo);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GPUIndex) * r->indiceCount, r->indices, GL_STATIC_DRAW);
   }

   // Free right after uploading to the GL. We don't need gpu data
   // in both system and gpu ram.
   sb_free(r->vertexData);
   sb_free(r->indices);
   r->vertexData = NULL;
   r->indices = NULL;
}

void uploadChunkToGL(Chunk *chunk) {
   for (S32 i = 0; i < CHUNK_SPLITS; ++i) {
      RenderChunk *r = &chunk->renderChunks[i];
      uploadRenderChunkToGL(r);
   }
}

static inline void freeRenderChunkGL(RenderChunk *r) {
   if (r->vertexCount > 0) {
      glDeleteBuffers(1, &r->vbo);
      glDeleteBuffers(1, &r->ibo);
   }
   memset(r, 0, sizeof(RenderChunk));
}

void freeChunkGL(Chunk *chunk) {
   for (S32 i = 0; i < CHUNK_SPLITS; ++i) {
      RenderChunk *r = &chunk->renderChunks[i];
      freeRenderChunkGL(r);
   }
}

void uploadGeometryToGL() {
   // *Single threaded*
   //
   // Upload to the GL
   // Now that we generated all of the geometry, upload to the GL.
   // Note if a chunk has no geometry we don't create a vbo
   //
   // TODO: use VAO if extension is supported??
   for (S32 x = -worldSize; x < worldSize; ++x) {
      for (S32 z = -worldSize; z < worldSize; ++z) {
         Chunk *chunk = chunktable_getAt(&gChunkTable, x * CHUNK_WIDTH, z * CHUNK_WIDTH);
         uploadChunkToGL(chunk);
      }
   }

   // Single buffer cube vbo/ibo
   glGenBuffers(1, &singleBufferCubeVBO);
   glBindBuffer(GL_ARRAY_BUFFER, singleBufferCubeVBO);
   glBufferData(GL_ARRAY_BUFFER, sizeof(F32) * 6 * 4 * 4, cubes, GL_STATIC_DRAW);

   glGenBuffers(1, &singleBufferCubeIBO);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, singleBufferCubeIBO);
   
   GPUIndex indices[36];
   S32 in = 0;
   for (S32 i = 0; i < 36; i += 6) {
      indices[i] = in;
      indices[i + 1] = in + 2;
      indices[i + 2] = in + 1;
      indices[i + 3] = in;
      indices[i + 4] = in + 3;
      indices[i + 5] = in + 2;

      in += 4;
   }
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GPUIndex) * 36, indices, GL_STATIC_DRAW);
}

int gVisibleChunks = 0;
int gTotalVisibleChunks = 0;
int gTotalChunks = 0;


S32 pickerShaderProjMatrixLoc;
S32 pickerShaderModelMatrixLoc;
GLuint pickerProgram;

// status of picking so we can't pick more than 1 in same keypress/mousepress
int pickerStatus = RELEASED;

// status of placing blocks so we can't spam place blocks
int placingStatus = RELEASED;

// status of placing torch
int placingTorchStatus = RELEASED;

void initWorld() {
   // Only 2 mip levels.
   bool ret = createTexture2D("Assets/block_atlas.png", 4, 2, &textureAtlas);
   if (!ret) {
      exit(-3);
   }

   // Create shader
   generateShaderProgram("Shaders/basic.vert", "Shaders/basic.frag", &program);
   projMatrixLoc = glGetUniformLocation(program, "projViewMatrix");
   modelMatrixLoc = glGetUniformLocation(program, "modelMatrix");
   textureLoc = glGetUniformLocation(program, "textureAtlas");

   // Create shader for picker
   generateShaderProgram("Shaders/red.vert", "Shaders/red.frag", &pickerProgram);
   pickerShaderProjMatrixLoc = glGetUniformLocation(pickerProgram, "projViewMatrix");
   pickerShaderModelMatrixLoc = glGetUniformLocation(pickerProgram, "modelMatrix");

   initTerrainGen();

   // Generate initial chunks.
   // Note that simply alloating here it is better to allocate up front before generating
   // the entire world as it will cause less thread contention with just inserting up front.
   // TODO: handle threading on the chunktable datastructure!
   gTotalChunks = worldSize * 2 * worldSize * 2 * CHUNK_SPLITS;
   chunktable_create(gTotalChunks / CHUNK_SPLITS, &gChunkTable);
   for (S32 x = -worldSize; x < worldSize; ++x) {
      for (S32 z = -worldSize; z < worldSize; ++z) {
         chunktable_insertAt(&gChunkTable, x * CHUNK_WIDTH, z * CHUNK_WIDTH);
      }
   }

   // Easilly put each chunk in a thread in here.
   // nothing OpenGL, all calculation and world generation.
//#pragma omp parallel for
   for (S32 x = -worldSize; x < worldSize; ++x) {
      for (S32 z = -worldSize; z < worldSize; ++z) {
         // World position calcuation before passing.
         generateWorld(x * CHUNK_WIDTH, z * CHUNK_WIDTH);
      }
   }

   // TODO MULTITHREADED: sync here before generating the Geometry.

   // Generate caves and tree
//#pragma omp parallel for
   for (S32 x = -worldSize; x < worldSize; ++x) {
      for (S32 z = -worldSize; z < worldSize; ++z) {
         generateCavesAndStructures(x * CHUNK_WIDTH, z * CHUNK_WIDTH);
      }
   }

   // Easilly put each chunk in a thread in here.
   // nothing OpenGL, all calculation and world generation.
//#pragma omp parallel for
   for (S32 x = -worldSize; x < worldSize; ++x) {
      for (S32 z = -worldSize; z < worldSize; ++z) {
         Chunk *chunk = chunktable_getAt(&gChunkTable, x * CHUNK_WIDTH, z * CHUNK_WIDTH);
         generateGeometry(chunk);
      }
   }

   // TODO MULTITHREADED: sync here before GL upload.

   uploadGeometryToGL();
}

static void freeChunk(const Chunk *chunk) {
   free(chunk->cubeData);
   chunk_freeLightmap((Chunk*)chunk);
   freeChunkGL((Chunk*)chunk);
}

void freeWorld() {
   chunktable_foreach(&gChunkTable, freeChunk);

   chunktable_free(&gChunkTable);
   freeTerrainGen();
}

void freeGenerateUpdate(Chunk *c, RenderChunk *r, S32 renderChunkId) {
   assert(c);
   assert(r);

   freeRenderChunkGL(r);
   generateGeometryForRenderChunk(c, renderChunkId);
   uploadRenderChunkToGL(r);
}

static void remeshChunkGeometryAtGlobalPos(S32 x, S32 y, S32 z) {
   // Rebuild this *render chunk*
   S32 renderChunkId;
   Chunk *c = getChunkAtWorldSpacePosition(x, y, z);
   RenderChunk *r = getRenderChunkAtWorldSpacePosition(x, y, z, &renderChunkId);
   freeGenerateUpdate(c, r, renderChunkId);

   // Check x,y,z axes to see if they lay on render chunk boundaries.
   // If they do, we need to update the render chunk that is next to it.
   S32 localX, localY, localZ;
   globalPosToLocalPos(x, y, z, &localX, &localY, &localZ);

   if (localX == 0) {
      c = getChunkAtWorldSpacePosition(x - CHUNK_WIDTH, y, z);
      r = getRenderChunkAtWorldSpacePosition(x - CHUNK_WIDTH, y, z, &renderChunkId);
      freeGenerateUpdate(c, r, renderChunkId);
   }
   else if (localX >= (CHUNK_WIDTH - 1)) {
      c = getChunkAtWorldSpacePosition(x + CHUNK_WIDTH, y, z);
      r = getRenderChunkAtWorldSpacePosition(x + CHUNK_WIDTH, y, z, &renderChunkId);
      freeGenerateUpdate(c, r, renderChunkId);
   }

   if (localY == 0) {
      c = getChunkAtWorldSpacePosition(x, y - RENDER_CHUNK_HEIGHT, z);
      r = getRenderChunkAtWorldSpacePosition(x, y - RENDER_CHUNK_HEIGHT, z, &renderChunkId);
      freeGenerateUpdate(c, r, renderChunkId);
   }
   else if (localY >= (RENDER_CHUNK_HEIGHT - 1)) {
      c = getChunkAtWorldSpacePosition(x, y + RENDER_CHUNK_HEIGHT, z);
      r = getRenderChunkAtWorldSpacePosition(x, y + RENDER_CHUNK_HEIGHT, z, &renderChunkId);
      freeGenerateUpdate(c, r, renderChunkId);
   }

   if (localZ == 0) {
      c = getChunkAtWorldSpacePosition(x, y, z - CHUNK_WIDTH);
      r = getRenderChunkAtWorldSpacePosition(x, y, z - CHUNK_WIDTH, &renderChunkId);
      freeGenerateUpdate(c, r, renderChunkId);
   }
   else if (localZ >= (CHUNK_WIDTH - 1)) {
      c = getChunkAtWorldSpacePosition(x, y, z + CHUNK_WIDTH);
      r = getRenderChunkAtWorldSpacePosition(x, y, z + CHUNK_WIDTH, &renderChunkId);
      freeGenerateUpdate(c, r, renderChunkId);
   }
}

void removeCubeAtWorldPosition(Cube *cube, S32 x, S32 y, S32 z) {
   // Jeff - We have infinite world, we don't need this anymore as we always have cubes around us.
   // Bounds check on removing cube if we are at a boundary.
   /*if (x <= -worldSize * CHUNK_WIDTH ||
      x >= worldSize * CHUNK_WIDTH ||
      z <= -worldSize * CHUNK_WIDTH ||
      z >= worldSize * CHUNK_WIDTH ||
      y <= 0 ||
      y >= MAX_CHUNK_HEIGHT) {
      printf("Cannot remove cube at %d %d %d. It is at a world edge boundary!\n", x, y, z);
      return;
   }*/

   cube->material = Material_Air;
   remeshChunkGeometryAtGlobalPos(x, y, z);
}

void addCubeAtGlobalPos(Vec3 position) {
   S32 x = (S32)position.x;
   S32 y = (S32)position.y;
   S32 z = (S32)position.z;

   // Bounds check on removing cube if we are at a boundary.
   // Jeff - We have infinite world, we don't need this anymore as we always have cubes around us.
   /*if (x <= -worldSize * CHUNK_WIDTH ||
      x >= worldSize * CHUNK_WIDTH ||
      z <= -worldSize * CHUNK_WIDTH ||
      z >= worldSize * CHUNK_WIDTH ||
      y <= 0 ||
      y >= MAX_CHUNK_HEIGHT) {
      printf("Cannot remove cube at %d %d %d. It is at a world edge boundary!\n", x, y, z);
      return;
   }
   */

   // Just add bedrock for now.
   getGlobalCubeAtWorldSpacePosition(x, y, z)->material = Material_Bedrock;
   remeshChunkGeometryAtGlobalPos(x, y, z);
}

// Cube xyz
void checkCubeAtLookAtCube(Vec3 cameraOrigin, Vec3 cameraDir, S32 x, S32 y, S32 z) {
   Vec3 cubePos = create_vec3((F32)x, (F32)y, (F32)z);

   Vec4 planes[6];
   planes[0] = create_vec4(cubeNormals[0][0], cubeNormals[0][1], cubeNormals[0][2], x + 1); // East
   planes[1] = create_vec4(cubeNormals[1][0], cubeNormals[1][1], cubeNormals[1][2], y + 1); // Up
   planes[2] = create_vec4(cubeNormals[2][0], cubeNormals[2][1], cubeNormals[2][2], -x);    // West
   planes[3] = create_vec4(cubeNormals[3][0], cubeNormals[3][1], cubeNormals[3][2], -y);    // Down
   planes[4] = create_vec4(cubeNormals[4][0], cubeNormals[4][1], cubeNormals[4][2], z + 1); // North
   planes[5] = create_vec4(cubeNormals[5][0], cubeNormals[5][1], cubeNormals[5][2], -z);    // South

   // Get the face of the cube that we are touching.
   for (S32 i = 0; i < 6; ++i) {
      Vec3 norm = create_vec3(planes[i].x, planes[i].y, planes[i].z);
      if (glm_vec_dot(cameraDir.vec, norm.vec) < 0.0f) {
         Vec3 outPos;
         if (rayIntersectsPlane(cameraOrigin, cameraDir, planes[i], &outPos)) {
            // Cube test 'if cube contains point'
            if (outPos.x >= cubePos.x && outPos.x <= cubePos.x + 1 &&
                outPos.y >= cubePos.y && outPos.y <= cubePos.y + 1 &&
                outPos.z >= cubePos.z && outPos.z <= cubePos.z + 1) {
               // We found our plane!
               glm_vec_add(cubePos.vec, norm.vec, cubePos.vec);
               addCubeAtGlobalPos(cubePos);
               break;
            }
         }
      }
   }
}

bool orthoFlag = false;

static void renderChunk(const Chunk *c) {
   Frustum frustum;
   getCameraFrustum(&frustum);

   for (S32 i = 0; i < CHUNK_SPLITS; ++i) {
      if (c->renderChunks[i].vertexCount > 0) {
         gTotalVisibleChunks++;

         // Set position.
         // Center y pos should actually be RENDER_CHUNK_HEIGHT * i
         // but pos should always be 0 for y since the pos is baked into the y coord.
         Vec3 pos = create_vec3(c->startX, 0.0f, c->startZ);
         Vec3 center;
         Vec3 halfExtents = create_vec3(CHUNK_WIDTH / 2.0f, RENDER_CHUNK_HEIGHT / 2.0f, CHUNK_WIDTH / 2.0f);
         glm_vec_add(pos.vec, halfExtents.vec, center.vec);
         center.y += (F32)(i * RENDER_CHUNK_HEIGHT); // We add since we already have RENDER_CHUNK_HEIGHT / 2.0

         if (FrustumCullSquareBox(&frustum, center, CHUNK_WIDTH / 2.0f)) {
            mat4 modelMatrix;
            glm_mat4_identity(modelMatrix);
            glm_translate(modelMatrix, pos.vec);
            glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &(modelMatrix[0][0]));
            glBindBuffer(GL_ARRAY_BUFFER, c->renderChunks[i].vbo);
            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GPUVertex), (void*)offsetof(GPUVertex, position));
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GPUVertex), (void*)offsetof(GPUVertex, uvx));
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GPUVertex), (void*)offsetof(GPUVertex, light));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, c->renderChunks[i].ibo);
            glDrawElements(GL_TRIANGLES, (GLsizei)c->renderChunks[i].indiceCount, GL_UNSIGNED_INT, (void*)0);
            glDisableVertexAttribArray(0);
            glDisableVertexAttribArray(1);
            glDisableVertexAttribArray(2);

            gVisibleChunks++;
         }
      }
   }
}

void renderWorld(F32 dt) {
   // Set GL State
   glEnable(GL_CULL_FACE);
   glCullFace(GL_BACK);
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LESS);

   glUseProgram(program);

   // proj/view matrix
   mat4 proj, view, projView;

   if (inputGetKeyStatus(KEY_V) == PRESSED)
      orthoFlag = true;
   else
      orthoFlag = false;

   // For debugging culling
   if (!orthoFlag) {
      getCurrentProjMatrix(&proj);
      getCurrentViewMatrix(&view);
   } else {
      glm_ortho(-256.0f, 256.0f, -256.0f / (1440.f / 900.f), 256.0f / (1440.f / 900.f), -200.0f, 200.0f, proj);
      Vec3 eye = create_vec3(0.0f, 0.0f, 0.0f);
      Vec3 center = create_vec3(0.0f, -1.0f, 0.0f);
      Vec3 up = create_vec3(1.0f, 0.0f, 0.0f);
      glm_lookat(eye.vec, center.vec, up.vec, view);
   }
   glm_mat4_mul(proj, view, projView);
   glUniformMatrix4fv(projMatrixLoc, 1, GL_FALSE, &(projView[0][0]));

   // Bind our texture atlas to texture unit 0
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, textureAtlas.glId);
   glUniform1i(textureLoc, 0);

   gVisibleChunks = 0;
   gTotalVisibleChunks = 0;

   // Render each chunk.
   // TODO: split up frustum culling and sort chunks front to back
   // before actually rendering.
   chunktable_foreach(&gChunkTable, renderChunk);

   // Do our raycast to screen world.
   Vec3 rayOrigin;
   Vec4 rayDir;
   screenRayToWorld(view, &rayOrigin, &rayDir);

   // Check to see if we have something within 8 blocks away.
   Vec3 point = rayOrigin;
   Vec3 scalar;
   glm_vec_scale(rayDir.vec, 0.01f, scalar.vec);
   for (S32 i = 0; i < 800; ++i) {
      glm_vec_add(point.vec, scalar.vec, point.vec);

      Vec3 pos = create_vec3(floorf(point.x), floorf(point.y), floorf(point.z));

      // Calculate chunk at point.
      Cube *c = getGlobalCubeAtWorldSpacePosition((S32)pos.x, (S32)pos.y, (S32)pos.z);
      if (c != NULL && c->material != Material_Air) {
         glUseProgram(pickerProgram);

         glUniformMatrix4fv(pickerShaderProjMatrixLoc, 1, GL_FALSE, &(projView[0][0]));
         mat4 modelMatrix;
         glm_mat4_identity(modelMatrix);
         glm_translate(modelMatrix, pos.vec);

         Vec3 scale = create_vec3(1.2f, 1.2f, 1.2f);
         Vec3 trans = create_vec3(-0.1f, -0.1f, -0.1f);
         glm_scale(modelMatrix, scale.vec);
         glm_translate(modelMatrix, trans.vec);

         glUniformMatrix4fv(pickerShaderModelMatrixLoc, 1, GL_FALSE, &(modelMatrix[0][0]));

         glBindBuffer(GL_ARRAY_BUFFER, singleBufferCubeVBO);
         glEnableVertexAttribArray(0);
         glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(F32) * 4, (void*)0);
         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, singleBufferCubeIBO);
         glDrawElements(GL_TRIANGLES, (GLsizei)36, GL_UNSIGNED_INT, (void*)0);
         glDisableVertexAttribArray(0);

         // TODO: Have mouse click. For now hit the G key.
         if (pickerStatus == RELEASED && inputGetKeyStatus(KEY_G) == PRESSED) {
            removeCubeAtWorldPosition(c, (S32)pos.x, (S32)pos.y, (S32)pos.z);
            pickerStatus = PRESSED;
         } else if (inputGetKeyStatus(KEY_G) == RELEASED) {
            // set picker status to released
            pickerStatus = RELEASED;
         }

         if (placingStatus == RELEASED && inputGetKeyStatus(KEY_H) == PRESSED) {
            Vec3 rayDirection = create_vec3(rayDir.x, rayDir.y, rayDir.z);
            checkCubeAtLookAtCube(rayOrigin, rayDirection, (S32)pos.x, (S32)pos.y, (S32)pos.z);
            placingStatus = PRESSED;
         } else if (inputGetKeyStatus(KEY_H) == RELEASED) {
            placingStatus = RELEASED;
         }

         if (placingStatus == RELEASED && inputGetKeyStatus(KEY_T) == PRESSED) {
            S32 localX, localY, localZ;
            Chunk *chunk = getChunkAtWorldSpacePosition((S32)pos.x, (S32)pos.y, (S32)pos.z);
            globalPosToLocalPos((S32)pos.x, (S32)pos.y, (S32)pos.z, &localX, &localY, &localZ);
            chunk_setBlockLight(chunk, localX, (S32)pos.y /* GLOBAL Y */, localZ, MAX_LIGHT_LEVEL);
            placingTorchStatus = PRESSED;
         } else if (inputGetKeyStatus(KEY_H) == RELEASED) {
            placingTorchStatus = RELEASED;
         }

         break;
      }
   }

   //return;

   // to keep infinite terrain with the world size, create new terrain here.
   // Note that this is far from optimal, but this is just for proof of concept purposes.
   Vec3 camPos;
   getCameraPosition(&camPos);
   camPos.y = 0; // don't care about y pos.
   
   typedef struct PositionToRemove {
      S32 x;
      S32 z;
   } PositionToRemove;
   PositionToRemove **positionsToRemove = NULL;

   const F32 DIST = (F32)CHUNK_WIDTH * (F32)worldSize + (F32)CHUNK_WIDTH;  // 1 chunk padding.

   // first, free all chunks that are out of scope.
   // have to build an array of stuff to remove as we can't remove as we iterate.
   for (S32 i = 0; i < gChunkTable.count; ++i) {
      Chunk *c = &gChunkTable.chunkTable[i];

      Vec3 pos = create_vec3((F32)c->startX + (CHUNK_WIDTH / 2.0f), 0, (F32)c->startZ + (CHUNK_WIDTH / 2.0f));
      
      // Check x and z distance independently.
      if (fabsf(camPos.x - pos.x) > DIST ||fabsf(camPos.z - pos.z) > DIST) {
         // out of range, remove
         PositionToRemove *p = (PositionToRemove*)malloc(sizeof(PositionToRemove));
         p->x = c->startX;
         p->z = c->startZ;
         sb_push(positionsToRemove, p);
      }
   }

   for (S32 i = 0; i < sb_count(positionsToRemove); ++i) {
      chunktable_removeAt(&gChunkTable, positionsToRemove[i]->x, positionsToRemove[i]->z);
      free(positionsToRemove[i]);
   }
   sb_free(positionsToRemove);
   
   // now add new chunk

   // Need to round campos to CHUNK_WIDTH
   S32 centerX = (S32)(camPos.x / CHUNK_WIDTH) * CHUNK_WIDTH;
   S32 centerZ = (S32)(camPos.z / CHUNK_WIDTH) * CHUNK_WIDTH;

   // Build grid around center. any chunks that don't exist add them to the world.
   for (S32 x = -worldSize; x < worldSize; ++x) {
      for (S32 z = -worldSize; z < worldSize; z++) {
         S32 chunkX = (x * CHUNK_WIDTH) + centerX;
         S32 chunkZ = (z * CHUNK_WIDTH) + centerZ;

         // Check to see if we have a chunk at this position. If we don't, make one!
         // TODO: a hashtable would be faster here instead of O(n)
         if (chunktable_getAt(&gChunkTable, chunkX, chunkZ) == NULL) {
            // Insert chunk.
            chunktable_insertAt(&gChunkTable, chunkX, chunkZ);
            Chunk *chunk = chunktable_getAt(&gChunkTable, chunkX, chunkZ);
            generateWorld(chunkX, chunkZ);
            generateCavesAndStructures(chunkX, chunkZ);
            generateGeometry(chunk);
            uploadChunkToGL(chunk);
         }
      }
   }
}
