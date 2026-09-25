#include "audio/AudioEngine.h"
#include "audio/AudioFileReader.h"
#include "shared/Constants.h"
#include <utility>
#include <vector>

AudioEngine::AudioEngine(SharedParams& sharedParams):
    m_sharedParams(sharedParams) {
}

AudioEngine::~AudioEngine() {
    stop();
}

void AudioEngine::loadPendingSamples() {
    // Only one thread at a time can load samples.
    std::lock_guard<std::mutex> loadLock(m_loadMutex);

    // Copy the file paths. Then we do not keep the shared mutex while we read the files.
    std::array<std::string, NUM_TRACKS> files;
    {
        std::lock_guard<std::mutex> lock(m_sharedParams.m_mutex);
        for (int i = 0; i < NUM_TRACKS; ++i) {
            files[i] = m_sharedParams.tracks[i].wavFile;
        }
    }

    for (int i = 0; i < NUM_TRACKS; ++i) {
        // Nothing to do if there is no file, or if this file is already loaded.
        if (files[i].empty() || files[i] == m_loadedFiles[i]) {
            continue;
        }

        // Reading a file is slow, so we do it without the shared mutex.
        std::vector<float> buffer;
        const bool loaded = AudioFileReader::loadFile(files[i], SAMPLE_RATE, buffer);

        std::lock_guard<std::mutex> lock(m_sharedParams.m_mutex);
        if (loaded) {
            // The audio thread will take this buffer at its next callback.
            m_audioPlayers[i].setPendingBuffer(std::move(buffer));
            m_loadedFiles[i] = files[i];
        }
        // Loading failed: show the previous file again in the UI
        // (only if the user did not choose another file in the meantime).
        else if (m_sharedParams.tracks[i].wavFile == files[i]) {
            m_sharedParams.tracks[i].wavFile = m_loadedFiles[i];
        }
    }
}

bool AudioEngine::start() {
    // Data given to the audio callback: the players, the sequencer and the shared params.
    m_callbackData = std::make_unique<AudioCallBackData>(
        AudioCallBackData{ m_audioPlayers, m_sequencer, m_sharedParams }
    );

    {
        std::lock_guard<std::mutex> lock(m_sharedParams.m_mutex);
        m_sharedParams.running = true;
    }

    // Open and start the audio stream.
    if (!m_audioGenerator.init(*m_callbackData)) {
        std::lock_guard<std::mutex> lock(m_sharedParams.m_mutex);
        m_sharedParams.running = false;
        return false;
    }
    return true;
}

void AudioEngine::stop() {
    {
        std::lock_guard<std::mutex> lock(m_sharedParams.m_mutex);
        m_sharedParams.running = false;
    }
    m_audioGenerator.stop();
}
