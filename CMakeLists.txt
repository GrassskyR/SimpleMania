cmake_minimum_required(VERSION 3.16)

project(SimpleMania LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
find_package(SDL2_mixer QUIET)

add_executable(simplemania
    src/main.cpp
    src/OsuParser.cpp
    src/Chart.cpp
    src/Game.cpp
    src/Renderer.cpp
)

target_include_directories(simplemania PRIVATE src)

if(TARGET SDL2::SDL2main)
    target_link_libraries(simplemania PRIVATE SDL2::SDL2main SDL2::SDL2)
else()
    target_link_libraries(simplemania PRIVATE SDL2::SDL2)
endif()

if(WIN32)
    target_link_libraries(simplemania PRIVATE winmm)
endif()

if(SDL2_mixer_FOUND)
    if(TARGET SDL2_mixer::SDL2_mixer)
        target_link_libraries(simplemania PRIVATE SDL2_mixer::SDL2_mixer)
    elseif(TARGET SDL2_mixer)
        target_link_libraries(simplemania PRIVATE SDL2_mixer)
    endif()
    target_compile_definitions(simplemania PRIVATE USE_SDL_MIXER=1)
endif()

add_executable(chartgen
    src/EditorCli.cpp
)

target_include_directories(chartgen PRIVATE src)
