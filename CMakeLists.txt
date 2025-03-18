cmake_minimum_required(VERSION 3.16)
project(SlayTheSpire)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Source files
file(GLOB ASSET_FILES "assets/Simple Tower Defense/Enemies/*.png"
                      "assets/Simple Tower Defense/Environment/*.png"
                      "assets/Simple Tower Defense/Towers/*.png")

set(SOURCES
  # Headers
  src/common.h
  # Source
  src/main.cpp
)

# Client Executable
add_executable(SlayTheSpire ${SOURCES})
target_link_libraries(SlayTheSpire PRIVATE SDL3::SDL3 SDL3_image::SDL3_image)

# Emscripten specific settings
if(EMSCRIPTEN)
  set(CMAKE_EXECUTABLE_SUFFIX ".html" CACHE INTERNAL "")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build-wasm/$<CONFIGURATION>")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build-wasm/$<CONFIGURATION>")
  set_target_properties(SlayTheSpire PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build-wasm")

  # Copy asset files to build-wasm directory
  add_custom_command(TARGET SlayTheSpire POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets/use" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets"
  )
else()
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build-client/$<CONFIGURATION>")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build-client/$<CONFIGURATION>")
  set_target_properties(SlayTheSpire PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build-client")

  # Copy asset files to build-client directory
  add_custom_command(TARGET SlayTheSpire POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets"
  )
endif()

# Copy compile_commands.json to root directory if different
add_custom_command(TARGET SlayTheSpire POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_SOURCE_DIR}/compile_commands.json
)

add_subdirectory(SDL EXCLUDE_FROM_ALL)
add_subdirectory(SDL_image EXCLUDE_FROM_ALL)