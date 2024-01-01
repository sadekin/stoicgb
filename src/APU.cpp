#include "APU.hpp"

APU::APU()
: audioBuffer(SAMPLE_SIZE, 0) {
    // Needs access to APU members.
    pulseChannel1.apu = this;
    pulseChannel2.apu = this;
    waveChannel.apu   = this;
    noiseChannel.apu  = this;

    // When length == 0, NR52's respective channel length status bit is set cleared.
    pulseChannel1.statusBitNR52 = 1 << 0;
    pulseChannel2.statusBitNR52 = 1 << 1;
    waveChannel.statusBitNR52   = 1 << 2;
    noiseChannel.statusBitNR52  = 1 << 3;

    // Reserve space in the audio buffer to improve performance by avoiding reallocations
    audioBuffer.reserve(SAMPLE_SIZE);

    // Initialize SDL audio specifications
    SDL_AudioSpec audioSpec;
    audioSpec.freq     = AUDIO_SAMPLE_RATE; // 44100 Hz
    audioSpec.format   = AUDIO_F32SYS;      // Floating point, system byte order
    audioSpec.channels = 2;                 // Stereo
    audioSpec.samples  = SAMPLE_SIZE;       // Number of samples for the audio buffer (might need to be tweaked)
    audioSpec.callback = nullptr;           // No callback function used, audio is queued manually
    audioSpec.userdata = this;              // User data is a pointer to the APU object

    // Open the audio device with the desired specifications (audioSpec)
    SDL_AudioSpec obtainedSpec;               // Structure to store the obtained audio specifications
    SDL_OpenAudio(&audioSpec, &obtainedSpec); // obtainedSpec is filled with the actual specifications
    SDL_PauseAudio(0);                        // Start playing audio (audio is initially paused)
}

APU::~APU() {
    SDL_CloseAudio();
}

/**
 * Reads from the APU registers and Wave RAM, whose address range is 0xFF10 - 0xFF3F.
 *
 * When an NRxx register is read back, the last written
 * value ORed with the following is returned:
 * -----------------------------
 *      NRx0 NRx1 NRx2 NRx3 NRx4
 * -----------------------------
 * NR1x  $80  $3F $00  $FF  $BF
 * NR2x  $FF  $3F $00  $FF  $BF
 * NR3x  $7F  $FF $9F  $FF  $BF
 * NR4x  $FF  $FF $00  $00  $BF
 * NR5x  $00  $00 $70
 * -----------------------------
 * Unmapped addresses $FF27-$FF2F always read back as $FF,
 * See https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware
 *
 * @param addr The address to read from.
 * @return The data stored by the register or Wave RAM.
 *
 * @note: All unused bits are set to 1.
 */
uint8_t APU::read(uint16_t addr) const {
    switch (addr) {
        // Pulse channel 1
        case 0xFF10: return nr10 | 0x80;
        case 0xFF11: return nr11 | 0x3F;
        case 0xFF12: return nr12 | 0x00;
        case 0xFF13: return nr13 | 0xFF;
        case 0xFF14: return nr14 | 0xBF;
        // Pulse channel 2
        case 0xFF16: return nr21 | 0x3F;
        case 0xFF17: return nr22 | 0x00;
        case 0xFF18: return nr23 | 0xFF;
        case 0xFF19: return nr24 | 0xBF;
        // Wave channel
        case 0xFF1A: return nr30 | 0x7F;
        case 0xFF1B: return nr31 | 0xFF;
        case 0xFF1C: return nr32 | 0x9F;
        case 0xFF1D: return nr33 | 0xFF;
        case 0xFF1E: return nr34 | 0xBF;
        // Noise channel
        case 0xFF20: return nr41 | 0xFF;
        case 0xFF21: return nr42 | 0x00;
        case 0xFF22: return nr43 | 0x00;
        case 0xFF23: return nr44 | 0xBF;
        // Control/Status
        case 0xFF24: return nr50 | 0x00;
        case 0xFF25: return nr51 | 0x00;
        case 0xFF26: return nr52 | 0x70;
        default: break;
    }

    // Wave pattern RAM (0xFF30 - 0xFF3F)
    if (0xFF30 <= addr && addr <= 0xFF3F)
        return waveChannel.wram[addr - 0xFF30];

//    printf("UNMAPPED APU::read(%04X) \n", addr);
    return 0xFF;
}

/**
 * Writes to the APU registers and the Wave RAM.
 *
 * @param addr The address to write to.
 * @param data The data to write to the register or Wave RAM.
 */
