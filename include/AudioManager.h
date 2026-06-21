#pragma once
#include <SFML/Audio.hpp>
#include <array>
#include <random>
#include <vector>

// Toate efectele sonore ale jocului (fisierele din assets/audio/, generate
// procedural de tools/generate_audio.py).
enum class Sfx {
    SHOOT_LASER,   // Antivirus trage
    SHOOT_TICK,    // Adblocker trage
    HIT,           // impact pe inamic
    KILL,          // inamic ucis
    LEAK,          // inamic a ajuns la baza (alarma)
    WAVE_START,
    WAVE_END,
    BUY,           // cumparare in shop / craft
    ERROR,         // actiune esuata (bani insuficienti, plasare invalida)
    PLACE,         // turn plasat / click UI
    SELL,
    TOKEN,         // token aplicat pe turn
    COIN,          // moneda de income ajunsa la contor (ding)
    JINGLE_WIN,
    JINGLE_LOSE,
    COUNT
};

// Incarca o singura data toate buffer-ele si reda prin un pool de voci.
// Lipsa unui fisier nu e fatala: sunetul respectiv pur si simplu nu se aude.
class AudioManager {
public:
    AudioManager();

    // volume in 0..100, pitch 1.0 = normal. Sunetele de gameplay frecvente
    // primesc automat un jitter mic de pitch (+-6%): la 4x speed urechea
    // percepe lupta, nu o mitraliera monotona. Jinglurile/alarmele raman fixe.
    // Throttle intern: acelasi sunet nu porneste de doua ori in <40ms.
    void play(Sfx id, float volume = 70.0f, float pitch = 1.0f);

    void startMusic();
    // cppcheck-suppress unusedFunction
    void stopMusic();

private:
    static constexpr size_t SFX_COUNT  = static_cast<size_t>(Sfx::COUNT);
    static constexpr size_t VOICE_POOL = 24;

    std::array<sf::SoundBuffer, SFX_COUNT> buffers_;
    std::array<bool, SFX_COUNT>            loaded_{};
    std::array<sf::Clock, SFX_COUNT>       last_play_;
    std::vector<sf::Sound>                 voices_;

    std::mt19937 rng_{std::random_device{}()};

    sf::Music music_;
    bool music_ok_ = false;
};
