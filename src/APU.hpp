#pragma once

#include "common.hpp"
#include <SDL.h>

// I am no expert on the Game Boy's audio system, nor am I an expert on audio in general,
// so, as always, comments will be abundant for self-education and self-documentation.
// I will not pretend that I understand everything that is going on here, nor will I pretend
// that the explanatory comments are my own and are 100% accurate. I will do my best to
// credit those who are much smarter than I in this domain.
// The most helpful resources I have found are:
// - https://www.reddit.com/r/EmuDev/comments/5gkwi5/comment/dat3zni/?utm_source=share&utm_medium=web2x&context=3
// - https://nightshade256.github.io/2021/03/27/gb-sound-emulation.html
// - https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware
// I only mention the Pan Docs for completeness, but I found it extremely unfriendly to a beginner like myself.
// Edit: After reading the Pan Docs again with a slightly better understanding of sound theory, I found it to be
//       much more helpful than I originally thought. I still think it's unfriendly to beginners, but
//       it's a great resource after gaining even an elementary understanding of concepts like
//       audio channels, sweep, envelope, and length functions, waveforms, etc. Wikipedia is awesome for this.

class APU {
    friend class PulseChannel;
    friend class WaveChannel;
    friend class NoiseChannel;

public:
    APU();
    ~APU();

public:
    void    tick();
    void    init();
    uint8_t read(uint16_t addr) const;
    void    write(uint16_t addr, uint8_t data);

private:
    // Audio registers ========================================================
    // Pulse channel 1 --------------------------------------------------------
    uint8_t nr10 = 0x00; // Channel 1 Sweep register                   (0xFF10)
    uint8_t nr11 = 0x00; // Channel 1 Length timer and duty cycle      (0xFF11)
    uint8_t nr12 = 0x00; // Channel 1 Volume and envelope              (0xFF12)
    uint8_t nr13 = 0x00; // Channel 1 Frequency (period? timer?) lo    (0xFF13)
    uint8_t nr14 = 0x00; // Channel 1 Frequency (period? timer?) hi    (0xFF14)

    // Pulse channel 2 --------------------------------------------------------
    uint8_t nr21 = 0x00; // Channel 2 Length timer and duty cycle      (0xFF16)
    uint8_t nr22 = 0x00; // Channel 2 Volume and envelope              (0xFF17)
    uint8_t nr23 = 0x00; // Channel 2 Frequency lo                     (0xFF18)
    uint8_t nr24 = 0x00; // Channel 2 Frequency hi                     (0xFF19)

    // Wave channel -----------------------------------------------------------
    uint8_t nr30 = 0x00; // Channel 3 DAC enable (sound on/off)        (0xFF1A)
    uint8_t nr31 = 0x00; // Channel 3 Sound length                     (0xFF1B)
    uint8_t nr32 = 0x00; // Channel 3 Select output level              (0xFF1C)
    uint8_t nr33 = 0x00; // Channel 3 Frequency's lower data           (0xFF1D)
    uint8_t nr34 = 0x00; // Channel 3 Frequency's higher data          (0xFF1E)

    // Noise channel ----------------------------------------------------------
    uint8_t nr41 = 0x00; // Channel 4 Sound length                     (0xFF20)
    uint8_t nr42 = 0x00; // Channel 4 Volume Envelope                  (0xFF21)
    uint8_t nr43 = 0x00; // Channel 4 Frequency and randomness         (0xFF22)
    uint8_t nr44 = 0x00; // Channel 4 Control                          (0xFF23)

    // Control ----------------------------------------------------------------
    uint8_t nr50 = 0x00; // Master volume and VIN panning              (0xFF24)
    uint8_t nr51 = 0x00; // Sound panning                              (0xFF25)
    uint8_t nr52 = 0x00; // Audio master control: Sound on/off         (0xFF26)

    struct Control {
        // NR50 (0xFF24) ALLL BRRR  Vin L enable, Left vol, Vin R enable, Right vol
        uint8_t rightVolume    = 0x00;
        bool    vinRightEnable = false; // Unsupported
        uint8_t leftVolume     = 0x00;
        bool    vinLeftEnable  = false; // Unsupported

