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

#include "math/aabb.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

/// @Reference gamedev stack exchange by zacharmarz
/// https://gamedev.stackexchange.com/a/18459
bool rayAABBTest(vec *rayDir, vec *rayOrigin, AABB *aabb) {
   vec inverseDir = vec3(1.0f / rayDir->x, 1.0f / rayDir->y, 1.0f / rayDir->z);

   float test[6];

   // Min
   test[0] = (aabb->min.x - rayOrigin->x) * inverseDir.x;
   test[1] = (aabb->min.y - rayOrigin->y) * inverseDir.y;
   test[2] = (aabb->min.z - rayOrigin->z) * inverseDir.z;
   
   // Max
   test[3] = (aabb->max.x - rayOrigin->x) * inverseDir.x;
   test[4] = (aabb->max.y - rayOrigin->y) * inverseDir.y;
   test[5] = (aabb->max.z - rayOrigin->z) * inverseDir.z;

   // Try min
   float minimum = max(max(min(test[0], test[3]), min(test[1], test[4])), min(test[2], test[5]));
   if (minimum < 0.0f)
      return false;

   // Try max.
   float maximum = min(min(max(test[0], test[3]), max(test[1], test[4])), max(test[2], test[5]));
   if (minimum > maximum)
      return false;

   // Passed.
   return true;
}