void APU::write(uint16_t addr, uint8_t data) {
    // If power is off, ignore writes to registers except NR41, NR52, Wave RAM.
    if (!control.power && addr < 0xFF26 && addr != 0xFF20)
        return;

    switch (addr) {
        // Pulse channel 1
        case 0xFF10: nr10 = data; pulseChannel1.update(0, nr10); return;
        case 0xFF11: nr11 = data; pulseChannel1.update(1, nr11); return;
        case 0xFF12: nr12 = data; pulseChannel1.update(2, nr12); return;
        case 0xFF13: nr13 = data; pulseChannel1.update(3, nr13); return;
        case 0xFF14: nr14 = data; pulseChannel1.update(4, nr14); return;
        // Pulse channel 2
        case 0xFF16: nr21 = data; pulseChannel2.update(1, nr21); return;
        case 0xFF17: nr22 = data; pulseChannel2.update(2, nr22); return;
        case 0xFF18: nr23 = data; pulseChannel2.update(3, nr23); return;
        case 0xFF19: nr24 = data; pulseChannel2.update(4, nr24); return;
        // Wave channel
        case 0xFF1A: nr30 = data; waveChannel.update(0, nr30); return;
        case 0xFF1B: nr31 = data; waveChannel.update(1, nr31); return;
        case 0xFF1C: nr32 = data; waveChannel.update(2, nr32); return;
        case 0xFF1D: nr33 = data; waveChannel.update(3, nr33); return;
        case 0xFF1E: nr34 = data; waveChannel.update(4, nr34); return;
        // Noise channel
        case 0xFF20: nr41 = data; noiseChannel.update(1, nr41); return;
        case 0xFF21: nr42 = data; noiseChannel.update(2, nr42); return;
        case 0xFF22: nr43 = data; noiseChannel.update(3, nr43); return;
        case 0xFF23: nr44 = data; noiseChannel.update(4, nr44); return;
        // Control/Status
        case 0xFF24: nr50 = data; control.update(0, nr50); return;
        case 0xFF25: nr51 = data; control.update(1, nr51); return;
        case 0xFF26:
            // Only bit 7 is writable.
            nr52 &= ~0x80;
            nr52 |= (data & 0x80);
            control.update(2, nr52);
            if (!control.power)
                clearRegisters();
            return;
        default: break;
    }

    // Wave pattern RAM (0xFF30 - 0xFF3F)
    if (0xFF30 <= addr && addr <= 0xFF3F)
        waveChannel.wram[addr - 0xFF30] = data;
    else
        printf("UNMAPPED APU::write(%04X, %02X) \n", addr, data);

}

void APU::init() {
    // Pulse channel 1
    nr10 = 0x80; pulseChannel1.update(0, nr10);
    nr11 = 0xBF; pulseChannel1.update(1, nr11);
    nr12 = 0xF3; pulseChannel1.update(2, nr12);
    nr13 = 0xFF; pulseChannel1.update(3, nr13);
    nr14 = 0xBF; pulseChannel1.update(4, nr14);
    // Pulse channel 2
    nr21 = 0x3F; pulseChannel2.update(1, nr21);
    nr22 = 0x00; pulseChannel2.update(2, nr22);
    nr23 = 0xFF; pulseChannel2.update(3, nr23);
    nr24 = 0xBF; pulseChannel2.update(4, nr24);
    // Wave channel
    nr30 = 0x7F; waveChannel.update(0, nr30);
    nr31 = 0xFF; waveChannel.update(1, nr31);
    nr32 = 0x9F; waveChannel.update(2, nr32);
    nr33 = 0xFF; waveChannel.update(3, nr33);
    nr34 = 0xBF; waveChannel.update(4, nr34);
    // Noise channel
    nr41 = 0xFF; noiseChannel.update(1, nr41);
    nr42 = 0x00; noiseChannel.update(2, nr42);
    nr43 = 0x00; noiseChannel.update(3, nr43);
    nr44 = 0xBF; noiseChannel.update(4, nr44);
    // Control
    nr50 = 0x77; control.update(0, nr50);
    nr51 = 0xF3; control.update(1, nr51);
    nr52 = 0xF1; control.update(2, nr52);
}

/**
 * Updates the entire audio processing unit each cycle.
 *
 * This method is responsible for advancing the state of the APU by one tick, which involves
 * updating the frame sequencer, stepping each audio channel, and mixing and queuing audio
 * for playback. It is designed to be called regularly as part of the main emulation loop to simulate
 * the progression of audio processing in the Game Boy.
 */
void APU::tick() {
    // Step the frame sequencer if enough T-cycles have passed.
    if (++ticks == 8192) {
        ticks = 0;
        frameSequencerTick();
    }

    // Step the channels.
    pulseChannel1.tick();
    pulseChannel2.tick();
    waveChannel.tick();
    noiseChannel.tick();

    // Mix and queue the audio.
    mixAndQueueAudio();
}

/**
 * Emulates a tick for the frame sequencer, which generates low frequency clocks for the
 * modulation units (Sweep, Length, and Envelope) of the channels. The FS is clocked at
 * 512 Hz (the CPU runs at 4194304 Hz => 4194304 / 512 = 8192 T-cycles = 2048 M-Cycles).
 * Here is a table that shows which unit is clocked and when:
 *   ---------------------------------------
 *   Step   Length Ctr  Vol Env     Sweep
 *   ---------------------------------------
 *   0      Clock       -           -
 *   1      -           -           -
 *   2      Clock       -           Clock
 *   3      -           -           -
 *   4      Clock       -           -
 *   5      -           -           -
 *   6      Clock       -           Clock
 *   7      -           Clock       -
 *   ---------------------------------------
 *   Rate   256 Hz      64 Hz       128 Hz
 *   ---------------------------------------
 * Source: https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware
 */
