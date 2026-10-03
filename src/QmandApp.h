//
// Created by qm210 on 19.09.2025.
//

#ifndef DLTROPHY_QMAND_QMANDAPP_H
#define DLTROPHY_QMAND_QMANDAPP_H

#include <GLFW/glfw3.h>
#include <optional>
#include "Config.h"
#include "geometry.h"
#include "UdpSender.h"
#include "AudioPlayer.h"
#include "TimingStuff.h"

struct nk_glfw;
struct nk_context;

class QmandApp {
public:

    explicit QmandApp(Config config);
    ~QmandApp();

    void run();
    void qmand(bool beVisible = true, bool doReset = true);
    int send(DeadlineWledPacket qmd);

private:
    Config config;
    GLFWwindow* window;
    const int initWidth = 500;
    const int initHeight = 400;
    nk_glfw *glfw;
    nk_context *ctx;
    void initializeWindow();
    static void handleWindowError(int error, const char* description);
    Size size;

    UdpSender* sender;
    AudioPlayer* audio;
    TimingStuff* timer;
    std::chrono::milliseconds latencyCorrection{0};

    void prepareQmands();
    DeadlineWledPacket preparedQmand;
    DeadlineWledPacket preparedQmandNoReset;
    DeadlineWledPacket preparedDark;
};

#endif //DLTROPHY_QMAND_QMANDAPP_H
