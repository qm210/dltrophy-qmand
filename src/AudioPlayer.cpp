//
// Created by qm210 on 20.09.2025.
//

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "AudioPlayer.h"

#include <iostream>
#include <string>
#include <format>
#include <filesystem>
namespace fs = std::filesystem;

inline static bool exists(const std::string& path) {
    fs::path p(path);
    return fs::exists(p) && fs::is_regular_file(p);
}

AudioPlayer::AudioPlayer(const Config& config)
{
    if (exists(config.audioFile)) {
        load(config.audioFile);
    }
}

void AudioPlayer::teardown()
{
    if (playing) {
        stopPlayback();
    }
    ma_device_uninit(&device);
    ma_decoder_uninit(&decoder);
}

void AudioPlayer::load(const std::string& filepath)
{
    fs::path currentPath = fs::current_path();
    if (fs::equivalent(fs::path(filepath), fs::path(loadedAudioAbsolutePath))) {
        return;
    }
    fileError = "";
    std::string absolutePath = (fs::absolute(filepath)).string();
    ma_result result = ma_decoder_init_file(absolutePath.c_str(), nullptr, &decoder);
    if (result != MA_SUCCESS) {
        fileError = std::format("Could not load {} (result: {})",
                                filepath, static_cast<int>(result));
        ma_decoder_uninit(&decoder);
        return;
    }
    lastGivenFilepath = filepath;
    loadedAudioAbsolutePath = absolutePath;
};

void AudioPlayer::startPlayback(bool shouldLoop)
{
    if (playing) {
        stopPlayback();
    }
    looping = shouldLoop;
    ma_data_source_set_looping(&decoder, looping ? MA_TRUE : MA_FALSE);
    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = decoder.outputFormat;
    deviceConfig.playback.channels = decoder.outputChannels;
    deviceConfig.sampleRate = decoder.outputSampleRate;
    deviceConfig.dataCallback = onAudioData;
    deviceConfig.pUserData = this;
    deviceError = "";
    bytesPerFrame = ma_get_bytes_per_frame(deviceConfig.playback.format,
                                           deviceConfig.playback.channels);
    ma_result result = ma_device_init(NULL, &deviceConfig, &device);
    if (result != MA_SUCCESS) {
        deviceError = "Failed to open device :(";
        ma_decoder_uninit(&decoder);
        return;
    }
    result = ma_device_start(&device);
    if (result != MA_SUCCESS) {
        deviceError = "Failed to play :'(";
        teardown();
        return;
    }
    framesPlayed = 0;
    playing = true;
}

void AudioPlayer::stopPlayback(bool rewind)
{
    if (!playing) {
        return;
    }
    ma_device_stop(&device);
    if (rewind) {
        ma_decoder_seek_to_pcm_frame(&decoder, 0);
    }
    playing = false;
}

void AudioPlayer::onAudioData(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    auto* self = reinterpret_cast<AudioPlayer*>(pDevice->pUserData);
    if (!self) {
        throw std::runtime_error(
                "static onAudioData() must get an instance of AudioPlayer as pUserData! "
                "Have you learned nothing?"
        );
    }
    self->processBuffer(pDevice, pOutput, frameCount);
}

void AudioPlayer::processBuffer(ma_device* pDevice, void* pOutput, ma_uint32 frameCount)
{
    ma_uint64 framesRead;
    ma_result result = ma_decoder_read_pcm_frames(&decoder, pOutput, frameCount, &framesRead);
    if (result != MA_SUCCESS) {
        deviceError = std::format("Error Reading Frames. Sorry for so little details, just result: {}",
                                  static_cast<int>(result));
        teardown();
        return;
    }
    framesPlayed += framesRead;
}
