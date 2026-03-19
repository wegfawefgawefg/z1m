cmake_minimum_required(VERSION 3.28)

project(z1m VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glm CONFIG REQUIRED)

find_package(SDL3 CONFIG QUIET)
if (NOT TARGET SDL3::SDL3 AND NOT TARGET SDL3::SDL3-shared AND NOT TARGET SDL3::SDL3-static)
    include(FetchContent)
    FetchContent_Declare(
        SDL3
        URL https://github.com/libsdl-org/SDL/releases/download/release-3.2.22/SDL3-3.2.22.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SDL3)
endif()

add_executable(z1m
    src/app/main.cpp
    src/app/run.cpp
    src/content/opening_content.cpp
    src/content/overworld_warps.cpp
    src/content/sandbox_content.cpp
    src/content/world_data.cpp
    src/debug/debug_ui.cpp
    src/render/camera.cpp
    src/render/debug_tileset.cpp
    src/render/scene_renderer.cpp
    src/game/game_session.cpp
    src/game/player.cpp
    src/game/world.cpp
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_sdl3.cpp
    third_party/imgui/backends/imgui_impl_sdlrenderer3.cpp
)

if (NOT MSVC)
    set_source_files_properties(
        third_party/imgui/imgui.cpp
        third_party/imgui/imgui_draw.cpp
        third_party/imgui/imgui_tables.cpp
        third_party/imgui/imgui_widgets.cpp
        third_party/imgui/backends/imgui_impl_sdl3.cpp
        third_party/imgui/backends/imgui_impl_sdlrenderer3.cpp
        PROPERTIES COMPILE_OPTIONS "-w"
    )
endif()

target_include_directories(z1m PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends
)

target_link_libraries(z1m PRIVATE glm::glm)

if (TARGET SDL3::SDL3)
    target_link_libraries(z1m PRIVATE SDL3::SDL3)
elseif (TARGET SDL3::SDL3-shared)
    target_link_libraries(z1m PRIVATE SDL3::SDL3-shared)
elseif (TARGET SDL3::SDL3-static)
    target_link_libraries(z1m PRIVATE SDL3::SDL3-static)
else()
    message(FATAL_ERROR "SDL3 target was not created.")
endif()

if (MSVC)
    target_compile_options(z1m PRIVATE /W4 /permissive-)
else()
    target_compile_options(z1m PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wsign-conversion
    )
endif()
