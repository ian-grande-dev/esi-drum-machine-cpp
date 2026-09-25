// Entry point of the drum machine.
//
// Two threads work together:
// - the main thread runs the UI (MainWindow);
// - PortAudio runs the audio callback in its own thread (AudioGenerator).
// They share their data through SharedParams, which is protected by a mutex.

#include "audio/AudioEngine.h"
#include "shared/Preferences.h"
#include "shared/SharedParams.h"
#include "view/MainWindow.h"
#include <SDL3/SDL.h>
#include <cstdlib>
#include <filesystem>
#include <string>

// Returns the folder of the executable. The preferences file is saved there.
static std::filesystem::path getExecutableDir() {
    // SDL gives the path in UTF-8. We use u8string so that paths with
    // special characters (é, ü, ...) also work on Windows.
    if (const char* basePath = SDL_GetBasePath()) {
        return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(basePath)));
    }
    return std::filesystem::current_path();
}

int main(int /*argc*/, char* /*argv*/[]) {
    SharedParams sharedParams;

    // Load the sample paths saved the last time.
    const std::filesystem::path exeDir = getExecutableDir();
    loadPreferences(sharedParams, exeDir);

    // Start the audio.
    AudioEngine audioEngine(sharedParams);
    audioEngine.loadPendingSamples();
    if (!audioEngine.start()) {
        return EXIT_FAILURE;
    }

    // Open the window. run() returns when the user closes it.
    MainWindow mainWindow(sharedParams, audioEngine);
    if (!mainWindow.init()) {
        audioEngine.stop();
        return EXIT_FAILURE;
    }
    mainWindow.run();

    audioEngine.stop();
    savePreferences(sharedParams, exeDir);
    return EXIT_SUCCESS;
}