void APU::frameSequencerTick() {
    switch (frameSequencer) {
        case 0:
            pulseChannel1.lengthTick();
            pulseChannel2.lengthTick();
            waveChannel.lengthTick();
            noiseChannel.lengthTick();
            break;
        case 2:
            pulseChannel1.sweepTick();
            pulseChannel1.lengthTick();
            pulseChannel2.lengthTick();
            waveChannel.lengthTick();
            noiseChannel.lengthTick();
            break;
        case 4:
            pulseChannel1.lengthTick();
            pulseChannel2.lengthTick();
            waveChannel.lengthTick();
            noiseChannel.lengthTick();
            break;
        case 6:
            pulseChannel1.sweepTick();
            pulseChannel1.lengthTick();
            pulseChannel2.lengthTick();
            waveChannel.lengthTick();
            noiseChannel.lengthTick();
            break;
        case 7:
            pulseChannel1.envelopeTick();
            pulseChannel2.envelopeTick();
            noiseChannel.envelopeTick();
            break;
        default:
            break;
    }

    // Increment the frame sequencer (wraps around to 0 after 7).
    frameSequencer = frameSequencer == 7 ? 0 : frameSequencer + 1;
}

/**
 * Mixes audio samples from all channels, downsamples them, and queues them for playback.
 *
 * This function aggregates audio samples from each of the Game Boy's sound channels (two pulse channels,
 * a wave channel, and a noise channel), applies volume settings, downsamples the mixed audio, and queues
 * it for playback. It is designed to be called at the rate the audio hardware processes sound samples.
 */
void APU::mixAndQueueAudio() {
    // Check and update downsample timer
    if (--downSampleTimer <= 0) {
        downSampleTimer = 90; // Reset downsample timer

        float leftSample = 0.0f, rightSample = 0.0f;
        // Check if power is on and mix audio samples for left and right channels
        if (control.power) {
            // Mix audio samples for left channel
            if (control.leftChannel1Enable) leftSample += pulseChannel1.getSample();
            if (control.leftChannel2Enable) leftSample += pulseChannel2.getSample();
            if (control.leftChannel3Enable) leftSample += waveChannel.getSample();
            if (control.leftChannel4Enable) leftSample += noiseChannel.getSample();
            // Mix audio samples for right channel
            if (control.rightChannel1Enable) rightSample += pulseChannel1.getSample();
            if (control.rightChannel2Enable) rightSample += pulseChannel2.getSample();
            if (control.rightChannel3Enable) rightSample += waveChannel.getSample();
            if (control.rightChannel4Enable) rightSample += noiseChannel.getSample();
        }

        // Normalize volumes and apply overall volume settings (15=max volume).
        leftSample = (leftSample / 4.0f) * ((float) control.leftVolume / 15.0f);
        rightSample = (rightSample / 4.0f) * ((float) control.rightVolume / 15.0f);

        // Add processed samples to audio buffer.
        audioBuffer.push_back(leftSample);
        audioBuffer.push_back(rightSample);
    }

    // Check if audio buffer needs to be queued for playback.
    if (audioBuffer.size() >= SAMPLE_SIZE) {
        // Delay execution to let the audio queue drain to about a frame's worth of audio
        while ((SDL_GetQueuedAudioSize(1)) > SAMPLE_SIZE * sizeof(float))
            SDL2Timing::delay(std::chrono::milliseconds(1));
        // Queue audio data for playback
        SDL_QueueAudio(1, audioBuffer.data(), SAMPLE_SIZE * sizeof(float));
        // Clear the buffer for next cycle
        audioBuffer.clear();
    }
}

/**
 * Clears most of the APU registers after powering off
 * (after clearing NR52's power bit 7).
 */
void APU::clearRegisters() {
    // Pulse channel 1
    nr10 = 0x00; pulseChannel1.update(0, nr10);
    nr11 = 0x00; pulseChannel1.update(1, nr11);
    nr12 = 0x00; pulseChannel1.update(2, nr12);
    nr13 = 0x00; pulseChannel1.update(3, nr13);
    nr14 = 0x00; pulseChannel1.update(4, nr14);
    // Pulse channel 2
    nr21 = 0x00; pulseChannel2.update(1, nr21);
    nr22 = 0x00; pulseChannel2.update(2, nr22);
    nr23 = 0x00; pulseChannel2.update(3, nr23);
    nr24 = 0x00; pulseChannel2.update(4, nr24);
    // Wave channel
    nr30 = 0x00; waveChannel.update(0, nr30);
    nr31 = 0x00; waveChannel.update(1, nr31);
    nr32 = 0x00; waveChannel.update(2, nr32);
    nr33 = 0x00; waveChannel.update(3, nr33);
    nr34 = 0x00; waveChannel.update(4, nr34);
    // Noise channel
  /*nr41 is unchanged after power off and is always writable*/
    nr42 = 0x00; noiseChannel.update(2, nr42);
    nr43 = 0x00; noiseChannel.update(3, nr43);
    nr44 = 0x00; noiseChannel.update(4, nr44);
    // Control
    nr50 = 0x00; control.update(0, nr50);
    nr51 = 0x00; control.update(1, nr51);
  /*nr52 controls power and is always writable*/
}

// Control/Status ======================================================================================================
/**
 * Updates Control/Status's members based on the register that was just written to.
 *
 * @param offset The last digit of the NR5x register that was just written to, where offset = x = 0, 1, 2.
 * @param data A copy of the data held by registers NR50, NR51, or NR52.
 */
