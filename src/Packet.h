//
// Created by qm210 on 20.09.2025.
//

#ifndef DLTROPHY_QMAND_PACKET_H
#define DLTROPHY_QMAND_PACKET_H

#include <string>
#include <map>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <variant>


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
    uint8_t fxIndex;
    uint8_t fxSpeed;
    // from here Notifier Version 1
    uint8_t primaryWhiteValue;
    uint8_t version = 5;
    // from here Notifier Version 2
    uint8_t secondaryColor[3];
    uint8_t secondaryWhiteValue;
    // from here Notifier Version 3
    uint8_t fxIntensity;
    // from here Notifier Version 4
    uint8_t transitionDurationUpper;
    uint8_t transitionDurationLower;
    // from here Notifier Version 5
    uint8_t fxPalette;
    uint8_t _remainderZeroes[4];
};

const std::map<int, std::string> byteDescriptionOfficial = {
        {0, "Purpose Byte"},
        {1, "Packet Reason"},
        {2, "Master Brightness"},
        {3, "Primary RED"},
        {4, "Primary GREEN"},
        {5, "Primary BLUE"},
        {6, "Nightlight running?"},
        {7, "Nightlight Delay in Minutes"},
        {8, "FX Index"},
        {9, "FX Speed"},
        {10, "Primary WHITE"},
        {11, "Version Byte"},
        {12, "Secondary RED"},
        {13, "Secondary GREEN"},
        {14, "Secondary BLUE"},
        {15, "Secondary WHITE"},
        {16, "FX Intensity"},
        {17, "Transition Delay (Upper)"},
        {18, "Transition Delay (Lower)"},
        {19, "FX Palette"},
};

struct DeadlineWledPacket {
    uint8_t purpose = MustBeZero;
    uint8_t reason = DirectChange;
    uint8_t doReset;
    uint8_t applyBrightness;
    uint8_t brightness;
    uint8_t applyFxIndex;
    uint8_t fxIndex;
    uint8_t applyFxSpeed;
    uint8_t fxSpeed;
    uint8_t applyAllWhite;
    uint8_t whiteValue;
    uint8_t version = 210;
    uint8_t subversion = 0;
};

const std::map<int, std::string> byteDescriptionDeadline = {
        {0, "Purpose Byte (must be zero)"},
        {1, "Packet Reason (1 = Direct Change, but ignored afaik)"},
        {2, "Master Brightness"},
        {3, "Reset Whole Strip? (if 1)"},
        {4, "Apply Brightness Value? (if 1)"},
        {5, "Apply FX Index? (if 1)"},
        {6, "Apply FX Speed? (if 1)"},
        {7, "Apply All-White? (if 1)"},
        {8, "FX Index"},
        {9, "FX Speed"},
        {10, "All LEDs to this (\"white\") value (for testing)"},
        {11, "Version Byte (must be 210)"},
        {12, "Deadline SubVersion Byte (just 0 for now)"},
};

using Packet = std::variant<OfficialWledPacket, DeadlineWledPacket>;

template<typename PacketType>
static const std::map<int, std::string>& getDescriptions();
template<>
const std::map<int, std::string>& getDescriptions<OfficialWledPacket>() {
    return byteDescriptionOfficial;
}
template<>
const std::map<int, std::string>& getDescriptions<DeadlineWledPacket>() {
    return byteDescriptionDeadline;
}

static std::string byteDescription(const Packet& packet, int key, const std::string& defaultValue) {
    return std::visit([key, &defaultValue](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        const auto& descriptions = getDescriptions<T>();
        auto it = descriptions.find(key);
        return it != descriptions.end() ? it->second : defaultValue;
    }, packet);
}

#endif //DLTROPHY_QMAND_PACKET_H
