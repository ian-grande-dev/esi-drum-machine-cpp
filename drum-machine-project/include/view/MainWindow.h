#ifndef DRUM_MACHINE_MAINWINDOW_H
#define DRUM_MACHINE_MAINWINDOW_H

#include <SDL3/SDL.h>

#include "audio/AudioEngine.h"
#include "shared/SharedParams.h"

class MainWindow;

// Data given to the SDL file dialog callback: which window and which track.
struct FileDialogContext {
    MainWindow* window;
    int         track;
};

// The main window of the app (SDL3 + Dear ImGui). It runs in the main thread.
// Each frame, it copies SharedParams into m_localParams, draws the UI with this copy,
// then writes the changes back to SharedParams.
class MainWindow {
    SDL_Window*     m_window        { nullptr };
    SDL_Renderer*   m_renderer      { nullptr };
    bool            m_sdlReady      { false };
    bool            m_imguiReady    { false };

    SharedParams&   m_params;
    Params          m_localParams;

    AudioEngine&    m_audioEngine;

public:
    explicit MainWindow(SharedParams& params, AudioEngine& engine);
    ~MainWindow();

    MainWindow(const MainWindow&)            = delete;
    MainWindow& operator=(const MainWindow&) = delete;

    // Creates the SDL window and sets up ImGui. Returns false if there is an error.
    bool init();

    // Main loop: reads the events and draws the UI until the user closes the window.
    void run();

    // Called by SDL when the user chooses a file (or cancels).
    // SDL can call it from another thread.
    static void onFileSelected(
        void* userdata,
        const char* const* files,
        int filter
    );

private:
    void initTrackNames();
    void shutdown();
    void draw();
    void drawPlayControls();
    void drawGrid(float gridWidth);
    void drawTrackButtons(float namesWidth, float btnSize, float rowSpacing);
    void drawSequencer(float seqWidth, float stepBtnW, float btnSize, float rowSpacing);
    void drawVolumeControls(float controlsWidth, float sliderW, float muteBtnW, float btnSize, float rowSpacing);
    void drawTrackControls(float trackWidth);
};

#endif // DRUM_MACHINE_MAINWINDOW_H
