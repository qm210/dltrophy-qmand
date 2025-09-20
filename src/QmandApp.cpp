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

            nk_layout_row_dynamic(ctx, size.partial_height(0.01), 1);

            nk_layout_row_dynamic(ctx, size.partial_height(0.1), 2);
            nk_label(ctx, "WLED host (IP):", NK_TEXT_LEFT);
            nk_label(ctx, "UDP port (default 21234):", NK_TEXT_LEFT);

            nk_layout_row_dynamic(ctx, size.partial_height(0.15), 2);
            string_edit(ctx, config.wledHost, 32);
            uint_edit(ctx, config.wledPort);

            nk_layout_row_dynamic(ctx, size.partial_height(0.15), 1);
            byte_slider(ctx, "Brightness:", config.brightness);

            nk_layout_row_dynamic(ctx, size.partial_height(0.05), 2);

//            nk_label(ctx, "FX ID:", NK_TEXT_LEFT);
//            uint_edit(ctx, config.fxIndex);

            nk_layout_row_dynamic(ctx, size.partial_height(0.12), 1);
            if (sender->isClosed()) {
                nk_label(ctx, sender->status(), NK_TEXT_CENTERED);
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

        sender->process();
    }

    nk_glfw3_shutdown(glfw);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void QmandApp::handleWindowError(int error, const char* description) {
    std::cerr << "Error: " << description << " (" << error << ")" << std::endl;
}

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

    DeadlineWledPacket qmd{
        .purpose = MustBeZero,
        .reason = Other,
        .brightness = brightness.value_or(config.brightness),
        .resetTime = 1,
        .version = 210,
    };
    try {
        sender->update(config);
        int sent = sender->send(qmd);
        std::cout << "[DEBUG] Sent " << sent << " bytes to " << sender->host << ":" << sender->port << std::endl;
        if (sent == 0) {
            std::cout << "[DEBUG] -- UDP Sender Queue Size: " << sender->queueSize() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] when sending UDP: " << e.what() << std::endl;
    }
}
