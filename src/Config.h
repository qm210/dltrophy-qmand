//
// Created by qm210 on 19.09.2025.
//

#ifndef DLTROPHY_QMAND_CONFIG_H
#define DLTROPHY_QMAND_CONFIG_H

#include <cstdint>

struct Config {
public:
    explicit Config(int argc, char* argv[]);

    std::string wledHost = "qm.local";
    uint16_t wledPort = 21234;

    uint8_t brightness = 130;
    uint8_t fxIndex = 210;

};

#endif //DLTROPHY_QMAND_CONFIG_H