void APU::Control::update(uint8_t offset, uint8_t data) {
    switch (offset) {
        case 0: // NR50 (0xFF24) ALLL BRRR  Vin L enable, Left vol, Vin R enable, Right vol
            rightVolume    = data & 0x7;
            vinRightEnable = data & (1 << 3);
            leftVolume     = (data >> 4) & 0x7;
            vinLeftEnable  = data & (1 << 7);
            break;
        case 1: // NR51 (0xFF25) NW21 NW21  Left enables, Right enables
            rightChannel1Enable = data & (1 << 0);
            rightChannel2Enable = data & (1 << 1);
            rightChannel3Enable = data & (1 << 2);
            rightChannel4Enable = data & (1 << 3);
            leftChannel1Enable  = data & (1 << 4);
            leftChannel2Enable  = data & (1 << 5);
            leftChannel3Enable  = data & (1 << 6);
            leftChannel4Enable  = data & (1 << 7);
            break;
        case 2: // NR52 (0xFF26) P--- NW21  Power control/status, Channel length statuses
            channel1LengthStatus = data & (1 << 0);
            channel2LengthStatus = data & (1 << 1);
            channel3LengthStatus = data & (1 << 2);
            channel4LengthStatus = data & (1 << 3);
            power                = data & (1 << 7);
            break;
        default:
            break;
    }
}
// =====================================================================================================================


// Pulse Channels ======================================================================================================
/**
 * Updates a Pulse Channel's members based on the register that was just written to.
 *
 * @param offset The last digit of the NRxy register that was just written to,
 *               where offset = y = 0 (only for channel 1), 1, 2, 3, 4. and x = 1 or 2.
 * @param data A copy of the data held by registers NR10, NR11 or NR12, NR13 or NR23, ...
 */
void APU::PulseChannel::update(uint8_t offset, uint8_t data) {
    switch (offset) {
        case 0: // NR10: -PPP NSSS  Sweep period, negate, shift (unique to channel 1)
            sweepShift = data & 0x7;
            if (sweepEnable && sweepNegate && !(data & 0x8) && negateModeUsed) {
                negateModeUsed = false;
                disableChannel();
            }
            sweepNegate = data & 0x8;
            sweepPeriod = (data & 0x70) >> 4;
            break;
        case 1: // NR11 and NR21: DDLL LLLL  Duty, Length load (64-L)
            lengthLoad  = data & 0x3F;
            lengthTimer = 64 - lengthLoad;
            duty        = data >> 6;
            if (!lengthTimer)
                disableChannel();
            break;
        case 2: // NR12 and NR22: VVVV APPP  Starting volume, Envelope add mode, period
            period          = data & 0x7;
            envelopeAddMode = data & 0x8;
            startVolume     = data >> 4;

            // "Channel x’s DAC is enabled if and only if [NRx2] & $F8 != 0; the exception is CH3,
            // whose DAC is directly controlled by bit 7 of NR30 instead. Note that the envelope
            // functionality changes the volume, but not the value stored in NRx2, and thus doesn't
            // disable the DACs." - https://gbdev.io/pandocs/Audio_details.html#dacs
            dacEnable = data & 0xF8;
            if (!dacEnable)
                disableChannel(); // Disable channel immediately if DAC disabled
            break;
        case 3: // NR13 and NR23: FFFF FFFF  Frequency LSB
            frequencyLSB = data;
            break;
        case 4: // NR14 and NR24: TL-- -FFF  Trigger, Length enable, Frequency MSB
            frequencyMSB = data & 0x07;
            lengthEnable = data & 0x40;
            triggered    = data & 0x80;
            if (triggered)
                trigger();
            break;
        default:
            break;
    }
}

/**
 * Updates the pulse channel's internal state based on the frequency timer.
 *
 * This method is responsible for progressing the state of the pulse channel by decrementing the frequency timer
 * and updating the wave duty position accordingly. It is designed to be called regularly to simulate the
 * progression of the wave duty cycle in the audio playback.
 */
void APU::PulseChannel::tick() {
    // "The role of frequency timer is to step wave generation. Each T-cycle the frequency timer is decremented by 1.
    // As soon as it reaches 0, it is reloaded with a value calculated using the below formula, and the wave duty
    // position register is incremented by 1." - https://nightshade256.github.io/2021/03/27/gb-sound-emulation.html
    if (--frequencyTimer <= 0) {
        frequencyTimer = (2048 - ((frequencyMSB << 8) | frequencyLSB)) * 4; // Reload the timer
        waveDutyPosition = (waveDutyPosition + 1) & 0x7;                    // Increment the wave duty position
    }
}

/**
 * Calculates the new frequency based on the sweep parameters.
 * Also checks for overflow, which disables the channel.
 *
 * @return The new frequency.
 */
void APU::PulseChannel::sweepFreqCalculation(bool update) {
    uint16_t newFrequency = shadowFrequency + (shadowFrequency >> sweepShift) * (sweepNegate ? -1 : 1);

    // Obscure hardware quirk. Disable channel when clearing the sweep negate bit in NR10 after
    // performing at least one calculation (since the last trigger event) using the negate mode.
    negateModeUsed = sweepNegate; // Set flag if negate mode was used

    if (newFrequency > 0x7FF) // Overflow check (frequency is 11 bits wide, 0x7FF = 2047)
        disableChannel();
    else if (sweepShift && update) {
        shadowFrequency = newFrequency;

        frequencyMSB    = (newFrequency >> 8) & 0x7;
        apu->nr14       = (apu->nr14 & ~0x7) | frequencyMSB;

        frequencyLSB    = newFrequency & 0xFF;
        apu->nr13       = frequencyLSB;
    }
}

