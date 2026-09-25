#include "audio/Mixer.h"
#include "audio/AudioGenerator.h"

void Mixer::updateTrackSettings(AudioCallBackData* data, int step, const Params& snapshot) {
    for (int track = 0; track < NUM_TRACKS; ++track) {
        const Track& settings = snapshot.tracks[track];
        AudioPlayer& player   = data->players[track];

        // Send the settings of the track to its player.
        player.setVolume(settings.muted ? 0.0f : settings.volume);
        player.setReversed(settings.reverse);
        player.setDelay(settings.delayEnabled, settings.delayTime, settings.delayMix);

        // Play the sample if this step is on and the track is not muted.
        if (settings.grid[step] && !settings.muted) {
            player.play();
        }
    }
    // Note: the current step is sent to the UI by AudioGenerator::snapshotParams().
}

void Mixer::mixPlayers(AudioCallBackData* data, float* out, unsigned long framesPerBuffer) {
    // Each player adds its sound to the output buffer.
    for (auto& player : data->players) {
        player.fillBuffer(out, framesPerBuffer);
    }
}
