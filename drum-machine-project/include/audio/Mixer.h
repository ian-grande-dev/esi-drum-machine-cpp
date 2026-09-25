#ifndef DRUM_MACHINE_MIXER_H
#define DRUM_MACHINE_MIXER_H

#include "shared/SharedParams.h"

struct AudioCallBackData;

// Sends the track settings to the players and mixes all players together.
// Only the audio thread uses it. It never locks the shared mutex:
// it works on the snapshot given by AudioGenerator.
class Mixer {
public:
    // Called when a new step starts: updates each player and plays
    // the tracks that are active on this step.
    void updateTrackSettings(
        AudioCallBackData* data,
        int step,
        const Params& snapshot
    );

    // Adds the sound of all players to the output buffer.
    void mixPlayers(
        AudioCallBackData* data,
        float* out,
        unsigned long framesPerBuffer
    );
};

#endif // DRUM_MACHINE_MIXER_H
