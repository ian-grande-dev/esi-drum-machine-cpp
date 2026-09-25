#ifndef DRUM_MACHINE_DELAY_H
#define DRUM_MACHINE_DELAY_H

#include <vector>

// A simple echo effect.
// It keeps the last second of sound in a circular buffer
// and adds this old sound to the new sound.
class Delay {
    std::vector<float>  m_delayBuffer   {};   // circular buffer (1 second of audio)
    int                 m_delayWritePos { 0 };
    int                 m_sampleRate    { 0 };
    bool                m_delayEnabled  { false };
    float               m_delayTime     { 0 };  // in seconds
    float               m_delayMix      { 0 };  // 0.0 to 1.0

public:
    // Creates the buffer. Call it once, before apply().
    // It allocates memory, so do not call it in the audio callback.
    void  init(int sampleRate);

    // Removes all old echoes (sets the buffer to silence).
    void  clear();

    // Changes the settings. Time and mix are kept between 0.0 and 1.0.
    void  set(bool enabled, float delayTime, float delayMix);

    // Adds the echo to one sample and returns the result.
    float apply(float sample);

    bool  isEnabled() const;
};

#endif // DRUM_MACHINE_DELAY_H