/**
 * Emulates a sweep tick for Pulse Channel 1.
 * "On a sweep clock from the frame sequencer the following steps occur,
 *      1. If the sweep timer is greater than zero, we decrement it.
 *      2. Now, if due to previous operation the sweep timer becomes zero only then continue with step 3.
 *      3. The sweep timer is reloaded with the sweep period value.
 *         If sweep period is zero, then sweep timer is loaded with the value 8 instead.
 *      4. If sweep is enabled and sweep period is non-zero, a new frequency is calculated.
 *         In this step we also perform an additional routine called as the overflow check.
 *      5. Now, if the new frequency is less than 2048 and the sweep shift is non-zero, then the shadow
 *         frequency and frequency registers are loaded with this new frequency.
 *      6. We now perform the overflow check again."
 * Source: https://nightshade256.github.io/2021/03/27/gb-sound-emulation.html
 */
void APU::PulseChannel::sweepTick() {
    if (--sweepTimer <= 0) {
        sweepTimer = sweepPeriod == 0 ? 8 : sweepPeriod;

        if (sweepEnable && sweepPeriod) {
            sweepFreqCalculation(true);  // Calculate new frequency and update if sweep shift is non-zero
            sweepFreqCalculation(false); // Overflow check (no update here)
        }
    }
}

/**
 * Emulates an envelope tick for the pulse channels, which is used to change the volume of a channel.
 * "Now on a volume clock from the frame sequencer the following steps take place,
 *      1. If the period (parameter three, not the period timer!) is zero, return.
 *      2. If the period timer is a non-zero value, decrement it.
 *      3. Now, if due to the previous step, the period timer becomes zero only then continue with step 4.
 *      4. Reload the period timer with the period (parameter 3).
 *      5. If the current volume is below 0xF and the envelope direction is upwards or if the current volume is above
 *         0x0 and envelope direction is downwards, we increment or decrement the current volume respectively.
 * Source: https://nightshade256.github.io/2021/03/27/gb-sound-emulation.html
 */
void APU::PulseChannel::envelopeTick() {
    if (period == 0) // Envelope is essentially disabled
        return;

    if (periodTimer > 0)
        periodTimer--;

    if (periodTimer == 0) {
        periodTimer = period;
        if ((currentVolume < 0xF && envelopeAddMode) || (currentVolume > 0x0 && !envelopeAddMode))
            currentVolume += envelopeAddMode ? 1 : -1;
    }
}

/**
 * Emulates a length tick for pulse channels. This is used to disable the channel when the length timer reaches 0.
 * "Whenever a length clock is provided by the frame sequencer AND bit 6 of NR24 register is set,
 * the length timer is decremented by one.
 * If the length timer, (only) as a result of the above decrement reaches the value of zero then
 * the channel is disabled until the next trigger event."
 * Source: https://nightshade256.github.io/2021/03/27/gb-sound-emulation.html
 */
void APU::PulseChannel::lengthTick() {
    if (lengthEnable && lengthTimer > 0) {
        if (--lengthTimer == 0)
            disableChannel(); // Channel disabled until next trigger event
    }
}

/**
 * Triggers the pulse channel, initializing it for audio playback.
 *
 * This method initializes the pulse channel and sets it up for sound playback. It involves enabling the channel
 * and resetting its sweep, length, envelope, and frequency components to their respective start states as per
 * Game Boy sound hardware specifications.
 *
 * Sweep:
 *   On a trigger event, the following actions are taken for the sweep unit:
 *     1. The shadow frequency register is loaded with the current frequency.
 *     2. The sweep timer is loaded with the sweep period, or 8 if the period is zero.
 *     3. Sweep is enabled if the sweep period or shift is non-zero.
 *     4. If the sweep shift is non-zero, an overflow check is performed, potentially disabling the channel.
 *
 * Frequency:
 *   The frequency timer is calculated using the formula: (2048 - frequency) * 4.
 *   This calculation translates the frequency registers' values into a timer value, controlling the playback
 *   rate of the wave duty cycle.
 *
 * Length Timer:
 *   If the length timer is currently zero, it is set to 64. This behavior mimics the Game Boy
 *   sound hardware's response to a trigger event, where the length timer is reloaded if it has expired.
 *
 * Envelope:
 *   On a trigger event, the internal period timer is loaded with the envelope period, and
 *   the current volume register is loaded with the initial volume.
 *
 * See https://nightshade256.github.io/2021/03/27/gb-sound-emulation.html
 */
void APU::PulseChannel::trigger() {
    enableChannel();

    // Sweep initialization
    shadowFrequency = (frequencyMSB << 8) | frequencyLSB;
    sweepTimer      = sweepPeriod == 0 ? 8 : sweepPeriod;
    sweepEnable     = sweepPeriod || sweepShift;
    if (sweepShift)
        sweepFreqCalculation(false); // Overflow check

    // Frequency Timer Calculation
    frequencyTimer = (2048 - ((frequencyMSB << 8) | frequencyLSB)) * 4;

    // Length Timer Initialization
    if (lengthTimer == 0)
        lengthTimer = 64;

    // Envelope Initialization
    periodTimer   = period;
    currentVolume = startVolume;

    // Channel is immediately disabled again if DAC is disabled.
    if (!dacEnable)
        disableChannel();
}

void APU::PulseChannel::enableChannel() {
    enable = true;
    apu->nr52 |= statusBitNR52;
    apu->control.update(2, apu->nr52);
}

