cmake_minimum_required(VERSION 3.22.1)
project("alfalah")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SOURCES
        ${CMAKE_SOURCE_DIR}/*.cpp
)

# ---- FreeType ----
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/freetype freetype_build)

# ---- HarfBuzz ----
set(HB_HAVE_FREETYPE ON CACHE BOOL "" FORCE)
set(HB_HAVE_CAIRO OFF CACHE BOOL "" FORCE)
set(HB_HAVE_GRAPHITE2 OFF CACHE BOOL "" FORCE)
set(HB_HAVE_GLIB OFF CACHE BOOL "" FORCE)
set(HB_HAVE_ICU OFF CACHE BOOL "" FORCE)
set(HB_BUILD_SUBSET OFF CACHE BOOL "" FORCE)
set(HB_BUILD_UTILS OFF CACHE BOOL "" FORCE)
set(HB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/harfbuzz harfbuzz_build)

list(FILTER SOURCES EXCLUDE REGEX "/third_party/harfbuzz/(test|docs)/")
list(FILTER SOURCES EXCLUDE REGEX "/third_party/freetype/(builds|docs|objs|src/tools)/")
# CMake's own freetype build dirs
list(FILTER SOURCES EXCLUDE REGEX "/freetype_build/")

add_library(${CMAKE_PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/third_party/freetype/include
        ${CMAKE_SOURCE_DIR}/third_party/harfbuzz/src)

target_link_libraries(${CMAKE_PROJECT_NAME}
        android
        log
        GLESv3
        freetype
        harfbuzz)