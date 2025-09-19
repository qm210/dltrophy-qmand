//
// Created by qm210 on 19.09.2025.
//

#ifndef DLTROPHY_QMAND_QMANDAPP_H
#define DLTROPHY_QMAND_QMANDAPP_H

#include <GLFW/glfw3.h>
#include "Config.h"
#include "geometry.h"


class QmandApp {
public:

    explicit QmandApp(Config config);
    ~QmandApp() = default;

    void run();

private:
    Config config;
    GLFWwindow* window;
    void initializeWindow();
    static void handleWindowError(int error, const char* description);
    Size size;

};

#endif //DLTROPHY_QMAND_QMANDAPP_H
