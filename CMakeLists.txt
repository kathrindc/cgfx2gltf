cmake_minimum_required(VERSION 3.5)

project(cgfx2gltf)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(
  ${PROJECT_NAME}
  src/main.c
  src/cgfx.c
  src/utilities.c
  src/cgfx/data.c
  src/cgfx/dict.c
  src/cgfx/pica/etc1.c
  src/cgfx/pica/texture.c
  src/cgfx/pica/shader.c
  libs/cgltf.c
  libs/kgflags.c
  libs/stb_image.c
  libs/stb_image_write.c
)

target_include_directories(
  ${PROJECT_NAME} 
  PRIVATE src libs
)

if(MSVC)
  add_compile_definitions(_AMD64_=1)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
  target_link_libraries(${PROJECT_NAME} m)
endif()

