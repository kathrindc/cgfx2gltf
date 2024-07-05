cmake_minimum_required(VERSION 3.5)

project(cgfx2gltf)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_executable(${PROJECT_NAME} main.c)
target_link_libraries(${PROJECT_NAME} m)

