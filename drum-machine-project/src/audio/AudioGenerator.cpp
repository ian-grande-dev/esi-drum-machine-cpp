#include "audio/AudioGenerator.h"
#include "shared/Constants.h"
#include <algorithm>
#include <iostream>
#include <mutex>

bool AudioGenerator::init(AudioCallBackData& data) {
    data.self = this;

    PaError error = Pa_Initialize();
    if (error != paNoError) {
        std::cerr << "PortAudio error in Pa_Initialize(): " << Pa_GetErrorText(error) << std::endl;
        return false;
    }
    m_paInitialized = true;

    // Output only (0 input channels), stereo, 32-bit float samples.
    error = Pa_OpenDefaultStream(
        &m_stream,
        0,
        NUM_CHANNELS,
        paFloat32,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        audioCallback,
        &data
    );
    if (error != paNoError) {
        std::cerr << "PortAudio error in Pa_OpenDefaultStream(): " << Pa_GetErrorText(error) << std::endl;
        m_stream = nullptr;
        stop();
        return false;
    }

    error = Pa_StartStream(m_stream);
    if (error != paNoError) {
        std::cerr << "PortAudio error in Pa_StartStream(): " << Pa_GetErrorText(error) << std::endl;
        stop();
        return false;
    }
    return true;
}

void AudioGenerator::stop() {
    if (m_stream) {
        Pa_StopStream(m_stream);
        Pa_CloseStream(m_stream);
        m_stream = nullptr;
    }
    // Close PortAudio only if Pa_Initialize() worked.
    if (m_paInitialized) {
        Pa_Terminate();
        m_paInitialized = false;
    }
}

int AudioGenerator::audioCallback(
    const void* /*inputBuffer*/,
    void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* /*timeInfo*/,
    PaStreamCallbackFlags /*statusFlags*/,
    void* userData)
{
    auto*  data = static_cast<AudioCallBackData*>(userData);
    float* out  = static_cast<float*>(outputBuffer);

    // Start with silence.
    std::fill(out, out + framesPerBuffer * NUM_CHANNELS, 0.0f);

    // Get a fresh copy of the params (or keep the last one if the mutex is busy).
    data->self->snapshotParams(data);
    const Params& snapshot = data->lastParams;

    // Not running (yet, or anymore): send silence.
    // We do not return paComplete here: that would stop the stream for good.
    // AudioGenerator::stop() stops the stream.
    if (!snapshot.running) {
        return paContinue;
    }

    // Move the sequencer forward and mix the players into the output buffer.
    data->self->processSequencer(data, framesPerBuffer, out, snapshot);

    return paContinue;
}

void AudioGenerator::snapshotParams(AudioCallBackData* data) {
    // try_to_lock: the audio thread must never wait for the UI thread.
    // If the mutex is busy, we keep the last snapshot for this buffer.
    std::unique_lock<std::mutex> lock(data->sharedParams.m_mutex, std::try_to_lock);
    if (!lock.owns_lock()) {
        return;
    }

    data->lastParams = static_cast<const Params&>(data->sharedParams);

    // Tell the UI which step is playing now.
    data->sharedParams.currentStep = data->sequencer.getCurrentStep();

    // Take the new samples loaded by the UI (only a swap, no allocation).
    for (auto& player : data->players) {
        player.applyPendingBuffer();
    }
}

void AudioGenerator::processSequencer(AudioCallBackData* data, unsigned long framesPerBuffer, float* out, const Params& snapshot) {
    // Playing: when a new step starts, update the players and play the active tracks.
    // Then mix all players into the output buffer.
    if (snapshot.isPlaying) {
        bool newStep = data->sequencer.advance(
            static_cast<int>(framesPerBuffer), SAMPLE_RATE, snapshot.bpm);

        if (newStep) {
            int step = data->sequencer.getCurrentStep();
            m_mixer.updateTrackSettings(data, step, snapshot);
        }

        m_mixer.mixPlayers(data, out, framesPerBuffer);
    }
    // Stopped: go back to step 0 and stop all players, so there is no sound.
    else {
        data->sequencer.reset();
        for (auto& player : data->players) {
            player.stop();
        }
    }
}
