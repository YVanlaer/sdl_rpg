#include "Sfx.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float SAMPLE_RATE = 44100.0f;
constexpr float TWO_PI = 6.28318530718f;

float waveform(Wave wave, float phase) {
    const float t = phase / TWO_PI;  // cycles
    switch (wave) {
        case Wave::Square: return std::sin(phase) >= 0.0f ? 1.0f : -1.0f;
        case Wave::Sawtooth: return 2.0f * (t - std::floor(t + 0.5f));
        case Wave::Triangle:
            return 2.0f * std::fabs(2.0f * (t - std::floor(t + 0.5f))) - 1.0f;
        case Wave::Sine:
        default: return std::sin(phase);
    }
}
}  // namespace

Sfx::~Sfx() {
    shutdown();
}

void Sfx::shutdown() {
    if (stream) {
        SDL_DestroyAudioStream(stream);
        stream = nullptr;
    }
}

bool Sfx::init() {
    SDL_AudioSpec spec{SDL_AUDIO_F32, 1, static_cast<int>(SAMPLE_RATE)};
    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec,
                                       &Sfx::audioCallback, this);
    if (!stream) return false;
    SDL_ResumeAudioStreamDevice(stream);
    return true;
}

void Sfx::queue(Wave wave, float freq, float dur, float vol, float slideTo, float delay) {
    if (!stream) return;
    SDL_LockAudioStream(stream);
    tones.push_back(Tone{wave, freq, dur, slideTo, vol, delay});
    SDL_UnlockAudioStream(stream);
}

void Sfx::play(const std::string& name) {
    if (muted || !stream) return;

    if (name == "swing") queue(Wave::Square, 320, 0.12f, 0.05f, 90);
    else if (name == "hit") queue(Wave::Square, 200, 0.08f, 0.08f, 120);
    else if (name == "hurt") queue(Wave::Sawtooth, 140, 0.25f, 0.09f, 60);
    else if (name == "squish") queue(Wave::Sawtooth, 110, 0.18f, 0.07f, 45);
    else if (name == "coin") {
        queue(Wave::Square, 880, 0.07f, 0.05f);
        queue(Wave::Square, 1320, 0.12f, 0.05f, 0, 0.07f);
    } else if (name == "heal") queue(Wave::Sine, 520, 0.2f, 0.07f, 780);
    else if (name == "levelup") {
        const float notes[] = {523, 659, 784, 1047};
        for (int i = 0; i < 4; ++i) queue(Wave::Square, notes[i], 0.12f, 0.06f, 0, i * 0.09f);
    } else if (name == "open") queue(Wave::Triangle, 220, 0.2f, 0.08f, 440);
    else if (name == "quest") {
        queue(Wave::Square, 660, 0.1f, 0.06f);
        queue(Wave::Square, 880, 0.15f, 0.06f, 0, 0.1f);
    } else if (name == "win") {
        const float notes[] = {523, 659, 784, 1047, 1319};
        for (int i = 0; i < 5; ++i) queue(Wave::Triangle, notes[i], 0.18f, 0.07f, 0, i * 0.12f);
    } else if (name == "death") {
        const float notes[] = {440, 349, 262, 196};
        for (int i = 0; i < 4; ++i) queue(Wave::Sawtooth, notes[i], 0.25f, 0.06f, 0, i * 0.18f);
    } else if (name == "potion") queue(Wave::Sine, 400, 0.15f, 0.07f, 800);
    else if (name == "deny") queue(Wave::Square, 160, 0.15f, 0.06f, 100);
}

void SDLCALL Sfx::audioCallback(void* userdata, SDL_AudioStream* stream,
                                int additionalAmount, int /*totalAmount*/) {
    Sfx* self = static_cast<Sfx*>(userdata);
    const int frames = additionalAmount / static_cast<int>(sizeof(float));
    if (frames <= 0) return;
    self->mixBuf.resize(frames);
    self->generate(self->mixBuf.data(), frames);
    SDL_PutAudioStreamData(stream, self->mixBuf.data(), additionalAmount);
}

void Sfx::generate(float* out, int frames) {
    std::fill(out, out + frames, 0.0f);

    for (Tone& tone : tones) {
        for (int i = 0; i < frames; ++i) {
            if (tone.delay > 0.0f) {
                tone.delay -= 1.0f / SAMPLE_RATE;
                continue;
            }
            if (tone.t >= tone.dur) break;

            const float k = tone.t / tone.dur;
            float f = tone.freq;
            if (tone.slideTo > 0.0f) f = tone.freq * std::pow(tone.slideTo / tone.freq, k);
            tone.phase += TWO_PI * f / SAMPLE_RATE;

            const float gain = tone.vol * std::pow(0.001f / tone.vol, k);
            out[i] += waveform(tone.wave, tone.phase) * gain;
            tone.t += 1.0f / SAMPLE_RATE;
        }
    }

    tones.erase(std::remove_if(tones.begin(), tones.end(),
                               [](const Tone& t) { return t.t >= t.dur; }),
                tones.end());

    for (int i = 0; i < frames; ++i) out[i] = SDL_clamp(out[i], -1.0f, 1.0f);
}
