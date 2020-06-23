cmake_minimum_required (VERSION 3.5)

# Always generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo) # good compromise
endif(NOT CMAKE_BUILD_TYPE)

set(TARGET_NAME simple_skeletal_animation)
project(${TARGET_NAME} VERSION 1.0 LANGUAGES CXX C)

if (CMAKE_BUILD_TYPE MATCHES "Debug")
  set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
  set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
else()
  set(CMAKE_CXX_FLAGS_DEBUG "-O3")
  set(CMAKE_C_FLAGS_DEBUG "-O3")
endif (CMAKE_BUILD_TYPE MATCHES "Debug")

find_package(glfw3 3.2 REQUIRED)

include_directories(.)
include_directories(include)
include_directories(include/camera)

include_directories(thirdparty)
include_directories(thirdparty/roar/core)
include_directories(thirdparty/CImg)

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

file(GLOB_RECURSE ASSET_IMPORTER_SOURCES
  simple_skeletal_animation.cpp
)

# CMake helper snippet
add_executable(${TARGET_NAME} ${ASSET_IMPORTER_SOURCES})

set(ALL_LIBRARIES ${OPENGL_LIBRARIES} ${GL_LIBRARIES})
target_link_libraries(${TARGET_NAME} glfw ${ALL_LIBRARIES})

# # Be slightly more pedantic
# target_compile_options(${TARGET_NAME} PRIVATE
#   -Wall
#   -Wextra
#   #	-Wmost
#   -Wconversion
#   -Wunreachable-code
#   -Wuninitialized
#   -Wno-gnu-zero-variadic-macro-arguments # Should fix this in long term, this is only needed for gtest typed tests at the moment
#   -pedantic-errors
#   -Wold-style-cast
#   -Wno-error=unused-variable
#   -Wshadow
#   -Wfloat-equal)
