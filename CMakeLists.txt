cmake_minimum_required(VERSION 3.20)

project(NightRideSimulator LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# ---- Dependencies ----
# OpenGL
find_package(OpenGL REQUIRED)

# Assimp (via vcpkg)
find_package(assimp CONFIG REQUIRED)

# GLFW: prefer system install, fallback to FetchContent
include(FetchContent)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
  FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.9
  )
  FetchContent_MakeAvailable(glfw)
endif()

# ---- Target ----
add_executable(NightRideSimulator
  NightRideSimulator.cpp
  third_party/LearnOpenGL-master/src/glad.c
)

# Include directories
# - third_party for learnopengl/* and stb_image
# - third_party/glm for glm/*
# - LearnOpenGL glad headers
# - project root for shaders/textures relative paths

target_include_directories(NightRideSimulator
  PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/third_party
    ${PROJECT_SOURCE_DIR}/third_party/glm
    ${PROJECT_SOURCE_DIR}/third_party/LearnOpenGL-master/includes
)

# Link libraries
if(TARGET glfw)
  target_link_libraries(NightRideSimulator PRIVATE glfw OpenGL::GL assimp::assimp)
else()
  target_link_libraries(NightRideSimulator PRIVATE glfw3 OpenGL::GL assimp::assimp)
endif()

# MSVC runtime and warnings (optional)
if(MSVC)
  target_compile_options(NightRideSimulator PRIVATE /W4)
else()
  target_compile_options(NightRideSimulator PRIVATE -Wall -Wextra -Wpedantic)
endif()
