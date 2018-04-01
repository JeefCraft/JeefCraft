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

#include <GL/glew.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "base/io.h"
#include "graphics/shader.h"

static const char* shaderTypeToString(GLenum shaderType) {
   switch (shaderType) {
      case GL_VERTEX_SHADER:
         return "Vertex";
      case GL_FRAGMENT_SHADER:
         return "Fragment";
      case GL_COMPUTE_SHADER:
         return "Compute";
   }
   return "(Uknown)";
}

static bool loadShader(GLenum shaderType, const char *file, GLuint *shader) {
   char *contents;
   WordSize contentsLen;
   if (!readTextFile(file, &contents, &contentsLen))
      return false;

   *shader = glCreateShader(shaderType);
   glShaderSource(*shader, 1, (const GLchar *const*)&contents, NULL);
   glCompileShader(*shader);

   // free contents of the file, we are done here as
   // we already compiled the shader.
   free(contents);

   GLint result;
   glGetShaderiv(*shader, GL_COMPILE_STATUS, &result);
   if (result == GL_FALSE) {

      // Let's get the error message from GL.
      GLint logLength;
      glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);

      char *log = (char*)calloc(logLength + 1, sizeof(char));
      glGetShaderInfoLog(*shader, logLength, NULL, log);

      const char *shaderTypeStr = shaderTypeToString(shaderType);
      printf("OpenGL %s Shader Error:\n%s\n", shaderTypeStr, log);

      free(log);
      return false;
   }

   return true;
}

bool generateShaderProgram(const char *vertexFile, const char *fragmentFile, U32 *program) {
   GLuint vertex;
   GLuint fragment;
   if (!loadShader(GL_VERTEX_SHADER, vertexFile, &vertex))
      return false;
   if (!loadShader(GL_FRAGMENT_SHADER, fragmentFile, &fragment))
      return false;

   // create the GL program, attach the shaders, and link.
   // If all goes successful we have a program.

   *program = glCreateProgram();
   glAttachShader(*program, vertex);
   glAttachShader(*program, fragment);
    
   // bind attrib locations
   glBindAttribLocation(*program, 0, "position");
   glBindAttribLocation(*program, 1, "uvs");
   glBindAttribLocation(*program, 2, "lightData");
    
   glLinkProgram(*program);

   // Delete vertex/frag individual shaders as they are linked now.
   glDeleteShader(vertex);
   glDeleteShader(fragment);

   GLint result;
   glGetProgramiv(*program, GL_LINK_STATUS, &result);
   if (result == GL_FALSE) {
      // Let's get the error message from GL.
      GLint logLength;
      glGetProgramiv(*program, GL_INFO_LOG_LENGTH, &logLength);

      char *log = (char*)calloc(logLength + 1, sizeof(char));
      glGetProgramInfoLog(*program, logLength, NULL, log);

      printf("OpenGL Shader Linking Error:\n%s\n", log);

      free(log);

      return false;
   }

   return true;
}
