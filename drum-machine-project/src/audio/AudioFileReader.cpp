#include "audio/AudioFileReader.h"
#include <SDL3/SDL.h>
#include <iostream>

/*
 * Reads a WAV file and stores its audio samples in out_buffer.
 * Parameters:
 * - path       : path of the WAV file
 * - sampleRate : sample rate of the audio engine (the file is converted to this rate)
 * - out_buffer : (output) the samples, mono, 32-bit float
 * Returns false if the file cannot be loaded or converted.
 */
bool AudioFileReader::loadFile(const std::string& path, int sampleRate, std::vector<float>& out_buffer) {
    SDL_AudioSpec spec {};
    Uint8* rawBuf = nullptr;
    Uint32 rawLen = 0;

    if (!SDL_LoadWAV(path.c_str(), &spec, &rawBuf, &rawLen)) {
        std::cerr << "Failed to load WAV file '" << path << "': " << SDL_GetError() << std::endl;
        return false;
    }

    // Target format: float samples, 1 channel, at the engine sample rate.
    // AudioPlayer reads one sample per frame, so the sound must be mono.
    // For a stereo file, SDL mixes the left and right channels together.
    SDL_AudioSpec dstSpec {};
    dstSpec.format   = SDL_AUDIO_F32;
    dstSpec.channels = 1;
    dstSpec.freq     = sampleRate;

    Uint8* floatBuf = nullptr;
    int floatLen = 0;

    const bool converted = SDL_ConvertAudioSamples(
        &spec, rawBuf, static_cast<int>(rawLen), &dstSpec, &floatBuf, &floatLen);
    SDL_free(rawBuf);

    if (!converted) {
        std::cerr << "Failed to convert audio samples of '" << path << "': " << SDL_GetError() << std::endl;
        return false;
    }

    // Copy the converted samples into the vector, then free the SDL buffer.
    const auto* floatData = reinterpret_cast<const float*>(floatBuf);
    const int sampleCount = floatLen / static_cast<int>(sizeof(float));
    out_buffer.assign(floatData, floatData + sampleCount);

    SDL_free(floatBuf);
    return true;
}
