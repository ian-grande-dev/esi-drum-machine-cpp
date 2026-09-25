#ifndef DRUM_MACHINE_AUDIOFILEREADER_H
#define DRUM_MACHINE_AUDIOFILEREADER_H

#include <string>
#include <vector>

// Reads WAV files with SDL.
class AudioFileReader {
public:
    // Loads a WAV file and converts it to mono float samples at `sampleRate`.
    // Returns false if the file cannot be read. In this case, out_buffer does not change.
    static bool loadFile(const std::string& path, int sampleRate, std::vector<float>& out_buffer);
};

#endif // DRUM_MACHINE_AUDIOFILEREADER_H