        // NR51 (0xFF25) NW21 NW21  Left enables, Right enables
        bool leftChannel1Enable = false;
        bool leftChannel2Enable = false;
        bool leftChannel3Enable = false;
        bool leftChannel4Enable = false;
        bool rightChannel1Enable = false;
        bool rightChannel2Enable = false;
        bool rightChannel3Enable = false;
        bool rightChannel4Enable = false;

        // NR52 (0xFF26) P--- NW21  Power control/status, Channel length statuses
        bool power = false;
        bool channel1LengthStatus = false;
        bool channel2LengthStatus = false;
        bool channel3LengthStatus = false;
        bool channel4LengthStatus = false;

        void update(uint8_t offset, uint8_t data);
    } control;

    void clearRegisters();

private:
    // Audio channels ==================================================================================================
    // Here are some simple (probably over-simplistic) definitions to help understand the following structs:
    // - A channel is a way to generate sound.
    // - A pulse channel is a channel that generates a square wave. The wave and noise channels are channels
    //   that generate wave and noise patterns, respectively.
    // - A sweep is a way to change the frequency of a channel over time (only available on channel 1).
    // - Length controls the duration of a channel (how long the channel plays before automatically stopping).
    // - Duty is a way to change the waveform of a channel over time, thus altering the character of a sound.
    // - The envelope is a way to change the volume of a channel over time.
    // - The frequency is the rate at which the waveform patterns repeats (how fast the channel generates sound,
    //   thus setting the pitch of the sound).
    // - The amplitude is the maximum absolute value of a signal.
    // - Periods are used to determine how often the sweep, length, and envelope are updated.
    // - Triggering a channel is a way to start a channel.
    // - The DAC (Digital-to-Analog Converter) is a way to convert digital audio signals to analog audio signals.
    // See the first table at https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware for the mappings
    // between the NRxy registers and channel members.
    struct PulseChannel {
        // NR10: -PPP NSSS  Sweep period, negate, shift (unique to channel 1)
        uint8_t sweepShift      = 0x00;  // How much the frequency changes
        bool    sweepNegate     = false; // Direction of frequency change (0=increase, 1=decrease)
        uint8_t sweepPeriod     = 0x00;  // Time interval between frequency changes

        bool    sweepEnable     = false; // If true, the sweep function is enabled
        int     shadowFrequency = 0;     // The frequency of the channel after the sweep function is applied
        int     sweepTimer      = 0;     // Decremented every CPU cycle; when it reaches 0, the sweep is updated

        // NR11 and NR21: DDLL LLLL  Duty, Length load (64-L)
        uint8_t lengthLoad  = 0x00; // Loads lengthTimer with (64 - lengthLoad) when written to
        uint8_t duty        = 0x00; // 0=12.5%, 1=25%, 2=50%, 3=75% (see waveDutyTable)
        int     lengthTimer = 0;    // Channel is disabled when it reaches 0 (perhaps 'duration' would be a better name)

        // NR12 and NR22: VVVV APPP  Starting volume, Envelope add mode, period
        uint8_t period          = 0x00;  // How often volume changes
        bool    envelopeAddMode = false; // Direction of volume change (0=decrease, 1=increase)
        uint8_t startVolume     = 0x00;  // Initial volume of the channel

        int     periodTimer     = 0;     // Decremented every CPU cycle; when it reaches 0, the envelope is updated
        int     currentVolume   = 0;     // The current volume of the channel, periodically adjusted by the envelope

        // NR13 and NR23: FFFF FFFF  Frequency LSB
        uint8_t frequencyLSB = 0x00; // lower 8 bits of the frequency

        // NR14 and NR24: TL-- -FFF  Trigger, Length enable, Frequency MSB
        uint8_t frequencyMSB = 0x00;  // Upper 3 bits of the frequency
        bool    lengthEnable = false; // If true, the length timer is enabled
        bool    triggered    = false; // If true, the channel is triggered (enabled)