void APU::PulseChannel::disableChannel() {
    enable = false;                    // Channel disabled until next trigger event
    apu->nr52 &= ~statusBitNR52;       // Clear channel status bit in NR52
    apu->control.update(2, apu->nr52); // Update control
}

/**
 * Retrieves the current audio sample from the pulse channel.
 *
 * This method computes the audio sample for a pulse channel based on the current state of the
 * wave duty cycle and the volume settings. The pulse channel generates audio waves of varying duty cycles,
 * creating different tonal sounds.
 *
 * @return A float representing the current audio sample. The value is normalized between 0.0 and 1.0,
 *         where 0.0 represents silence. If the pulse channel or its DAC (Digital-to-Analog Converter)
 *         is disabled, the method returns 0.0, indicating silence.
 *
 * @note: The sample value is determined by the current position in the wave duty table. The wave duty
 *        table defines a pattern of high and low amplitudes over a cycle. If the current position in the
 *        duty cycle is high, the method returns the normalized current volume. If it is low, the method
 *        returns 0.0, representing the low part of the wave duty cycle. The volume is normalized by dividing
 *        the current volume level by the maximum volume level (15).
 */
float APU::PulseChannel::getSample() {
    if (!enable || !dacEnable) return 0.0f; // If channel is disabled or DAC is disabled, return silence

    // Check the current position in the wave duty table and return the amplitude.
    bool isHigh = waveDutyTable[duty][waveDutyPosition];  // Amplitude
    return isHigh ? (float) currentVolume / 15.0f : 0.0f; // Normalize volume to range 0-1
}
// =====================================================================================================================


// Wave Channel ========================================================================================================
/**
 * Updates the wave channel's settings based on the provided offset and data.
 *
 * @param offset An unsigned 8-bit integer representing the offset of the register being updated.
 *               This corresponds to different NR3X registers in the Game Boy hardware.
 *               For example, offset = 0 corresponds to register NR30, offset = 1 corresponds to NR31, etc.
 * @param data An unsigned 8-bit integer representing the data that was just written to the register.
 */
void APU::WaveChannel::update(uint8_t offset, uint8_t data) {
    switch (offset) {
        case 0: // NR30 (0xFF1A) E--- ----  DAC power
            dacEnable = data & 0x80;
            if (!dacEnable)
                disableChannel(); // Disable channel immediately if DAC disabled
            break;
        case 1: // NR31 (0xFF1B) LLLL LLLL  Length load (256-L)
            lengthLoad  = data;
            lengthTimer = 256 - lengthLoad;
            if (!lengthTimer)
                disableChannel(); // Disable channel immediately if length timer is 0
            break;
        case 2: // NR32 (0xFF1C) -VV- ----  Volume code
            volumeCode = (data & 0x60) >> 5;
            break;
        case 3: // NR33 (0xFF1D) FFFF FFFF  Frequency LSB
            frequencyLSB = data;
            break;
        case 4: // NR34 (0xFF1E) TL-- -FFF  Trigger, Length enable, Frequency MSB
            frequencyMSB = data & 0x7;
            lengthEnable = data & 0x40;
            triggered    = data & 0x80;
            if (triggered)
                trigger();
            break;
        default:
            break;
    }
}

/**
 * Updates the wave channel's internal state based on the frequency timer.
 *
 * This method progresses the state of the wave channel by decrementing the frequency timer
 * and updating the position in the Wave RAM (WRAM) accordingly. It is designed to be called
 * regularly to simulate the passage of time in the audio playback.
 *
 * The method operates as follows:
 * - The frequency timer is decremented. If it reaches zero or a negative value, it indicates
 *   that it's time to move to the next sample in the Wave RAM.
 * - The frequency timer is then reloaded based on the current frequency settings. This is done
 *   using the formula: (2048 - frequency) * 2. This calculation translates the frequency
 *   registers' values into a new timer value, controlling when the next WRAM position
 *   should be accessed.
 * - The WRAM position is incremented and wrapped around to stay within the 32-byte boundary
 *   (0x1F in hexadecimal or 31 in decimal), ensuring the wave pattern loops correctly.
 */
void APU::WaveChannel::tick() {
    if (--frequencyTimer <= 0) {
        frequencyTimer = (2048 - ((frequencyMSB << 8) | frequencyLSB)) * 2;
        wramPosition = (wramPosition + 1) & 0x1F;
    }
}

/**
 * Emulates a length tick for the wave channel.
 * "Whenever a length clock is provided by the frame sequencer AND bit 6 of NR24 register is set,
 * the length timer is decremented by one.
 * If the length timer, (only) as a result of the above decrement reaches the value of zero then
 * the channel is disabled until the next trigger event."
 * Source: https://nightshade256.github.io/2021/03/27/gb-sound-emulation.html
 */
void APU::WaveChannel::lengthTick() {
    if (lengthEnable && lengthTimer > 0) {
        if (--lengthTimer == 0)
            disableChannel(); // Channel disabled until next trigger event
    }
}

