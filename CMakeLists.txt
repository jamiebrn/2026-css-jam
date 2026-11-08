cmake_minimum_required(VERSION 3.5)
project(game LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

include(FetchContent)

set(RAYLIB_VERSION 6.2)

FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    FIND_PACKAGE_ARGS
)

FetchContent_MakeAvailable(raylib)

include_directories(include/)
include_directories(extlib/)

file(GLOB_RECURSE SRC_FILES src/*.cpp)
file(GLOB_RECURSE EXT_SRC_FILES extlib/*.c extlib/*.cpp)
add_executable(game ${SRC_FILES} ${EXT_SRC_FILES})

if(EMSCRIPTEN)
  target_compile_options(game PRIVATE 
    -Oz
  )

  target_link_options(game PRIVATE 
    -sUSE_SDL=2
    -sASSERTIONS=1
    -sSINGLE_FILE
    -sSTACK_SIZE=4MB
    -sALLOW_MEMORY_GROWTH
    -sGROWABLE_ARRAYBUFFERS=0
    -sEXPORTED_FUNCTIONS=_main
    -Oz
  )

  target_link_options(${PROJECT_NAME} PUBLIC -sUSE_GLFW=3 PUBLIC --preload-file assets)

  add_custom_command(
      TARGET ${PROJECT_NAME} PRE_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/../assets
  )

else()
  add_custom_command(
      TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
  )
endif()

target_link_libraries(${PROJECT_NAME} raylib)

target_compile_features(game PRIVATE cxx_std_20)

if (EMSCRIPTEN)
  add_custom_command(TARGET game POST_BUILD
    COMMAND cp ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/game.js web/.
    COMMENT "Copying web build"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
  add_custom_command(TARGET game POST_BUILD
    COMMAND cp ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/game.data web/.
    COMMENT "Copying web build assets"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
endif()
