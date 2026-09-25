#ifndef DRUM_MACHINE_AUDIOPLAYER_H
#define DRUM_MACHINE_AUDIOPLAYER_H

#include <cstddef>
#include <vector>

#include "audio/Delay.h"

// Plays the WAV sample (mono) of one track.
// The audio thread calls all the methods, except setPendingBuffer().
class AudioPlayer {

    std::vector<float>  m_buffer        {};         // sample used now by the audio thread
    std::vector<float>  m_pendingBuffer {};         // new sample, waiting to replace m_buffer
    bool                m_hasPending    { false };

    size_t              m_position      { 0 };      // index of the next sample to play
    bool                m_playing       { false };
    float               m_volume        { 1 };
    bool                m_reversed      { false };

    Delay               m_delay;

public:
    AudioPlayer();

    // Gives a new sample to the player. The player does not use it at once:
    // the audio thread takes it later with applyPendingBuffer().
    // Call it only when SharedParams::m_mutex is locked.
    void setPendingBuffer(std::vector<float> buffer);

    // Audio thread: replaces the current sample with the pending one (if there is one).
    // It only swaps two vectors, so it does not allocate or free memory.
    // Call it only when SharedParams::m_mutex is locked.
    void applyPendingBuffer();

    // Starts the sample from the beginning (or from the end if it is reversed).
    void play();
    void stop();

    void setVolume(float volume);      // kept between 0.0 and 1.0
    void setReversed(bool reversed);
    void setDelay(bool enabled, float delayTime, float delayMix);

    // Adds the sound of this player to `out` (stereo, `frames` frames).
    void fillBuffer(float* out, size_t frames);

private:
    void  writeSample(float* out, size_t i, float sample);
    void  advancePosition();
};

#endif // DRUM_MACHINE_AUDIOPLAYER_H