/**
 * Triggers the wave channel, initializing it for audio playback.
 *
 * This method is responsible for enabling the wave channel and setting its initial state,
 * which is essential for starting the playback of wave-based audio. It involves resetting
 * the position in the wave RAM (wram), initializing the length timer, and calculating the
 * frequency timer based on the current frequency settings.
 *
 * - The wave channel is enabled, and the position in the Wave RAM is reset to 0.
 * - The length timer is set to 256 if it's currently 0. This behavior mimics the Game Boy
 *   sound hardware's response to a trigger event on channel three, where the length timer
 *   is reloaded if it has expired.
 * - The frequency timer is calculated using the formula:
 *   (2048 - frequency) * 2. This calculation translates the frequency registers'
 *   values into a timer value, controlling the playback rate of the wave data.
 *
 * @note: This method is called when there is a trigger event for the wave channel,
 *        (setting bit 7 of the NR34 register).
 */
void APU::WaveChannel::trigger() {
    // Enable the channel and reset the position in the wave RAM.
    enableChannel();
    wramPosition = 0;

    // Length Timer Initialization
    if (lengthTimer == 0)
        lengthTimer = 256;

    // Frequency Timer Calculation
    frequencyTimer = (2048 - ((frequencyMSB << 8) | frequencyLSB)) * 2;

    // Channel is immediately disabled again if DAC is disabled.
    if (!dacEnable)
        disableChannel();
}

void APU::WaveChannel::enableChannel() {
    enable = true;
    apu->nr52 |= statusBitNR52;
    apu->control.update(2, apu->nr52);
}

void APU::WaveChannel::disableChannel() {
    enable = false;                    // Channel disabled until next trigger event
    apu->nr52 &= ~statusBitNR52;       // Clear channel status bit in NR52
    apu->control.update(2, apu->nr52); // Update control
}

/**
 * Retrieves the current audio sample from the wave channel.
 *
 * This function computes and returns the audio sample for the wave channel based on the current
 * state of the Wave RAM (WRAM) and the channel's volume settings. It also checks the status of
 * the channel and its Digital-to-Analog Converter (DAC) to determine if sound should be produced.
 *
 * @return A float representing the current audio sample. The value ranges from 0.0 to 1.0,
 *         where 0.0 represents silence. If the wave channel or its DAC is disabled,
 *         the function returns 0.0 signifying silence.
 *
 * @note The function first checks if the wave channel and its DAC are enabled. If either is
 *       disabled, it returns 0.0 (silence).
 *       The sample is extracted from the WRAM using the current position (wramPosition).
 *       The WRAM stores 4-bit samples, so for even positions, the high nibble is used, and for
 *       odd positions, the low nibble is used since high nibbles are played before low nibbles.
 *       The volume of the sample is then adjusted based on the 'volumeCode'. If 'volumeCode' is 0,
 *       the output is muted. Otherwise, the volume is adjusted by multiplying the sample by a factor
 *       determined by 'volumeCode', specifically 1 shifted left by (volumeCode - 1).
 */
float APU::WaveChannel::getSample() {
    if (!enable || !dacEnable) return 0.0f;

    uint8_t sample = wram[wramPosition / 2];
    sample = (wramPosition & 1) ? (sample & 0xF) : (sample >> 4); // odd=low nibble, even=high nibble
    return ((float) sample / 15.0f) * (float) (volumeCode == 0 ? 0 : 1 << (volumeCode - 1));
}
// =====================================================================================================================


// Noise Channel =======================================================================================================
/**
 * Updates Noise Channel's members based on the register that was just written to.
 *
 * @param offset The last digit of the NR4x register that was just written to, where offset = x = 1, 2, 3, 4.
 * @param data A copy of the data held by registers NR41, NR42, NR43, NR44.
 */
void APU::NoiseChannel::update(uint8_t offset, uint8_t data) {
    switch (offset) {
        case 1: // NR41 (0xFF20) --LL LLLL  Length load (64-L)
            lengthLoad  = data & 0x3F;
            lengthTimer = 64 - lengthLoad;
            if (!lengthTimer)
                disableChannel(); // Disable channel immediately if length timer is 0
            break;
        case 2: // NR42 (0xFF21) VVVV APPP  Starting volume, Envelope add mode, period
            period          = data & 0x7;
            envelopeAddMode = data & 0x8;
            startVolume     = data >> 4;

            dacEnable = data & 0xF8;
            if (!dacEnable)
                disableChannel(); // Disable channel immediately if DAC disabled
            break;
        case 3: // NR43 (0xFF22) SSSS WDDD  Clock shift, Width mode of LFSR, Divisor code
            divisorCode   = data & 0x7;
            lfsrWidthMode = (data & 0x8) >> 3;
            clockShift    = data >> 4;
            break;
        case 4: // NR44 (0xFF23) TL-- ----  Trigger, Length enable
            lengthEnable = data & 0x40;
            triggered    = data & 0x80;
            if (triggered)
                trigger();
            break;
        default:
            break;
    }
}

/**
 * Emulates a tick for the noise channel.
 * "Whenever the frequency timer expires the following operations take place,
 *     1. The frequency timer is reloaded using the above formula. [Frequency Timer = Divisor << Shift Amount]
 *     2. The XOR result of the 0th and 1st bit of LFSR is computed.
 *     3. The LFSR is shifted right by one bit and the above XOR result is stored in bit 14.
 *     4. If the width mode bit is set, the XOR result is also stored in bit 6."
 * - https://nightshade256.github.io/2021/03/27/gb-sound-emulation.html
 */
