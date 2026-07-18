#pragma once

#include <SDL3/SDL.h>

#include <string>
#include <vector>

enum class Wave { Square, Sawtooth, Sine, Triangle };

// Tiny procedural sound synth — the C++ counterpart of the WebAudio tone
// table in the original JS game. Sounds are square/sawtooth/sine/triangle
// oscillator blips with an exponential frequency slide and volume envelope,
// mixed in the audio stream callback.
class Sfx {
public:
    ~Sfx();

    bool init();
    void shutdown();  // safe to call more than once
    void play(const std::string& name);
    void toggleMute() { muted = !muted; }

private:
    struct Tone {
        Wave wave;
        float freq;
        float dur;
        float slideTo;  // 0 = no slide
        float vol;
        float delay;
        // playback state
        float t = 0.0f;
        float phase = 0.0f;
    };

    static void SDLCALL audioCallback(void* userdata, SDL_AudioStream* stream,
                                      int additionalAmount, int totalAmount);
    void generate(float* out, int frames);
    void queue(Wave wave, float freq, float dur, float vol, float slideTo = 0.0f,
               float delay = 0.0f);

    SDL_AudioStream* stream = nullptr;
    std::vector<Tone> tones;
    std::vector<float> mixBuf;
    bool muted = false;
};
