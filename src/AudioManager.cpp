#include "AudioManager.h"
#include <iostream>

namespace {
    const char* sfxFile(Sfx id) {
        switch (id) {
            case Sfx::SHOOT_LASER: return "assets/audio/shoot_laser.wav";
            case Sfx::SHOOT_TICK:  return "assets/audio/shoot_tick.wav";
            case Sfx::HIT:         return "assets/audio/hit.wav";
            case Sfx::KILL:        return "assets/audio/kill.wav";
            case Sfx::LEAK:        return "assets/audio/leak.wav";
            case Sfx::WAVE_START:  return "assets/audio/wave_start.wav";
            case Sfx::WAVE_END:    return "assets/audio/wave_end.wav";
            case Sfx::BUY:         return "assets/audio/buy.wav";
            case Sfx::ERROR:       return "assets/audio/error.wav";
            case Sfx::PLACE:       return "assets/audio/place.wav";
            case Sfx::SELL:        return "assets/audio/sell.wav";
            case Sfx::TOKEN:       return "assets/audio/token.wav";
            case Sfx::COIN:        return "assets/audio/coin.wav";
            case Sfx::JINGLE_WIN:  return "assets/audio/jingle_win.wav";
            case Sfx::JINGLE_LOSE: return "assets/audio/jingle_lose.wav";
            case Sfx::COUNT:       break;
        }
        return "";
    }

    // Sunetele de gameplay frecvente primesc jitter de pitch; cele "de
    // ceremonie" (jingle, alarma, start/end de val) raman identice mereu.
    bool wantsJitter(Sfx id) {
        switch (id) {
            case Sfx::SHOOT_LASER:
            case Sfx::SHOOT_TICK:
            case Sfx::HIT:
            case Sfx::KILL:
            case Sfx::PLACE:
            case Sfx::SELL:
            case Sfx::BUY:
            case Sfx::TOKEN:
            case Sfx::COIN:
                return true;
            default:
                return false;
        }
    }
}

AudioManager::AudioManager() {
    for (size_t i = 0; i < SFX_COUNT; ++i) {
        loaded_[i] = buffers_[i].loadFromFile(sfxFile(static_cast<Sfx>(i)));
        if (!loaded_[i]) {
            std::cerr << "AudioManager: lipseste " << sfxFile(static_cast<Sfx>(i)) << "\n";
        }
    }
    voices_.resize(VOICE_POOL);

    music_ok_ = music_.openFromFile("assets/audio/music_loop.wav");
    if (music_ok_) {
        music_.setLoop(true);
        music_.setVolume(35.0f);
    }
}

void AudioManager::play(Sfx id, float volume, float pitch) {
    size_t idx = static_cast<size_t>(id);
    if (idx >= SFX_COUNT || !loaded_[idx]) return;

    // Throttle per-sunet (anti-spam la attack speed mare / 4x).
    if (last_play_[idx].getElapsedTime().asSeconds() < 0.04f) return;
    last_play_[idx].restart();

    if (wantsJitter(id)) {
        std::uniform_real_distribution<float> jitter(0.94f, 1.06f);
        pitch *= jitter(rng_);
    }

    for (auto& v : voices_) {
        if (v.getStatus() != sf::Sound::Playing) {
            v.setBuffer(buffers_[idx]);
            v.setVolume(volume);
            v.setPitch(pitch);   // mereu setat: vocile se refolosesc
            v.play();
            return;
        }
    }
    // toate vocile ocupate: sarim peste sunet (mai bine decat sa taiem unul)
}

void AudioManager::startMusic() {
    if (music_ok_ && music_.getStatus() != sf::Music::Playing) {
        music_.play();
    }
}

void AudioManager::stopMusic() {
    if (music_ok_) music_.stop();
}
