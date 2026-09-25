#ifndef DRUM_MACHINE_AUDIOGENERATOR_H
#define DRUM_MACHINE_AUDIOGENERATOR_H

#include <array>

#include "portaudio.h"
#include "audio/AudioPlayer.h"
#include "audio/Mixer.h"
#include "audio/Sequencer.h"
#include "shared/SharedParams.h"

class AudioGenerator;

// Data given to the PortAudio callback.
// The callback is a static function, so it takes everything it needs from this struct.
struct AudioCallBackData {
    std::array<AudioPlayer, NUM_TRACKS>&    players;
    Sequencer&                              sequencer;
    SharedParams&                           sharedParams;
    Params                                  lastParams {};       // last copy of the shared params
    AudioGenerator*                         self       { nullptr };
};

// Opens the PortAudio output stream and computes each audio buffer.
class AudioGenerator {
    PaStream*   m_stream        { nullptr };
    bool        m_paInitialized { false };
    Mixer       m_mixer;

public:
    // Starts PortAudio and the audio stream. Returns false if there is an error.
    // `data` must stay alive until stop() is called.
    bool init(AudioCallBackData& data);

    // Stops the stream and closes PortAudio. You can call it many times.
    void stop();

private:
    // PortAudio calls this function in a real-time thread, each time it needs new samples.
    // Rules for this thread: never wait for a lock, and avoid slow work
    // (memory allocation, files, console output).
    static int audioCallback(
        const void *inputBuffer,
        void *outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData
    );

    // Tries to copy the shared params. If the UI thread has the mutex,
    // we do not wait: we keep the last copy.
    void snapshotParams(
        AudioCallBackData* data
    );

    // Moves the sequencer forward and writes the sound in `out`.
    void processSequencer(
        AudioCallBackData* data,
        unsigned long framesPerBuffer,
        float* out,
        const Params& snapshot
    );
};

#endif // DRUM_MACHINE_AUDIOGENERATOR_H
