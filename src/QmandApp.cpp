//
// Created by qm210 on 19.09.2025.
//

#include "UI.h"
#include <string>
#include <stdexcept>
#include <iostream>
#include <map>
#include "QmandApp.h"
#include "Packet.h"


QmandApp::QmandApp(Config config)
: config(config) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    initializeWindow();
    gladLoadGL(glfwGetProcAddress);

    glfw = new nk_glfw{0};
    ctx = nk_glfw3_init(glfw, window, NK_GLFW3_INSTALL_CALLBACKS);

    sender = new UdpSender(config);
    audio = new AudioPlayer(config);
    timer = new TimingStuff();
}

QmandApp::~QmandApp() {
    glfwTerminate();
    delete glfw;
    delete sender;
    delete audio;
    delete timer;
}

void QmandApp::initializeWindow() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwSetErrorCallback(handleWindowError);
    window = glfwCreateWindow(500, 360, "Trophy Qmander", NULL, NULL);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to open GLFW window");
    }
    glfwMakeContextCurrent(window);
    glfwGetWindowSize(window, &size.width, &size.height);
}

void QmandApp::run() {
    struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(glfw, &atlas);
    nk_glfw3_font_stash_end(glfw);
    auto unit = ctx->style.font->height / 2;
    ctx->style.window.padding = nk_vec2(unit, unit);
    ctx->style.slider.show_buttons = 1;
    auto nk_window_flags = NK_WINDOW_BACKGROUND | NK_WINDOW_NO_SCROLLBAR;
    char text[1024];

    timer->reset();

    while (!glfwWindowShouldClose(window)) {
        timer->process();
        sender->process();
        glfwPollEvents();

        nk_glfw3_new_frame(glfw);
        auto rect = nk_rect(0, 0, size.width, size.height);
        int rowHeight = 0;
        bool qmandChanged = false;
        bool hostChanged = false;

        if (nk_begin(ctx, "", rect, nk_window_flags)) {

            rowHeight = 3 * unit;
            nk_layout_row_dynamic(ctx, rowHeight, 2);
            nk_label(ctx, "WLED host (IP):", NK_TEXT_LEFT);
            nk_label(ctx, "UDP port (default 21234):", NK_TEXT_LEFT);

            rowHeight = 4 * unit;
            nk_layout_row_dynamic(ctx, rowHeight, 2);
            hostChanged |=
                    string_edit(ctx, config.wledHost, 32);
            hostChanged |=
                    int_edit(ctx, config.wledPort);

            rowHeight = 3 * unit;
            nk_layout_row_dynamic(ctx, rowHeight, 1);
            if (sender->isClosed()) {
                nk_label(ctx, sender->status(), NK_TEXT_CENTERED);
            } else {
                nk_label(ctx, "unusually quiet for a Friday night...", NK_TEXT_CENTERED);
            }

            rowHeight = 5 * unit;
            sprintf(text, "Brightness: %d", config.brightness);
            qmandChanged |=
                    parameterWithCheckboxRow(ctx, rowHeight, text,
                                             config.applyBrightness,
                                             config.brightness);
            sprintf(text, "FX Index: %d", config.fxIndex);
            qmandChanged |=
                    parameterWithCheckboxRow(ctx, rowHeight, text,
                                             config.applyFxIndex,
                                             config.fxIndex);
            sprintf(text, "FX Speed: %d", config.fxSpeed);
            qmandChanged |=
                    parameterWithCheckboxRow(ctx, rowHeight, text,
                                             config.applyFxSpeed,
                                             config.fxSpeed);

            rowHeight = 5 * unit;
            nk_layout_row_dynamic(ctx, rowHeight, 2);
            if (nk_button_label(ctx, "dark.")) {
                qmand(false);
            }
            if (nk_button_label(ctx, "!QMAND!")) {
                qmand(true);
            }

            rowHeight = unit;
            nk_layout_row_dynamic(ctx, rowHeight, 2);

            rowHeight = 2 * unit;
            nk_layout_row_dynamic(ctx, rowHeight, 1);
            nk_label(ctx, audio->errorMessage(), NK_TEXT_LEFT);

            rowHeight = 5 * unit;
            nk_layout_row_begin(ctx, NK_DYNAMIC, rowHeight, 3);
            nk_layout_row_push(ctx, 0.6f);
            string_edit(ctx, config.audioFile, 256);
            nk_layout_row_push(ctx, 0.15f);
            nk_label(ctx, audio->playingLabel(), NK_TEXT_CENTERED);
            nk_layout_row_push(ctx, 0.25f);
            if (audio->isPlaying()) {
                if (nk_button_label(ctx, "Stop Track")) {
                    audio->stopPlayback();
                }
            } else if (audio->hasLoaded(config.audioFile)) {
                if (nk_button_label(ctx, "Play Track")) {
                    audio->startPlayback(config.audioLoop);
                }
            } else {
                if (nk_button_label(ctx, "Load Track")) {
                    audio->load(config.audioFile);
                }
            }
            nk_layout_row_end(ctx);

            rowHeight = 5 * unit;
            nk_layout_row_begin(ctx, NK_DYNAMIC, rowHeight, 3);
            nk_layout_row_push(ctx, 0.5f);
            sprintf(text, "Autoplay %s", config.autoplay ? "on QMAND" : "off");
            checkbox(ctx, text, config.autoplay);
            if (config.autoplay) {
                if (nk_widget_is_hovered(ctx)) {
                    nk_tooltip(ctx, "Wait for playing after sending the qmand (to account for latency)");
                }
                nk_layout_row_push(ctx, 0.25f);
                nk_label(ctx, "with delay [ms] ", NK_TEXT_RIGHT);
                int_edit(ctx, config.autoplayDelayMs, NK_EDIT_READ_ONLY);
            }
        }
        nk_end(ctx);

        glfwGetFramebufferSize(window, &size.width, &size.height);
        glViewport(0, 0, size.width, size.height);
        glClear(GL_COLOR_BUFFER_BIT);

        nk_glfw3_render(glfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);

        glfwSwapBuffers(window);

        if (qmandChanged) {
            prepareQmands();
        }
        if (hostChanged) {
            timer->startMeasurement("updating");
            sender->update(config);
            auto timing = timer->finishMeasurement("updating");
            std::cout << "[bla] updating has cost us: " << timing << std::endl;
        }
    }

    nk_glfw3_shutdown(glfw);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void QmandApp::handleWindowError(int error, const char* description) {
    std::cerr << "Error: " << description << " (" << error << ")" << std::endl;
}

void QmandApp::qmand(bool theGoodOne) {
    int sent;
    try {
        sent = send(theGoodOne ? preparedQmand : preparedDark);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] when sending UDP: " << e.what() << std::endl;
    }
    if (config.autoplay) {
        if (!theGoodOne) {
            audio->stopPlayback();
            return;
        }
        timer->startMeasurement("delay");
        auto delay = std::chrono::milliseconds(config.autoplayDelayMs) - latencyCorrection;
        timer->executeIn(delay, [this]() {
            audio->startPlayback(config.audioLoop);
            auto delayMs = static_cast<int>(timer->finishMeasurement("delay"));
            latencyCorrection = std::chrono::milliseconds{delayMs - config.autoplayDelayMs};
            std::cout << "[INFO] Delayed Action Executed, delay wanted " << config.autoplayDelayMs
                      << ", took " << delayMs << " ms -> remember correction: " << latencyCorrection.count()
                      << std::endl;
            timer->printDebug = true;
        });
    }
    std::cout << "[DEBUG] Sent " << sent << " bytes to " << sender->host << ":" << sender->port << std::endl;
}

int QmandApp::send(DeadlineWledPacket qmd) {
    int sent = sender->send(qmd);
    if (sent == 0) {
        std::cerr << "[WARNING] -- UDP Sender did not send, Queue Size: " << sender->queueSize() << std::endl;
    }
    return sent;
}

void QmandApp::prepareQmands() {
    // cache these so sending takes less latency
    preparedQmand = {
        .purpose = MustBeZero,
        .reason = DirectChange,
        .doReset = 1,
        .applyBrightness = static_cast<uint8_t>(config.applyBrightness),
        .brightness = config.brightness,
        .applyFxIndex = static_cast<uint8_t>(config.applyFxIndex),
        .fxIndex = config.fxIndex,
        .applyFxSpeed = static_cast<uint8_t>(config.applyFxSpeed),
        .fxSpeed = config.fxSpeed,
        .version = 210,
        .subversion = 0,
    };
    // has default copy constructor
    preparedDark = preparedQmand;
    preparedDark.brightness = 0;
}