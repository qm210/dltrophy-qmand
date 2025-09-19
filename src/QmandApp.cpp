//
// Created by qm210 on 19.09.2025.
//

#include "UI.h"
#include <string>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <map>
#include "QmandApp.h"


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
}

QmandApp::~QmandApp() {
    glfwTerminate();
    delete glfw;
    delete sender;
}

void QmandApp::initializeWindow() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwSetErrorCallback(handleWindowError);
    window = glfwCreateWindow(480, 180, "Trophy Qmander", NULL, NULL);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to open GLFW window");
    }
    glfwMakeContextCurrent(window);
    glfwGetWindowSize(window, &size.width, &size.height);
}

void QmandApp::run() {
    // cf. https://github.com/Immediate-Mode-UI/Nuklear/blob/master/demo/glfw_opengl3/nuklear_glfw_gl3.h
    struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(glfw, &atlas);
    nk_glfw3_font_stash_end(glfw);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        nk_glfw3_new_frame(glfw);
        auto rect = nk_rect(0, 0, size.width, size.height);

        if (nk_begin(ctx, "", rect, NK_WINDOW_BACKGROUND | NK_WINDOW_NO_SCROLLBAR)) {

            nk_layout_row_dynamic(ctx, size.partial_height(0.1), 2);
            nk_label(ctx, "WLED host (IP):", NK_TEXT_LEFT);
            nk_label(ctx, "UDP port (default 21234):", NK_TEXT_LEFT);

            nk_layout_row_dynamic(ctx, size.partial_height(0.15), 2);
            string_edit(ctx, config.wledHost, 32);
            uint_edit(ctx, config.wledPort);

            nk_layout_row_dynamic(ctx, size.partial_height(0.15), 1);
            byte_slider(ctx, "Brightness:", config.brightness);

//            nk_layout_row_dynamic(ctx, size.partial_height(0.15), 2);
//            nk_label(ctx, "FX ID:", NK_TEXT_LEFT);
//            uint_edit(ctx, config.fxIndex);

            nk_layout_row_dynamic(ctx, size.partial_height(0.12), 1);
            if (sender->isClosed()) {
                nk_label(ctx, sender->error_message(), NK_TEXT_CENTERED);
            } else {
                nk_label(ctx, "unusually quiet for a Friday night...", NK_TEXT_CENTERED);
            }

            nk_layout_row_dynamic(ctx, size.partial_height(0.20), 2);
            if (nk_button_label(ctx, "dark.")) {
                qmand(0);
            }
            if (nk_button_label(ctx, "QMAND!!")) {
                qmand();
            }
        }
        nk_end(ctx);

        glfwGetFramebufferSize(window, &size.width, &size.height);
        glViewport(0, 0, size.width, size.height);
        glClear(GL_COLOR_BUFFER_BIT);

        nk_glfw3_render(glfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);

        glfwSwapBuffers(window);
    }

    nk_glfw3_shutdown(glfw);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void QmandApp::handleWindowError(int error, const char* description) {
    std::cerr << "Error: " << description << " (" << error << ")" << std::endl;
}

enum Reason {
    InitialBoot = 0,
    DirectChange = 1,
    ButtonPress = 2,
    UpdateByOtherNotification = 3,
    NightlightActivated = 4,
    Other = 5,
    EffectChanged = 6,
    HueLightChanged = 7,
    PresetCycleActive = 8,
    UpdatedViaBlynk = 9,
};

const int MustBeZero = 0;

// This implements https://kno.wled.ge/interfaces/udp-notifier/
struct OfficialWledPacket {
    uint8_t purpose = MustBeZero;
    uint8_t reason = DirectChange;
    uint8_t brightness;
    uint8_t primaryColor[3];
    uint8_t nightlightActive;
    uint8_t nightlightDelayMins;
    uint8_t effectIndex;
    uint8_t effectSpeed;
    // from here Notifier Version 1
    uint8_t primaryWhiteValue;
    uint8_t version = 5;
    // from here Notifier Version 2
    uint8_t secondaryColor[3];
    uint8_t secondaryWhiteValue;
    // from here Notifier Version 3
    uint8_t effectIntensity;
    // from here Notifier Version 4
    uint8_t transitionDurationUpper;
    uint8_t transitionDurationLower;
    // from here Notifier Version 5
    uint8_t effectPalette;
    uint8_t _remainderZeroes[4];
};

struct CustomWledPacket {
    uint8_t purpose = MustBeZero;
    uint8_t reason = Other;
    uint8_t brightness;
    uint8_t resetTime;
    uint8_t _unusedG;
    uint8_t _unusedB;
    uint8_t _unusedNight;
    uint8_t _unusedNightMin;
    uint8_t effectIndex;
    uint8_t effectSpeed;
    uint8_t _unusedW;
    uint8_t version = 210;
};

std::map<int, std::string> byteDescriptionOfficial = {
        {0, "Purpose Byte"},
        {1, "Packet Reason"},
        {2, "Master Brightness"},
        {3, "Primary RED"},
        {4, "Primary GREEN"},
        {5, "Primary BLUE"},
        {6, "Nightlight running?"},
        {7, "Nightlight Delay in Minutes"},
        {8, "Effect Index"},
        {9, "Effect Speed"},
        {10, "Primary WHITE"},
        {11, "Version Byte"},
        {12, "Secondary RED"},
        {13, "Secondary GREEN"},
        {14, "Secondary BLUE"},
        {15, "Secondary WHITE"},
        {16, "Effect Intensity"},
        {17, "Transition Delay (Upper)"},
        {18, "Transition Delay (Lower)"},
        {19, "Effect Palette"},
};

// previously called sendPacket(), but why should it do so??
void QmandApp::qmand(std::optional<uint8_t> brightness) {
    // for now, we can just go with version 0
    // (basic support since WLED 0.3, even though the WLED has WLED 0.16)

    /*
    OfficialWledPacket qmd{
        .reason = DirectChange,
        .brightness = brightness.value_or(config.brightness),
//        .effectIndex = config.fxIndex,
    };
    std::cout << "[DEBUG] Updating Sender to " << config.wledHost << ":" << config.wledPort << std::endl;
    sender->update(config);
    std::cout << "[DEBUG] Trying to send message:" << std::endl;
    const char* msg = reinterpret_cast<const char*>(&qmd);
    for (int i = 0; i < sizeof(OfficialWledPacket); i++) {
        unsigned int byte = static_cast<unsigned int>(static_cast<unsigned char>(msg[i]));
        std::cout << "         " << std::setw(2) << i << ": ";
        auto it = byteDescriptionOfficial.find(i);
        auto name = it != byteDescriptionOfficial.end() ? it->second : "--unused--";
        std::cout << name << " = " << byte << std::endl;
    }
     */

    CustomWledPacket qmd{
        .purpose = MustBeZero,
        .reason = Other,
        .brightness = brightness.value_or(config.brightness),
        .resetTime = 1,
        .version = 210,
    };
    const char* msg = reinterpret_cast<const char*>(&qmd);
    try {
        int sent = sender->send(msg, sizeof(qmd));
        std::cout << "[DEBUG] Sent bytes: " << sent << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception during UDP Sending: " << e.what() << std::endl;
    }
}
