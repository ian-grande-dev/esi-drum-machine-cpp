#include "audio/Delay.h"
#include <algorithm>

void Delay::init(int sampleRate) {
    // The buffer keeps 1 second of sound, so the longest delay is 1 second.
    m_sampleRate = sampleRate;
    m_delayBuffer.assign(sampleRate, 0.0f);
    m_delayWritePos = 0;
}

void Delay::clear() {
    // Remove the old sound, so no echo continues after a stop.
    std::fill(m_delayBuffer.begin(), m_delayBuffer.end(), 0.0f);
}

void Delay::set(bool enabled, float delayTime, float delayMix) {
    m_delayEnabled = enabled;
    // Keep the values in a safe range. A mix above 1.0 would make
    // the echo louder each time and the sound would explode.
    m_delayTime    = std::clamp(delayTime, 0.0f, 1.0f);
    m_delayMix     = std::clamp(delayMix,  0.0f, 1.0f);
}

bool Delay::isEnabled() const {
    return m_delayEnabled;
}

float Delay::apply(float sample) {
    // No buffer: init() was not called, so we cannot make an echo.
    if (m_delayBuffer.empty()) {
        return sample;
    }

    const int size = static_cast<int>(m_delayBuffer.size());

    // Delay time in samples, between 1 and the size of the buffer.
    int delaySamples = static_cast<int>(m_delayTime * static_cast<float>(m_sampleRate));
    delaySamples     = std::clamp(delaySamples, 1, size);

    // Read the sample written `delaySamples` samples ago.
    int readIndex = (m_delayWritePos - delaySamples + size) % size;

    // Mix the old (delayed) sample with the current sample.
    float delayedSample = m_delayBuffer[readIndex];
    sample = sample + m_delayMix * delayedSample;

    // Write the result in the buffer: it will make the next echo (feedback).
    m_delayBuffer[m_delayWritePos] = sample;
    m_delayWritePos = (m_delayWritePos + 1) % size;

    return sample;
}
