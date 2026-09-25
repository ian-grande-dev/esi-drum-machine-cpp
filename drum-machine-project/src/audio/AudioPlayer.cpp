#include "audio/AudioPlayer.h"
#include "shared/Constants.h"
#include <algorithm>
#include <utility>

// writeSample() writes left and right channels only.
static_assert(NUM_CHANNELS == 2, "AudioPlayer only supports stereo output");

AudioPlayer::AudioPlayer() {
    // Allocate the delay buffer now, and not later in the audio thread.
    m_delay.init(SAMPLE_RATE);
}

void AudioPlayer::setPendingBuffer(std::vector<float> buffer) {
    // The old pending buffer (if any) is freed here, in the caller thread,
    // and not in the audio thread.
    m_pendingBuffer = std::move(buffer);
    m_hasPending    = true;
}

void AudioPlayer::applyPendingBuffer() {
    if (!m_hasPending) {
        return;
    }
    // Swap: the new sample goes to m_buffer, the old one waits in m_pendingBuffer.
    // The next call to setPendingBuffer() will free it.
    m_buffer.swap(m_pendingBuffer);
    m_hasPending = false;

    // Start again from a clean state with the new sample.
    m_playing  = false;
    m_position = 0;
    m_delay.clear();
}

void AudioPlayer::play() {
    if (m_buffer.empty()) {
        return;
    }
    m_playing = true;
    m_position = m_reversed ? m_buffer.size() - 1 : 0;
}

void AudioPlayer::stop() {
    m_playing = false;
    m_position = 0;

    // Clear the delay buffer, so no echo continues after the stop.
    m_delay.clear();
}

void AudioPlayer::setVolume(float volume) {
    m_volume = std::clamp(volume, 0.0f, 1.0f);
}

void AudioPlayer::setReversed(bool reversed) {
    m_reversed = reversed;
}

void AudioPlayer::setDelay(bool enabled, float delayTime, float delayMix) {
    m_delay.set(enabled, delayTime, delayMix);
}

void AudioPlayer::fillBuffer(float* out, size_t frames) {
    // Nothing to play and no echo: nothing to add.
    if (!m_playing && !m_delay.isEnabled()) {
        return;
    }

    if (m_buffer.empty()) {
        return;
    }

    for (size_t i = 0; i < frames; ++i) {
        float sample = 0.0f;

        // If the player is playing, read the current sample and apply the volume.
        if (m_playing) {
            sample = m_buffer[m_position] * m_volume;
            advancePosition();
        }

        // If the delay is on, add the echo. The echo continues after the end of the sample.
        if (m_delay.isEnabled()) {
            sample = m_delay.apply(sample);
        }

        writeSample(out, i, sample);
    }
}

void AudioPlayer::writeSample(float* out, size_t i, float sample) {
    // Add the sample to the left and right channels (interleaved: L R L R ...).
    out[i * NUM_CHANNELS]     += sample;  // left
    out[i * NUM_CHANNELS + 1] += sample;  // right
}

void AudioPlayer::advancePosition() {
    // Reversed: go from the end to the start of the sample.
    if (m_reversed) {
        if (m_position == 0) {
            m_playing = false;
            return;
        }
        --m_position;
    }
    // Normal: go from the start to the end of the sample.
    else {
        ++m_position;
        if (m_position >= m_buffer.size()) {
            m_playing = false;
        }
    }
}
