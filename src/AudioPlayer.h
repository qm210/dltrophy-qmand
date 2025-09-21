//
// Created by qm210 on 20.09.2025.
//

#ifndef DLTROPHY_QMAND_AUDIOPLAYER_H
#define DLTROPHY_QMAND_AUDIOPLAYER_H

#include "miniaudio.h"
#include <string>
#include <optional>
#include "Config.h"


class AudioPlayer {

private:
    std::string lastGivenFilepath;
    std::string loadedAudioAbsolutePath;
    std::string fileError;
    std::string deviceError;
    ma_decoder decoder;
    ma_device_config deviceConfig;
    ma_device device;
    uint32_t bytesPerFrame = 0;
    uint64_t framesPlayed = 0;
    bool playing = false;
    bool looping = true;

    static void onAudioData(ma_device* pDevice, void* pOutput,
                            const void* pInput, ma_uint32 frameCount);

    void processBuffer(ma_device* pDevice, void* pOutput, ma_uint32 frameCount);

public:

    explicit AudioPlayer(const Config& config);
    ~AudioPlayer() { teardown(); }

    void teardown();
    void load(const std::string& filepath);
    void startPlayback(bool looping = true);
    void stopPlayback(bool rewind = true);

    [[nodiscard]]
    bool hasLoaded(const std::string& filepath) const {
        // whatever is checked here has to be cheap, it's checked once per UI frame! (no fs::equivalent() etc.)
        return !loadedAudioAbsolutePath.empty() && lastGivenFilepath == filepath;
    }

    [[nodiscard]]
    bool isPlaying() const { return playing; };

    [[nodiscard]]
    int samplerate() const { return decoder.outputSampleRate; }

    [[nodiscard]]
    float playingSeconds() const {
        return !playing
            ? 0
            : static_cast<float>(framesPlayed) / samplerate();
    }

    [[nodiscard]]
    const char* playingLabel() const {
        static char playSec[16];
        if (!playing) {
            strcpy(playSec, "--");
        } else {
            sprintf(playSec, "%.2f sec.", playingSeconds());
        }
        return playSec;
    }

    [[nodiscard]]
    const char* errorMessage() const {
        if (!fileError.empty()) {
            return fileError.c_str();
        }
        if (!deviceError.empty()) {
            return deviceError.c_str();
        }
        return "";
    }
};

#endif //DLTROPHY_QMAND_AUDIOPLAYER_H
