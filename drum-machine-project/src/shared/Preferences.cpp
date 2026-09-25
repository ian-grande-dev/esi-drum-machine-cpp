#include "shared/Preferences.h"
#include "shared/Constants.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

void loadPreferences(SharedParams& params, const std::filesystem::path& exeDir)
{
    const std::filesystem::path filePath = exeDir / PREFERENCES_FILENAME;

    // First start: there is no file yet.
    if (!std::filesystem::exists(filePath)) {
        return;
    }
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Cannot open " << filePath.string() << std::endl;
        return;
    }

    // One line per track: the path of its WAV file (the line can be empty).
    std::lock_guard<std::mutex> lock(params.m_mutex);
    for (int i = 0; i < NUM_TRACKS; ++i) {
        std::string line;
        if (!std::getline(file, line)) {
            break;
        }
        // Remove the '\r' of Windows line endings.
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        params.tracks[i].wavFile = line;
    }
}

void savePreferences(const SharedParams& params, const std::filesystem::path& exeDir)
{
    const std::filesystem::path filePath = exeDir / PREFERENCES_FILENAME;

    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Cannot write " << filePath.string() << std::endl;
        return;
    }

    // One line per track, in the same order as in loadPreferences().
    std::lock_guard<std::mutex> lock(params.m_mutex);
    for (int i = 0; i < NUM_TRACKS; ++i) {
        file << params.tracks[i].wavFile << '\n';
    }
}
