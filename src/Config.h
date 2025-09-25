//
// Created by qm210 on 19.09.2025.
//

#ifndef DLTROPHY_QMAND_CONFIG_H
#define DLTROPHY_QMAND_CONFIG_H

#include <cstdint>

struct Config {
public:

    explicit Config(int argc, char* argv[]);

    std::string wledHost = "192.168.178.156";
    uint16_t wledPort = 21234;

    bool applyBrightness = true;
    uint8_t brightness = 130;
    bool applyFxIndex = false;
    uint8_t fxIndex = 210;
    bool applyFxSpeed = false;
    uint8_t fxSpeed = 110;

    std::string audioFile = "../track/synctrophy_v2.wav";
    bool audioLoop = true;
    bool autoplay = true;
    int autoplayDelayMs = 0;

};

#endif //DLTROPHY_QMAND_CONFIG_H