void APU::NoiseChannel::tick() {
    if (--frequencyTimer <= 0) {
        frequencyTimer = divisors[divisorCode] << clockShift; // Reload the timer

        uint8_t xorResult = (lfsr & 0x1) ^ ((lfsr >> 1) & 0x1);
        lfsr = (xorResult << 14) | (lfsr >> 1);

        if (lfsrWidthMode) {
            lfsr &= ~(1 << 6);
            lfsr |= xorResult << 6;
        }
    }
}

/**
 * Emulates an envelope tick for the noise channel. This is used to change the volume of the channel.
 * "Now on a volume clock from the frame sequencer the following steps take place,
 *      1. If the period (parameter three, not the period timer!) is zero, return.
 *      2. If the period timer is a non-zero value, decrement it.
 *      3. Now, if due to the previous step, the period timer becomes zero only then continue with step 4.
 *      4. Reload the period timer with the period (parameter 3).
 *      5. If the current volume is below 0xF and the envelope direction is upwards or if the current volume is above
 *         0x0 and envelope direction is downwards, we increment or decrement the current volume respectively.
 * - https://nightshade256.github.io/2021/03/27/gb-sound-emulation.html
 */
void APU::NoiseChannel::envelopeTick() {
    if (periodTimer == 0)
        return;

    if (--periodTimer == 0) {
        periodTimer = period;
        if ((currentVolume < 0xF && envelopeAddMode) || (currentVolume > 0x0 && !envelopeAddMode))
            currentVolume += envelopeAddMode ? 1 : -1;
    }
}

/**
 * Updates the length timer of the noise channel.
 *
 * This method is responsible for decrementing the length timer of the noise channel, if the length counter
 * is enabled. The length timer controls the duration for which the noise channel remains active. Once the
 * length timer reaches zero, the channel is disabled, ceasing its sound production.
 */
void APU::NoiseChannel::lengthTick() {
    if (lengthEnable && lengthTimer > 0) {
        if (--lengthTimer == 0)
            disableChannel(); // Channel disabled until next trigger event
    }
}

/**
 * Triggers the noise channel to start playing sound.
 *
 * This function initializes the noise channel and sets it up for sound playback.
 * It involves enabling the channel and resetting its length, envelope, and Linear Feedback Shift Register
 * (LFSR) components to their respective start states as per Game Boy sound hardware specifications.
 *
 * Length Timer:
 *   If the length timer is currently zero, it is set to 64. This simulates the behavior
 *   of the Game Boy sound hardware on a trigger event, where the length timer is reloaded
 *   if it has expired.
 *
 * Envelope:
 *   On a trigger event, the internal period timer is loaded with the 'period' value and
 *   the current volume register is loaded with the 'startVolume' value. This simulates
 *   the envelope initialization in the Game Boy sound hardware.
 *
 * LFSR:
 *   The LFSR is set to 0x7FFF (binary: 0111 1111 1111 1111). This step initializes the LFSR
 *   to a predefined state at the start of sound playback, which is essential for generating
 *   the characteristic noise sound of this channel.
 *
 * @note This function is typically called when the bit 7 of NRx4 is enabled, indicating
 *       a trigger event for the noise channel. It is a critical part of emulating
 *       the sound generation functionality of the original Game Boy hardware.
 */
void APU::NoiseChannel::trigger() {
    enableChannel();

    // Length Timer Initialization
    if (lengthTimer == 0)
        lengthTimer = 64;

    // Envelope Initialization
    periodTimer   = period;
    currentVolume = startVolume;

    // LFSR Initialization
    lfsr = 0x7FFF; // 0b 0111 1111 1111 1111 = 2^15 - 1

    // Channel is immediately disabled again if DAC is disabled.
    if (!dacEnable)
        disableChannel();
}

void APU::NoiseChannel::enableChannel() {
    enable = true;
    apu->nr52 |= statusBitNR52;
    apu->control.update(2, apu->nr52);
}

void APU::NoiseChannel::disableChannel() {
    enable = false;                    // Channel disabled until next trigger event
    apu->nr52 &= ~statusBitNR52;       // Clear channel status bit in NR52
    apu->control.update(2, apu->nr52); // Update control
}

/**
 * Retrieves the current audio sample from the noise channel.
 *
 * This function computes and returns the audio sample for the noise channel
 * based on the Linear Feedback Shift Register (LFSR) value. The LFSR is used
 * to generate a pseudo-random noise pattern. The function also takes into account
 * the current volume and the status of the channel and its Digital-to-Analog Converter (DAC).
 *
 * @return A float representing the current audio sample. The value ranges from 0.0 to 1.0,
 *         where 0.0 represents silence. If the noise channel or its DAC is disabled,
 *         the function returns 0.0 signifying silence.
 *
 * @note The function checks if the noise channel and its DAC are enabled before proceeding
 *       with the sample calculation. If either is disabled, it returns 0.0.
 *       The sample value is determined by the LFSR; a 0 bit in the LFSR corresponds to a high
 *       volume output. The output volume is normalized to the range [0, 1] by dividing
 *       the current volume level by the maximum volume level (15).
 */
float APU::NoiseChannel::getSample() const {
    if (!enable || !dacEnable) return 0.0f; // If channel is disabled or DAC is disabled, return silence.

    // The LFSR determines the noise pattern. A value of 0 in the LFSR means high volume.
    return (lfsr & 0x1) ? 0.0f : (float) currentVolume / 15.0f; // Normalize volume to range 0-1
}
// =====================================================================================================================