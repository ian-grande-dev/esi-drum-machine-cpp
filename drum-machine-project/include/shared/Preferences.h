#ifndef DRUM_MACHINE_PREFERENCES_H
#define DRUM_MACHINE_PREFERENCES_H

#include "shared/SharedParams.h"
#include <filesystem>

// Save and load the user preferences: the WAV file of each track.
// The file is in the folder of the executable, so the app finds it at the next start.

// Reads the preferences file and sets the wavFile of each track.
// If the file does not exist, nothing changes.
void loadPreferences(SharedParams& params, const std::filesystem::path& exeDir);

// Writes the wavFile of each track in the preferences file (one path per line).
void savePreferences(const SharedParams& params, const std::filesystem::path& exeDir);

#endif // DRUM_MACHINE_PREFERENCES_H