        // This table represents different waveforms that can be produced by the channel.
        // The duty cycle affects the shape of the sound wave, altering the timbre (character)
        // of the sound.
                                                    // Duty   Waveform    Ratio
        const uint8_t waveDutyTable[4][8] = {       // -------------------------
            { 0, 0, 0, 0, 0, 0, 0, 1 },             // 0      00000001    12.5%
            { 1, 0, 0, 0, 0, 0, 0, 1 },             // 1      00000011    25%
            { 1, 0, 0, 0, 0, 1, 1, 1 },             // 2      00001111    50%
            { 0, 1, 1, 1, 1, 1, 1, 0 }              // 3      11111100    75%
        };

        // "The role of frequency timer is to step wave generation. Each T-cycle the frequency timer
        // is decremented by 1. As soon as it reaches 0, it is reloaded with a value calculated using the below
        // formula, and the wave duty position register is incremented by 1."
        // See Channel 2 section: https://nightshade256.github.io/2021/03/27/gb-sound-emulation.html
        int frequencyTimer = 0; // Reloaded with (2048 - frequency) * 4 every time it reaches 0

        // This is the index of the next sample to be played in the wave duty table when the frequency timer reaches 0.
        uint8_t waveDutyPosition = 0x00; // Wraps around 0-7

        // These control whether the channel produces sound. If either is false, the channel is silenced.
        // "A channel is turned on by triggering it (i.e. setting bit 7 of NRx4)3.
        // A channel is turned off when any of the following occurs:
        // - The channel’s length timer is enabled in NRx4 and expires, or
        // - For CH1 only: when the period sweep overflows, or
        // - The channel’s DAC is turned off. The envelope reaching a volume of 0 does NOT turn the channel off!"
        // See https://gbdev.io/pandocs/Audio_Registers.html#global-control-registers
        bool    enable        = false;
        bool    dacEnable     = false; // Controlled by upper 5 bits of NR12 and NR22
        uint8_t statusBitNR52 = 0x00;  // Bit 0 is the status bit for channel 1, bit 1 is the status bit for channel 2
        bool    negateModeUsed  = false;

        // The description of these functions are provided in their implementations.
        void  tick();
        void  sweepTick();
        void  sweepFreqCalculation(bool update);
        void  envelopeTick();
        void  lengthTick();
        void  trigger();
        void  update(uint8_t offset, uint8_t data);
        void  enableChannel();
        void  disableChannel();
        float getSample();

        APU* apu = nullptr;
    };

    PulseChannel pulseChannel1;
    PulseChannel pulseChannel2;

    struct WaveChannel {
        // NR30:  E--- ----  DAC power
        // These control whether the channel produces sound. If either is false, the channel is silenced.
        bool    enable        = false;
        bool    dacEnable     = false;  // Controlled by upper 5 bits of NR42
        uint8_t statusBitNR52 = 1 << 3; // Bit 2 is the status bit for channel 3

        // NR31: LLLL LLLL  Length load (256-L)
        uint8_t lengthLoad  = 0x00;
        int     lengthTimer = 0;

        // NR32: -VV- ----  Volume code (00=0%, 01=100%, 10=50%, 11=25%)
        // The Volume Shift Register (4-bit) is used to control the volume of the channel (note that this channel
        // has no envelope) by specifying the amount of right shift applied to the output samples of the channel.
        // Volume code -> Shift amount:
        // 0 -> 4,     1 -> 0,    2 -> 1,    3 -> 2,
        // i.e., volumeCode -> (volumeCode - 1), for volumeCode != 0 (silence)
        uint8_t volumeCode = 0x00;

        // NR33: FFFF FFFF  Frequency LSB
        uint16_t frequencyLSB = 0x00;

        // NR34: TL-- -FFF  Trigger, Length enable, Frequency MSB
        uint8_t frequencyMSB  = 0x00;
        bool    lengthEnable = false;
        bool    triggered     = false;

