#include "view/MainWindow.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"

// The UI is limited to 60 frames per second, so it does not use 100% of the CPU.
constexpr float FRAMERATE = 60.0f;
constexpr std::chrono::duration<double, std::milli> TARGET_FRAMETIME(1000.0 / FRAMERATE);

// Sliders: the user can also type a value (Ctrl + click).
// AlwaysClamp keeps this value between the min and the max of the slider.
constexpr ImGuiSliderFlags SLIDER_FLAGS = ImGuiSliderFlags_AlwaysClamp;

MainWindow::MainWindow(SharedParams& params, AudioEngine& engine):
    m_params(params),
    m_audioEngine(engine) {
    initTrackNames();
}

MainWindow::~MainWindow() {
    shutdown();
}

void MainWindow::initTrackNames() {
    std::lock_guard<std::mutex> lock(m_params.m_mutex);
    for (int i = 0; i < NUM_TRACKS; ++i)
        m_params.tracks[i].name = "Track " + std::to_string(i + 1);
}

bool MainWindow::init() {
    // Setup SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("Error: SDL_Init(): %s\n", SDL_GetError());
        return false;
    }
    m_sdlReady = true;

    // Create the window (hidden until it is ready) and its renderer.
    m_window = SDL_CreateWindow("Drum Machine", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_HIDDEN);
    if (m_window == nullptr) {
        SDL_Log("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return false;
    }
    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (m_renderer == nullptr) {
        SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
        return false;
    }
    SDL_SetRenderVSync(m_renderer, 1);
    SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(m_window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowRounding = 0.0f;

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(m_window, m_renderer);
    ImGui_ImplSDLRenderer3_Init(m_renderer);
    m_imguiReady = true;

    return true;
}

void MainWindow::shutdown() {
    // Free only what init() created. We can call this function many times.
    if (m_imguiReady) {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        m_imguiReady = false;
    }
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    if (m_sdlReady) {
        SDL_Quit();
        m_sdlReady = false;
    }
}

void MainWindow::run() {
    // init() failed or was not called: there is nothing to show.
    if (!m_imguiReady) {
        return;
    }

    const auto clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    bool done { false };
    while (!done) {
        auto frameStart = std::chrono::steady_clock::now();

        // Read all the events (keyboard, mouse, close button...).
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (SDL_EVENT_QUIT == event.type)
                done = true;
            if ((SDL_EVENT_WINDOW_CLOSE_REQUESTED == event.type)
                && (SDL_GetWindowID(m_window) == event.window.windowID))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Describe all the UI of this frame
        draw();

        // Rendering
        ImGui::Render();
        SDL_SetRenderDrawColorFloat(m_renderer,
                                    clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        SDL_RenderClear(m_renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_renderer);
        SDL_RenderPresent(m_renderer);

        // If the frame was fast, wait a little to keep 60 frames per second.
        auto frameDuration = std::chrono::steady_clock::now() - frameStart;
        if (frameDuration < TARGET_FRAMETIME) {
            std::this_thread::sleep_for(TARGET_FRAMETIME - frameDuration);
        }
    }
}

void MainWindow::onFileSelected(void* userdata, const char* const* files, int /*filter*/) {
    // We own the context now: unique_ptr deletes it when the function ends.
    std::unique_ptr<FileDialogContext> ctx(static_cast<FileDialogContext*>(userdata));

    // files == nullptr: error. files[0] == nullptr: the user cancelled.
    if (files == nullptr || files[0] == nullptr)
        return;

    // This callback can run in another thread, so we lock the mutex.
    {
        std::lock_guard<std::mutex> lock(ctx->window->m_params.m_mutex);
        ctx->window->m_params.tracks[ctx->track].wavFile = std::string(files[0]);
    }

    // Read the new file now (this is thread-safe).
    ctx->window->m_audioEngine.loadPendingSamples();
}

void MainWindow::draw() {

    // Copy the shared params into a local copy.
    // The UI works on this copy, so the mutex is locked only for a short time.
    {
        std::lock_guard<std::mutex> lock(m_params.m_mutex);
        m_localParams = static_cast<const Params&>(m_params);
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Drum Machine", nullptr,
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoTitleBar);

    drawPlayControls();
    ImGui::Spacing();

    // Grid (70% of the width) + track controls (the rest)
    float availWidth = ImGui::GetContentRegionAvail().x;
    float gridWidth  = availWidth * 0.70f;
    float trackWidth = availWidth - gridWidth - ImGui::GetStyle().ItemSpacing.x;

    drawGrid(gridWidth);

    // Same line: the track controls are on the right of the grid
    ImGui::SameLine();

    drawTrackControls(trackWidth);

    ImGui::End();

    // Write the changes of the user back into the shared params.
    // We do not write `running` and `currentStep`: only the audio side changes them.
    {
        std::lock_guard<std::mutex> lock(m_params.m_mutex);
        m_params.isPlaying     = m_localParams.isPlaying;
        m_params.bpm           = m_localParams.bpm;
        m_params.selectedTrack = m_localParams.selectedTrack;

        for (int i = 0; i < NUM_TRACKS; ++i) {
            // Keep wavFile: only onFileSelected() changes it, maybe in another thread
            // while we were drawing this frame. We must not replace it with an old value.
            std::string wavFile = std::move(m_params.tracks[i].wavFile);
            m_params.tracks[i]  = m_localParams.tracks[i];
            m_params.tracks[i].wavFile = std::move(wavFile);
        }
    }
}

void MainWindow::drawPlayControls() {
    // --- Container: Play ---
    ImGui::BeginChild("Play", ImVec2(0, 45), true);

    // Play / Stop button
    bool& isPlaying = m_localParams.isPlaying;
    if (isPlaying)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); // playing - red
    else
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f)); // stopped - green

    if (ImGui::Button(isPlaying ? "Stop" : "Play", ImVec2(60, 28))) {
        isPlaying = !isPlaying;
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();

    // BPM decrease button
    int& bpm = m_localParams.bpm;
    if (ImGui::Button("-##bpm", ImVec2(24, 28)) && bpm > MIN_BPM) {
        bpm -= BPM_STEP;
    }
    ImGui::SameLine();

    // BPM label
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5); // vertical align
    ImGui::Text("BPM:");
    ImGui::SameLine();

    // BPM value display
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5); // vertical align
    ImGui::Text("%3d", bpm);
    ImGui::SameLine();

    // BPM increase button
    if (ImGui::Button("+##bpm", ImVec2(24, 28)) && bpm < MAX_BPM) {
        bpm += BPM_STEP;
    }

    // Safety: the BPM always stays in the allowed range.
    bpm = std::clamp(bpm, MIN_BPM, MAX_BPM);
    ImGui::EndChild();
}

