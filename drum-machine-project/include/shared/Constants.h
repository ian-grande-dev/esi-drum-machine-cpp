#ifndef DRUM_MACHINE_CONSTANTS_H
#define DRUM_MACHINE_CONSTANTS_H

#include <string>

// Global settings of the drum machine.
// All values are known at compile time.

// --- Sequencer ---
constexpr int NUM_TRACKS        = 4;     // number of sound tracks (rows of the grid)
constexpr int NUM_STEPS         = 16;    // number of steps in one pattern (columns of the grid)
constexpr int STEPS_PER_BEAT    = 4;     // 4 steps = 1 beat (sixteenth notes)
constexpr int MIN_BPM           = 40;
constexpr int MAX_BPM           = 240;
constexpr int BPM_STEP          = 1;     // value added or removed by the +/- buttons

// --- Audio ---
constexpr int SAMPLE_RATE       = 44100; // samples per second
constexpr int FRAMES_PER_BUFFER = 256;   // frames written by the audio callback each time
constexpr int NUM_CHANNELS      = 2;     // stereo output (left + right)

// --- Window ---
constexpr int WINDOW_WIDTH      = 1080;
constexpr int WINDOW_HEIGHT     = 270;

// File where the app saves the WAV path of each track.
inline const std::string PREFERENCES_FILENAME = "preferences.txt";

#endif // DRUM_MACHINE_CONSTANTS_H
