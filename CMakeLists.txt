cmake_minimum_required(VERSION 3.20)
project(MineCppClone VERSION 0.4.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
include(FetchContent)
FetchContent_Declare(raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(MineCppClone
  src/main.cpp
  src/i18n/Localization.cpp
  src/world/Block.cpp
  src/world/Chunk.cpp
  src/world/World.cpp
  src/render/TerrainViewer.cpp
)
target_include_directories(MineCppClone PRIVATE src)
target_link_libraries(MineCppClone PRIVATE raylib)
if(MSVC)
  target_compile_options(MineCppClone PRIVATE /W4 /permissive- /utf-8)
else()
  target_compile_options(MineCppClone PRIVATE -Wall -Wextra -Wpedantic)
endif()
enable_testing()
add_test(NAME terrain_smoke_test COMMAND MineCppClone --map --size 32 --seed 1122 --output terrain_test.ppm --lang en)
add_test(NAME localization_russian COMMAND MineCppClone --map --size 16 --seed 1122 --output terrain_ru.ppm --lang ru)
add_test(NAME localization_spanish COMMAND MineCppClone --map --size 16 --seed 1122 --output terrain_es.ppm --lang es)
add_test(NAME localization_german COMMAND MineCppClone --map --size 16 --seed 1122 --output terrain_de.ppm --lang de)
add_test(NAME localization_french COMMAND MineCppClone --map --size 16 --seed 1122 --output terrain_fr.ppm --lang fr)
add_test(NAME localization_chinese COMMAND MineCppClone --map --size 16 --seed 1122 --output terrain_zh.ppm --lang zh_CN)
add_test(NAME world_chunk_core COMMAND MineCppClone --self-test)
