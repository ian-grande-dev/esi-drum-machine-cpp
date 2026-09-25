# Class diagrams

## 1. Overview

The main classes and how they are linked. `AudioEngine` owns all the audio parts. `MainWindow` (UI thread) and the audio callback (PortAudio thread) share data through `SharedParams`.

```mermaid
classDiagram
    direction LR
    class MainWindow {
        -m_localParams : Params
        +init() bool
        +run()
        +onFileSelected(userdata, files, filter)$
        -draw()
    }
    class AudioEngine {
        -m_loadedFiles : array~string, 4~
        -m_loadMutex : mutex
        +loadPendingSamples()
        +start() bool
        +stop()
    }
    class AudioGenerator {
        -m_stream : PaStream*
        +init(data) bool
        +stop()
        -audioCallback(...)$ int
        -snapshotParams(data)
        -processSequencer(data, frames, out, snapshot)
    }
    class Mixer {
        +updateTrackSettings(data, step, snapshot)
        +mixPlayers(data, out, frames)
    }
    class Sequencer {
        -m_currentStep : int
        -m_samplesPerStep : int
        +advance(frames, sampleRate, bpm) bool
        +getCurrentStep() int
        +reset()
    }
    class AudioPlayer {
        -m_buffer : vector~float~
        -m_pendingBuffer : vector~float~
        +setPendingBuffer(buffer)
        +applyPendingBuffer()
        +play()
        +stop()
        +fillBuffer(out, frames)
    }
    class Delay {
        -m_delayBuffer : vector~float~
        +init(sampleRate)
        +set(enabled, time, mix)
        +apply(sample) float
    }
    class AudioFileReader {
        +loadFile(path, sampleRate, out_buffer)$ bool
    }
    class AudioCallBackData
    class SharedParams

    MainWindow --> SharedParams : m_params
    MainWindow --> AudioEngine : m_audioEngine
    AudioEngine --> SharedParams
    AudioEngine *-- AudioGenerator
    AudioEngine *-- Sequencer
    AudioEngine *-- "4" AudioPlayer
    AudioEngine *-- AudioCallBackData
    AudioEngine ..> AudioFileReader : uses
    AudioGenerator *-- Mixer
    AudioGenerator ..> AudioCallBackData : uses
    AudioPlayer *-- Delay
```

## 2. Shared data

The data shared between the UI thread and the audio thread. Each thread works on its own copy of `Params` and locks `SharedParams::m_mutex` only to copy it.

```mermaid
classDiagram
    direction LR
    class Track {
        +name : string
        +grid : array~bool, 16~
        +volume : float
        +muted : bool
        +wavFile : string
        +reverse : bool
        +delayEnabled : bool
        +delayTime : float
        +delayMix : float
    }
    class Params {
        +running : bool
        +isPlaying : bool
        +bpm : int
        +selectedTrack : int
        +currentStep : int
        +tracks : array~Track, 4~
    }
    class SharedParams {
        +m_mutex : mutex
    }
    class AudioCallBackData {
        +players : array~AudioPlayer, 4~&
        +sequencer : Sequencer&
        +sharedParams : SharedParams&
        +lastParams : Params
        +self : AudioGenerator*
    }
    class MainWindow
    class AudioPlayer
    class Sequencer

    Params <|-- SharedParams
    Params *-- "4" Track
    AudioCallBackData --> SharedParams
    AudioCallBackData *-- Params : lastParams
    AudioCallBackData --> "4" AudioPlayer
    AudioCallBackData --> Sequencer
    MainWindow *-- Params : m_localParams
    MainWindow --> SharedParams : m_params
```