void MainWindow::drawGrid(float gridWidth) {
    // --- Container: Grid (Left) ---
    ImGui::BeginChild("Grid", ImVec2(gridWidth, 200), true);

    float gridInner      = ImGui::GetContentRegionAvail().x;
    float style_spacing  = ImGui::GetStyle().ItemSpacing.x;

    constexpr float MUTE_BTN_W  = 28.0f;  // mute button width
    constexpr float TRACK_BTN_W = 80.0f;  // track name button width (fixed)
    constexpr float SLIDER_W    = 60.0f;  // volume slider width (fixed)
    constexpr float BTN_SIZE    = 28.0f;  // step button height (square)
    constexpr float ROW_SPACING = 4.0f;   // spacing between rows

    // Controls width = slider + gap + mute button
    float controlsWidth = SLIDER_W + style_spacing + MUTE_BTN_W;

    // Sequencer width = space left after the track buttons and the controls
    float namesWidth = TRACK_BTN_W;
    float seqWidth   = gridInner
                       - namesWidth    - style_spacing
                       - controlsWidth - style_spacing
                       - 8.0f;         // inner border padding

    // Step button width: sequencer width divided by the number of steps (minus the spacing)
    float stepBtnW = (seqWidth - (NUM_STEPS - 1) * style_spacing) / NUM_STEPS;
    stepBtnW       = std::max(stepBtnW, 8.0f); // minimum width if the window is small

    // --- Sub-container 1: Track buttons ---
    drawTrackButtons(namesWidth, BTN_SIZE, ROW_SPACING);
    ImGui::SameLine();

    // --- Sub-container 2: Sequencer step buttons ---
    drawSequencer(seqWidth, stepBtnW, BTN_SIZE, ROW_SPACING);
    ImGui::SameLine();

    // --- Sub-container 3: Volume slider + Mute button ---
    drawVolumeControls(controlsWidth, SLIDER_W, MUTE_BTN_W, BTN_SIZE, ROW_SPACING);

    ImGui::EndChild(); // Container: Grid
}

void MainWindow::drawTrackButtons(float namesWidth, float btnSize, float rowSpacing) {
    int& selectedTrack = m_localParams.selectedTrack;

    ImGui::BeginChild("Tracks", ImVec2(namesWidth, 0), false);

    for (int track = 0; track < NUM_TRACKS; ++track) {
        // "##trkN" gives a unique ImGui ID without changing the text on the button.
        const std::string& name = m_localParams.tracks[track].name;
        const std::string trackId = name + "##trk" + std::to_string(track);

        // Highlight the selected track button
        if (selectedTrack == track)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.9f, 1.0f)); // selected - blue
        else
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f)); // default - dark grey

        if (ImGui::Button(trackId.c_str(), ImVec2(-1, btnSize)))
            selectedTrack = track; // select on click

        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, rowSpacing));
    }
    ImGui::EndChild();
}

