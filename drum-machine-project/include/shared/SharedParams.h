#ifndef DRUM_MACHINE_SHAREDPARAMS_H
#define DRUM_MACHINE_SHAREDPARAMS_H

#include "shared/Constants.h"
#include <array>
#include <mutex>
#include <string>

// All the settings of one track (one row of the grid).
struct Track {
    std::string                 name          {};         // name shown in the UI
    std::array<bool, NUM_STEPS> grid          {};         // true = the sound plays on this step
    float                       volume        { 0.8f };   // 0.0 (silent) to 1.0 (full volume)
    bool                        muted         { false };
    std::string                 wavFile       {};         // path of the WAV sample
    bool                        reverse       { false };  // play the sample backwards
    bool                        delayEnabled  { false };
    float                       delayTime     { 0.0f };   // time between echoes, in seconds (0.0 to 1.0)
    float                       delayMix      { 0.0f };   // level of the echo (0.0 to 1.0)
};

// The state of the whole drum machine.
// It is a simple struct that we can copy, so each thread can work
// on its own copy (a "snapshot") without keeping the mutex locked.
struct Params {
    bool running       { false };  // true while the audio stream is open
    bool isPlaying     { false };  // true when the user pressed Play
    int  bpm           { 120 };    // tempo, in beats per minute
    int  selectedTrack { 0 };      // track shown in the right panel of the UI
    int  currentStep   { 0 };      // step played now (written by the audio thread)
    std::array<Track, NUM_TRACKS> tracks {};
};

// Params shared between the UI thread and the audio thread.
// Always lock m_mutex before you read or write a field.
// To copy only the data (without the mutex), write:
//     Params copy = static_cast<const Params&>(shared);
struct SharedParams : Params {
    mutable std::mutex m_mutex;
};

#endif // DRUM_MACHINE_SHAREDPARAMS_H