        // Wave Pattern RAM (32 4-bit custom samples) located at 0xFF30-0xFF3F,
        // which "is a 16 byte region, therefore there are two samples contained within one byte.
        // We play the upper four bits of a byte before the lower four."
        // See Channel 3 section: https://nightshade256.github.io/2021/03/27/gb-sound-emulation.html
        std::array<uint8_t, 16> wram{};

        // The role of frequency timer is to step wave generation. Each T-cycle the frequency timer
        // is decremented by 1. As soon as it reaches 0, it is reloaded with a value calculated using the below
        // formula, and the wave duty position register is incremented by 1.
        // See Channel 2 section: https://nightshade256.github.io/2021/03/27/gb-sound-emulation.html
        int frequencyTimer = 0; // (2048 - frequency) * 4

        // This is the index of the next sample to be played in the WRAM when the frequency timer reaches 0.
        // It wraps around 0-31, so, since the high nibble is before the low nibble, if it is even,
        // the high nibble of the next sample is played and if it is odd, the low nibble is played.
        uint8_t wramPosition = 0x00;

        // The description of these functions are provided in their implementations.
        void  tick();
        void  lengthTick();
        void  trigger();
        void  update(uint8_t offset, uint8_t data);
        void  enableChannel();
        void  disableChannel();
        float getSample();

        APU* apu = nullptr;
    } waveChannel;

    struct NoiseChannel {
        // NR41 (0xFF20) --LL LLLL  Length load (64-L)
        uint8_t lengthLoad = 0x00;
        int     lengthTimer = 0;

        // NR42 (0xFF21) VVVV APPP  Starting volume, Envelope add mode, period
        uint8_t period          = 0x00;
        bool    envelopeAddMode = false;
        uint8_t startVolume     = 0x00;

        int periodTimer   = 0;
        int currentVolume = 0;

        // NR43 (0xFF22) SSSS WDDD  Clock shift, Width mode of LFSR, Divisor code
        uint8_t divisorCode    = 0x00;  // 0=divisor 8, 1=divisor 16, 2=divisor 32, ..., 7=divisor 128
        bool    lfsrWidthMode  = false; // "width: 0 = 15-bit, 1 = 7-bit (more regular output; some frequencies sound more like pulse than noise)"
        uint8_t clockShift     = 0x00;

        uint8_t  divisors[8]    = { 8, 16, 32, 48, 64, 80, 96, 112 };
        int      frequencyTimer = 0; // divisors[divisorCode] << clockShift
        uint16_t lfsr           = 0x0000; // Linear Feedback Shift Register (15-bit or 7-bit depending on lfsrWidthMode)

        // NR44 (0xFF23) TL-- ----  Trigger, Length enable
        bool lengthEnable = false;
        bool triggered     = false;

        // These control whether the channel produces sound. If either is false, the channel is silenced.
        bool    enable        = false;
        bool    dacEnable     = false;  // Controlled by upper 5 bits of NR42
        uint8_t statusBitNR52 = 1 << 3; // Bit 3 is the status bit for channel 4


        // The description of these functions are provided in their implementations.
        void  tick();
        void  envelopeTick();
        void  lengthTick();
        void  trigger();
        void  update(uint8_t offset, uint8_t data);
        void  enableChannel();
        void  disableChannel();
        float getSample() const;

        APU* apu = nullptr;
    } noiseChannel;

private:
    uint8_t  frameSequencer = 0x00; // Generates clocks for channel modulation units
    void     frameSequencerTick();  // Called every 8192 T-cycles (512 Hz)
    uint16_t ticks          = 0x00; // When it reaches 8192, the frame sequencer is stepped


private:
    void mixAndQueueAudio(); // Mixes the audio channels and queues the audio buffer
    static const int AUDIO_SAMPLE_RATE = 44100; // 44.1 kHz
    static const int SAMPLE_SIZE       = 4096;  // (4 bytes per sample) * (1024 samples per buffer)
    int downSampleTimer = 90; // Decremented every CPU cycle; when it reaches 0, the audio is outputted to the speakers
    std::vector<float> audioBuffer; // Buffer for mixed audio samples
};