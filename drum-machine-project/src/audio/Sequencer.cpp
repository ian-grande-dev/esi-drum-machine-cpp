#include "audio/Sequencer.h"
#include "shared/Constants.h"

bool Sequencer::advance(int frames, int sampleRate, int bpm) {
    // A BPM of 0 or less makes no sense (and would divide by zero).
    if (bpm <= 0) {
        return false;
    }

    // Compute the length of one step again only when the tempo changes.
    // One beat lasts 60 / bpm seconds, and one beat has STEPS_PER_BEAT steps.
    if (bpm != m_lastBpm) {
        m_samplesPerStep = static_cast<int>((60.0 / bpm) * sampleRate / STEPS_PER_BEAT);
        m_lastBpm = bpm;
    }

    // First call after a reset: play step 0 at once.
    if (m_firstTick) {
        m_firstTick = false;
        return true;
    }

    m_sampleCounter += frames;

    // Enough samples for one step: go to the next step (and back to 0 after the last one).
    if (m_sampleCounter >= m_samplesPerStep) {
        m_sampleCounter -= m_samplesPerStep;
        m_currentStep = (m_currentStep + 1) % NUM_STEPS;
        return true;
    }
    return false;
}

int Sequencer::getCurrentStep() const {
    return m_currentStep;
}

void Sequencer::reset() {
    m_currentStep = 0;
    m_sampleCounter = 0;
    m_firstTick = true;
}
