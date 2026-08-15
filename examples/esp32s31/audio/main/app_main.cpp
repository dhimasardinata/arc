#include <array>
#include <cstdint>
#include <cstdio>
#include <span>

#include "arc/board/esp32s31_korvo.hpp"
#include "arc/dma_chain.hpp"
#include "arc/soc.hpp"
#include "arc/soc/target.hpp"

#if __has_include("driver/gpio.h") && __has_include("driver/i2c_master.h") && __has_include("driver/i2s_std.h")
#include "arc/gpio.hpp"
#include "arc/i2c.hpp"
#include "arc/i2s.hpp"
#define ARC_S31_KORVO_AUDIO_DRIVER_CONTRACT 1
#else
#define ARC_S31_KORVO_AUDIO_DRIVER_CONTRACT 0
#endif

namespace {

using Board = arc::board::Korvo1;

#if ARC_S31_KORVO_AUDIO_DRIVER_CONTRACT
using AmpEnable = arc::Gpio<Board::Audio::pa>;
using CodecBus = arc::I2cBus<0, Board::I2c::sda, Board::I2c::scl>;
using AudioLink = arc::I2s<
    Board::Audio::sclk,
    Board::Audio::lrclk,
    Board::Audio::dsin,
    Board::Audio::sdout,
    48'000,
    I2S_DATA_BIT_WIDTH_16BIT,
    I2S_SLOT_MODE_STEREO,
    arc::I2sStd::philips,
    I2S_NUM_AUTO,
    6,
    240,
    Board::Audio::mclk>;

static_assert(AmpEnable::mask() == (std::uint32_t{1} << Board::Audio::pa));
static_assert(CodecBus::sda() == Board::I2c::sda);
static_assert(CodecBus::scl() == Board::I2c::scl);
static_assert(sizeof(Board::Resource::AudioBus) > 0U);
static_assert(sizeof(Board::Resource::AudioPa) > 0U);
static_assert(sizeof(Board::Resource::CodecI2c) > 0U);
static_assert(sizeof(CodecBus::Resource) > 0U);
static_assert(sizeof(AudioLink::Resource) > 0U);
static_assert(AudioLink::duplex());
static_assert(AudioLink::hz() == 48'000U);
#endif

}  // namespace

extern "C" void app_main()
{
    static_assert(arc::soc::s31, "audio requires ARC_TARGET=esp32s31");
    static_assert(arc::Soc::i2c && arc::Soc::i2s, "ESP32-S31 audio scaffold expects SDK I2C/I2S capabilities");
    static_assert(arc::Topology<Board>);
    static_assert(arc::board::Korvo1AudioGraph::valid());
    static_assert(arc::board::Korvo1CodecGraph::valid());
    static_assert(Board::Audio::mclk == 2 && Board::Audio::sdout == 6, "Korvo audio bus changed");
    static_assert(Board::Audio::pa == 7, "Korvo audio PA pin changed");
    static_assert(Board::AudioCodec::pa_count == 2U, "Korvo PA topology changed");
    static_assert(Board::AudioCodec::mic_count == 2U, "Korvo microphone topology changed");
    static_assert(Board::AudioCodec::speaker_count == 2U, "Korvo speaker topology changed");
    static_assert(Board::AudioCodec::speaker_ohm == 4U && Board::AudioCodec::speaker_w == 3U, "Korvo speaker rating changed");
    static_assert(Board::AudioCodec::pitch_mm == 2U && Board::AudioCodec::pitch_mil == 80U, "Korvo speaker connector pitch changed");
    static_assert(Board::AudioCodec::stereo && Board::AudioCodec::dual_adc && Board::AudioCodec::dual_dac, "Korvo codec capability changed");
    static_assert(Board::AudioCodec::analog_mics, "Korvo analog microphone routing changed");
    static_assert(Board::AudioCodec::speech && Board::AudioCodec::near_wake && Board::AudioCodec::far_wake, "Korvo speech wake topology changed");
    static_assert(Board::Setup::speaker_min == 1U, "Korvo minimum speaker setup changed");
    static_assert(Board::Setup::speaker_max == Board::AudioCodec::speaker_count, "Korvo speaker setup count changed");
    static_assert(Board::Power::audio_split, "Korvo audio power isolation changed");
    static_assert(Board::I2c::sda == 0 && Board::I2c::scl == 1, "Korvo codec control bus changed");

    std::array<std::uint8_t, 128> pcm{};
    arc::DmaChain<2> chain{};
    chain.bind(0, std::span<std::uint8_t>{pcm}.first(64U));
    chain.bind(1, std::span<std::uint8_t>{pcm}.last(64U), true);
    chain.link_circular();

    std::printf(
        "arc-s31-audio target=%s board=%s arch=%s codec=%s pa_model=%s pa_count=%u i2s=%d,%d,%d,%d,%d i2c=%d,%d pa=%d mics=%u analog_mics=%d speakers=%u speaker_pitch_mm=%u speaker_pitch_mil=%u speaker_setup=%u..%u speech=%d near_wake=%d far_wake=%d audio_split=%d audio_edges=%zu codec_edges=%zu driver_contract=%d dma_head=%p ready=%d\n",
        arc::soc::name,
        Board::name,
        arc::soc::arch,
        Board::AudioCodec::model,
        Board::AudioCodec::pa_model,
        static_cast<unsigned>(Board::AudioCodec::pa_count),
        Board::Audio::mclk,
        Board::Audio::sclk,
        Board::Audio::lrclk,
        Board::Audio::dsin,
        Board::Audio::sdout,
        Board::I2c::sda,
        Board::I2c::scl,
        Board::Audio::pa,
        static_cast<unsigned>(Board::AudioCodec::mic_count),
        Board::AudioCodec::analog_mics ? 1 : 0,
        static_cast<unsigned>(Board::AudioCodec::speaker_count),
        static_cast<unsigned>(Board::AudioCodec::pitch_mm),
        static_cast<unsigned>(Board::AudioCodec::pitch_mil),
        static_cast<unsigned>(Board::Setup::speaker_min),
        static_cast<unsigned>(Board::Setup::speaker_max),
        Board::AudioCodec::speech ? 1 : 0,
        Board::AudioCodec::near_wake ? 1 : 0,
        Board::AudioCodec::far_wake ? 1 : 0,
        Board::Power::audio_split ? 1 : 0,
        arc::board::Korvo1AudioGraph::edge_count,
        arc::board::Korvo1CodecGraph::edge_count,
        ARC_S31_KORVO_AUDIO_DRIVER_CONTRACT,
        static_cast<void*>(chain.head()),
        Board::pins::has<Board::Audio::pa>() ? 1 : 0);
}
