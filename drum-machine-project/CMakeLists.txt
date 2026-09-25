cmake_minimum_required(VERSION 3.24)

project(drum_machine
        VERSION 1.0
        DESCRIPTION "16-step drum machine in C++20 (SDL3 + Dear ImGui + PortAudio)"
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Third-party libraries
# CMake downloads them from their official repositories at the first build.
# The versions are pinned, so every build uses the same code.
# ---------------------------------------------------------------------------
include(FetchContent)

# SDL3: window, input, file dialog and WAV loading (built as a static library)
set(SDL_SHARED        OFF CACHE BOOL "" FORCE)
set(SDL_STATIC        ON  CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY  OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        release-3.2.28
        GIT_SHALLOW    TRUE)

# PortAudio: audio output (built as a static library)
# Its CMakeLists asks for a very old CMake version, which CMake 4 refuses.
# This variable tells CMake 4 to accept it.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(PA_BUILD_SHARED   OFF CACHE BOOL "" FORCE)
set(PA_BUILD_STATIC   ON  CACHE BOOL "" FORCE)
FetchContent_Declare(portaudio
        GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
        GIT_TAG        v19.7.0
        GIT_SHALLOW    TRUE)

# Dear ImGui: user interface (no CMake project, we compile its sources below)
FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.91.9b
        GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(SDL3 portaudio imgui)

add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3-static)

# ---------------------------------------------------------------------------
# Drum machine
# ---------------------------------------------------------------------------
add_executable(drum-machine
        src/main.cpp
        src/view/MainWindow.cpp
        src/audio/AudioFileReader.cpp
        src/audio/AudioPlayer.cpp
        src/audio/AudioEngine.cpp
        src/audio/AudioGenerator.cpp
        src/audio/Sequencer.cpp
        src/audio/Delay.cpp
        src/audio/Mixer.cpp
        src/shared/Preferences.cpp)

target_include_directories(drum-machine PRIVATE include)
target_link_libraries(drum-machine PRIVATE imgui SDL3::SDL3-static portaudio_static)

if (MSVC)
    target_compile_options(drum-machine PRIVATE /W4)
else ()
    target_compile_options(drum-machine PRIVATE -Wall -Wextra)
endif ()

# MinGW (for example the CLion toolchain on Windows): link the C++ runtime statically,
# so the .exe also starts outside of the IDE, without extra DLL files.
if (MINGW)
    target_link_options(drum-machine PRIVATE -static)
endif ()
