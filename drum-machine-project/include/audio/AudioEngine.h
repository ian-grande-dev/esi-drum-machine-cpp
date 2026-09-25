#ifndef DRUM_MACHINE_AUDIOENGINE_H
#define DRUM_MACHINE_AUDIOENGINE_H

#include <array>
#include <memory>
#include <mutex>
#include <string>

#include "audio/AudioGenerator.h"
#include "audio/AudioPlayer.h"
#include "audio/Sequencer.h"
#include "shared/SharedParams.h"

// Owns all the audio parts: one player per track, the sequencer and the audio generator.
// The UI talks to the audio only through this class and SharedParams.
class AudioEngine {
    SharedParams&                       m_sharedParams;
    AudioGenerator                      m_audioGenerator;
    Sequencer                           m_sequencer;

    std::array<AudioPlayer, NUM_TRACKS> m_audioPlayers;
    std::array<std::string, NUM_TRACKS> m_loadedFiles;   // path of the sample loaded in each player
    std::mutex                          m_loadMutex;     // only one thread loads samples at a time

    std::unique_ptr<AudioCallBackData>  m_callbackData;

public:
    explicit AudioEngine(SharedParams& sharedParams);
    ~AudioEngine();

    // The audio callback keeps pointers to the members, so we cannot copy or move the engine.
    AudioEngine(const AudioEngine&)            = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    // Loads the WAV file of each track if it changed since the last call.
    // You can call it from any thread (for example the file dialog thread).
    void loadPendingSamples();

    // Opens the audio stream. Returns false if there is an error.
    bool start();
    void stop();
};

#endif // DRUM_MACHINE_AUDIOENGINE_H