void MainWindow::drawSequencer(float seqWidth, float stepBtnW, float btnSize, float rowSpacing) {
    const int playhead = m_localParams.currentStep;
    const bool showPlayhead = m_localParams.isPlaying;

    ImGui::BeginChild("Sequencer", ImVec2(seqWidth, 0), false);

    // One row per track, one button per step. Click a button to turn the step on or off.
    for (int track = 0; track < NUM_TRACKS; ++track) {
        for (int step = 0; step < NUM_STEPS; ++step) {
            const std::string btnId = "##" + std::to_string(track) + "_" + std::to_string(step);
            bool& active = m_localParams.tracks[track].grid[step];
            bool  isHead = showPlayhead && (step == playhead);

            ImVec4 color;
            if (isHead && active) {
                color = {0.95f, 0.80f, 0.00f, 1.f}; // yellow - playhead on an active step
            }
            else if (isHead && !active) {
                color = {0.50f, 0.42f, 0.00f, 1.f}; // dark yellow - playhead on an inactive step
            }
            else if (active) {
                color = {0.20f, 0.80f, 0.20f, 1.f}; // green - active
            }
            else {
                color = {0.30f, 0.30f, 0.30f, 1.f}; // grey - inactive
            }

            ImGui::PushStyleColor(ImGuiCol_Button, color);
            if (ImGui::Button(btnId.c_str(), ImVec2(stepBtnW, btnSize))) {
                active = !active;
            }
            ImGui::PopStyleColor();

            if (step < NUM_STEPS - 1) {
                ImGui::SameLine();
            }
        }
        ImGui::Dummy(ImVec2(0, rowSpacing));
    }
    ImGui::EndChild();
}

void MainWindow::drawVolumeControls(float controlsWidth, float sliderW, float muteBtnW, float btnSize, float rowSpacing) {
    ImGui::BeginChild("VolumeControls", ImVec2(controlsWidth, 0), false);
    for (int track = 0; track < NUM_TRACKS; ++track) {

        // Volume slider
        const std::string volId = "##vol" + std::to_string(track);
        float& volume = m_localParams.tracks[track].volume;
        ImGui::SetNextItemWidth(sliderW);
        ImGui::SliderFloat(volId.c_str(), &volume, 0.0f, 1.0f, "%.1f", SLIDER_FLAGS);
        ImGui::SameLine();

        // Mute button
        const std::string muteId = "M##" + std::to_string(track);
        bool& muted = m_localParams.tracks[track].muted;

        if (muted)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); // muted - red
        else
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f)); // active - grey

        if (ImGui::Button(muteId.c_str(), ImVec2(muteBtnW, btnSize))) {
            muted = !muted;
        }
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, rowSpacing));
    }
    ImGui::EndChild(); // Sub-container: VolumeControls
}

void MainWindow::drawTrackControls(float trackWidth) {
    // --- Container: Controls (Right) ---
    ImGui::BeginChild("TrackControls", ImVec2(trackWidth, 200), true);
    const int selectedTrack = m_localParams.selectedTrack;
    Track& t = m_localParams.tracks[selectedTrack];

    // --- Track title ---
    ImGui::Text("Track %d", selectedTrack + 1);
    ImGui::Separator();

    // --- WAV file name (without the folder) ---
    std::string displayName = t.wavFile.empty()
        ? "No file loaded"
        : std::filesystem::path(t.wavFile).filename().string();
    ImGui::Text("File: %s", displayName.c_str());

    // --- Load sample button: opens the system file dialog ---
    const std::string loadId = "Load sample##" + std::to_string(selectedTrack);
    if (ImGui::Button(loadId.c_str())) {
        // SDL gives the context back to onFileSelected(), which deletes it.
        auto ctx = std::make_unique<FileDialogContext>(this, selectedTrack);
        SDL_DialogFileFilter filter { "WAV files", "wav" };
        SDL_ShowOpenFileDialog(onFileSelected, ctx.release(),
            m_window, &filter, 1, nullptr, false);
    }

    // --- Reverse checkbox ---
    const std::string revId = "Reverse##" + std::to_string(selectedTrack);
    ImGui::Checkbox(revId.c_str(), &t.reverse);

    // --- Delay enable checkbox ---
    const std::string delayId = "Delay##" + std::to_string(selectedTrack);
    ImGui::Checkbox(delayId.c_str(), &t.delayEnabled);

    // --- Delay time slider (seconds) ---
    const std::string delayTimeId = "Delay time##" + std::to_string(selectedTrack);
    ImGui::SliderFloat(delayTimeId.c_str(), &t.delayTime, 0.0f, 1.0f, "%.3f", SLIDER_FLAGS);

    // --- Delay mix slider ---
    const std::string delayMixId = "Delay mix##" + std::to_string(selectedTrack);
    ImGui::SliderFloat(delayMixId.c_str(), &t.delayMix, 0.0f, 1.0f, "%.3f", SLIDER_FLAGS);

    ImGui::EndChild();
}
