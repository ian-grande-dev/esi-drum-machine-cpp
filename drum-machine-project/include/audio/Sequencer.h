#ifndef DRUM_MACHINE_SEQUENCER_H
#define DRUM_MACHINE_SEQUENCER_H

// Counts audio samples and says when the next step starts.
// Only the audio thread uses it, so it does not need a mutex.
class Sequencer {
    int  m_currentStep    { 0 };
    int  m_sampleCounter  { 0 };  // samples played since the start of the current step
    int  m_samplesPerStep { 0 };  // length of one step, in samples
    int  m_lastBpm        { 0 };  // we compute m_samplesPerStep again only when the BPM changes

    // True at the start, so the first call to advance() plays step 0 at once.
    bool m_firstTick      { true };

public:
    // Moves the sequencer forward by `frames` samples.
    // Returns true when a new step starts: the sounds of this step must play.
    bool advance(int frames, int sampleRate, int bpm);

    int  getCurrentStep() const;

    // Goes back to step 0 (used when the user presses Stop).
    void reset();
};

#endif // DRUM_MACHINE_SEQUENCER_H
