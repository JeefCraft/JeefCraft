#------------------------------------------------------------------------------
# Copyright 2018 Jeff Hutchinson
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# Set C99, for MSVC we compile as C++98 as MSVC doesn't support C99
# Until as of MSVC 2015
if (MSVC)
	set(CMAKE_CXX_STANDARD 98)

	# Flag ref:
	#
	# /TP - Compile as C++
	# /MP - Compile with multiple processors.
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /TP /MP")
else()
	set(CMAKE_C_STANDARD 99)
endif()

set(THIRDPARTY_DIR "<path here>" CACHE PATH "Sets the ThirdParty directory")
set(EXECUTABLE_NAME "JeefCraft" CACHE STRING "Sets the name of the executable")

#Find OpenGL
find_package(OpenGL REQUIRED)

# GLEW build
add_library(glew STATIC "${THIRDPARTY_DIR}/glew/src/glew.c")
target_include_directories(glew PUBLIC "${THIRDPARTY_DIR}/glew/include" ${OPENGL_INCLUDE_DIR})
target_compile_definitions(glew PUBLIC GLEW_STATIC)

# Open Simplex Noise build
add_library(open_simplex_noise STATIC "${THIRDPARTY_DIR}/opensimplexnoise/open-simplex-noise.c")
target_include_directories(open_simplex_noise PUBLIC "${THIRDPARTY_DIR}/opensimplexnoise/")


# Link GLFW3
add_subdirectory("${THIRDPARTY_DIR}/glfw3" "${CMAKE_BINARY_DIR}/ThirdParty")

set(JEEFCRAFT_SRC 
	src/base/io.c
	src/base/io.h
	src/base/types.h

	src/game/camera.c
	src/game/camera.h

	src/graphics/shader.c
	src/graphics/shader.h
	src/graphics/texture2d.c
	src/graphics/texture2d.h

	src/main/main.c

	src/math/aabb.c
	src/math/aabb.h
	src/math/frustum.c
	src/math/frustum.h
	src/math/math.h
	src/math/ray.c
	src/math/ray.h
	src/math/screenWorld.c
	src/math/screenWorld.h

	src/platform/input.h
	src/platform/platform.h
	src/platform/window.h

	src/platform/glfw3/glfw3Input.c
	src/platform/glfw3/glfw3Platform.c
	src/platform/glfw3/glfw3Window.c

	src/world/cube.h
	src/world/lighting.c
	src/world/lighting.h
	src/world/material.h
	src/world/terrainGen.c
	src/world/terrainGen.h
	src/world/util.c
	src/world/util.h
	src/world/world.c
	src/world/world.h
	src/world/worldMap.c
	src/world/worldMap.h
)
add_executable(${EXECUTABLE_NAME} ${JEEFCRAFT_SRC})
target_link_libraries(${EXECUTABLE_NAME}
	${OPENGL_LIBRARIES}
	glfw
	glew
	open_simplex_noise	
)

# Platform specific library linking.
if (MSVC)
	target_link_libraries(${EXECUTABLE_NAME} 
		kernel32.lib
	)

	# Files must be set to compile with the C++ compiler on MSVC
	set_source_files_properties(${JEEFCRAFT_SRC} PROPERTIES LANGUAGE CXX)
	set_target_properties(${EXECUTABLE_NAME} PROPERTIES LINKER_LANGUAGE CXX)	
elseif(APPLE)
	target_link_libraries(${EXECUTABLE_NAME} 
		"-framework Cocoa"
	)
endif()

target_include_directories(${EXECUTABLE_NAME}
	PUBLIC ${OPENGL_INCLUDE_DIR}
	PUBLIC "${THIRDPARTY_DIR}/glfw3/include"
	PUBLIC "${THIRDPARTY_DIR}/stb"
	PUBLIC "${THIRDPARTY_DIR}/opensimplexnoise"
	PUBLIC "${THIRDPARTY_DIR}/cglm/include"
	PUBLIC src
)

target_compile_definitions(${EXECUTABLE_NAME} PUBLIC RAYMATH_STANDALONE)

source_group("base" REGULAR_EXPRESSION src/base/*)
source_group("game" REGULAR_EXPRESSION src/game/*)
source_group("graphics" REGULAR_EXPRESSION src/graphics/*)
source_group("main" REGULAR_EXPRESSION src/main/*)
source_group("math" REGULAR_EXPRESSION src/math/*)
source_group("platform" REGULAR_EXPRESSION src/platform/*)
source_group("platform\\glfw3" REGULAR_EXPRESSION src/platform/glfw3/*)
source_group("world" REGULAR_EXPRESSION src/world/*)