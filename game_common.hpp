#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <algorithm>
#include <sstream>
#include <functional>
#include <iomanip>
#include <map>
#include <fstream>
#include <string>
#include <utility>

const int WIDTH = 1300;
const int HEIGHT = 730;
const float GRAVITY = 1200.0f;
const float PI = 3.14159265f;
const float GUN_DURATION = 8.0f;
const float BULLET_SPEED = 900.0f;
const float BULLET_LIFETIME = 1.2f;
const float GUN_COOLDOWN = 0.12f;
const float GUN_CHARGE_MAX = 1.0f;
const float SLASH_DURATION = 0.18f;
const float SLASH_COOLDOWN = 0.32f;
const float SLASH_RANGE = 85.0f;
const float SLASH_HEIGHT = 60.0f;
const float SOUL_MAX = 99.0f;
const float SOUL_PER_HIT = 12.0f;
const float FOCUS_COST = 33.0f;
const float FOCUS_TIME = 0.75f;
const float WALL_SLIDE_SPEED = 240.0f;
const float WALL_JUMP_H = -640.0f;
const float WALL_JUMP_X = 420.0f;
const float AIR_GRAVITY_MULT = 1.12f;
const int MAX_LIVES = 9;
extern float QUALITY_MULT;
extern int QUALITY_LEVEL;
extern bool HARDCORE_MODE;
const float POGO_BOUNCE = -520.0f;
const float BENCH_PROMPT_RANGE = 48.0f;
const float COYOTE_TIME = 0.12f;
const float JUMP_BUFFER_TIME = 0.12f;
const float MAX_FALL_SPEED = 980.0f;
extern const char *WINDOW_TITLE;
extern bool DIE_AGAIN_MODE;
extern bool ADAPTIVE_DIRECTOR;
extern int TRAP_DENSITY_LEVEL; // 0: sparse, 1: balanced, 2: dense
extern int SPIKE_TEMPO_LEVEL;  // 0: calm, 1: standard, 2: savage
extern int OPEN_FLOOR_MODE;    // 0: off, 1: standard, 2: die-again

static void applyLetterboxView(sf::View &view, unsigned int windowWidth, unsigned int windowHeight)
{
    if (windowWidth == 0 || windowHeight == 0)
    {
        return;
    }

    float windowRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    float viewRatio = view.getSize().x / view.getSize().y;
    float sizeX = 1.0f;
    float sizeY = 1.0f;
    float posX = 0.0f;
    float posY = 0.0f;

    if (windowRatio > viewRatio)
    {
        sizeX = viewRatio / windowRatio;
        posX = (1.0f - sizeX) * 0.5f;
    }
    else if (windowRatio < viewRatio)
    {
        sizeY = windowRatio / viewRatio;
        posY = (1.0f - sizeY) * 0.5f;
    }

    view.setViewport(sf::FloatRect(posX, posY, sizeX, sizeY));
}

struct ExternalSettings
{
    int frameLimit = 60;
    int qualityIndex = 1;
    int masterVolume = 85;
    int screenShake = 1;
    int characterStyle = 0;
    int hardcoreMode = 0;
    int dieAgainMode = 1;
    int adaptiveDirector = 1;
    int trapDensity = 1;
    int spikeTempo = 1;
    int openFloorMode = 1;
};

static int clampInt(int v, int lo, int hi)
{
    return std::max(lo, std::min(hi, v));
}

static bool parseIntValue(const std::string &text, int &value)
{
    std::istringstream input(text);
    int parsed = 0;
    input >> std::ws;
    if (!(input >> parsed))
        return false;
    input >> std::ws;
    if (!input.eof())
        return false;
    value = parsed;
    return true;
}

static bool parseFloatValue(const std::string &text, float &value)
{
    std::istringstream input(text);
    float parsed = 0.0f;
    input >> std::ws;
    if (!(input >> parsed))
        return false;
    input >> std::ws;
    if (!input.eof() || !std::isfinite(parsed))
        return false;
    value = parsed;
    return true;
}

static std::string wrapText(const std::string &text, std::size_t maxLineLength)
{
    if (maxLineLength == 0)
        return text;

    std::istringstream words(text);
    std::string wrapped;
    std::string line;
    std::string word;

    while (words >> word)
    {
        if (line.empty())
        {
            line = word;
            continue;
        }

        if (line.size() + 1 + word.size() <= maxLineLength)
        {
            line += " " + word;
        }
        else
        {
            if (!wrapped.empty())
                wrapped += "\n";
            wrapped += line;
            line = word;
        }
    }

    if (!line.empty())
    {
        if (!wrapped.empty())
            wrapped += "\n";
        wrapped += line;
    }

    return wrapped;
}

static bool loadFontFromCommonLocations(sf::Font &font)
{
    const std::vector<std::string> paths = {
        "assets/fonts/DejaVuSans-Bold.ttf",
        "assets/fonts/arial.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/arialbd.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf"};

    for (const std::string &path : paths)
    {
        if (font.loadFromFile(path))
            return true;
    }
    return false;
}

static ExternalSettings loadExternalSettings(const std::string &path)
{
    ExternalSettings s;
    std::ifstream in(path);
    if (!in)
        return s;

    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        std::size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;

        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        int num = 0;
        if (!parseIntValue(value, num))
            continue;

        if (key == "frame_limit")
            s.frameLimit = num;
        else if (key == "quality_index")
            s.qualityIndex = num;
        else if (key == "master_volume")
            s.masterVolume = num;
        else if (key == "screen_shake")
            s.screenShake = num;
        else if (key == "character_style")
            s.characterStyle = num;
        else if (key == "hardcore_mode")
            s.hardcoreMode = num;
        else if (key == "die_again_mode")
            s.dieAgainMode = num;
        else if (key == "adaptive_director")
            s.adaptiveDirector = num;
        else if (key == "trap_density")
            s.trapDensity = num;
        else if (key == "spike_tempo")
            s.spikeTempo = num;
        else if (key == "open_floor_mode")
            s.openFloorMode = num;
    }

    s.qualityIndex = clampInt(s.qualityIndex, 0, 3);
    s.masterVolume = clampInt(s.masterVolume, 0, 100);
    s.screenShake = s.screenShake ? 1 : 0;
    s.characterStyle = clampInt(s.characterStyle, 0, 2);
    s.hardcoreMode = s.hardcoreMode ? 1 : 0;
    s.dieAgainMode = s.dieAgainMode ? 1 : 0;
    s.adaptiveDirector = s.adaptiveDirector ? 1 : 0;
    s.trapDensity = clampInt(s.trapDensity, 0, 2);
    s.spikeTempo = clampInt(s.spikeTempo, 0, 2);
    s.openFloorMode = clampInt(s.openFloorMode, 0, 2);
    return s;
}

static void saveExternalSettings(const std::string &path, const ExternalSettings &s)
{
    std::ofstream out(path, std::ios::trunc);
    if (!out)
        return;

    out << "# Eclipse Knight external settings\n";
    out << "# Performance\n";
    out << "frame_limit=" << s.frameLimit << "\n";
    out << "quality_index=" << s.qualityIndex << "\n";
    out << "master_volume=" << s.masterVolume << "\n";
    out << "\n# Gameplay\n";
    out << "screen_shake=" << s.screenShake << "\n";
    out << "character_style=" << s.characterStyle << "\n";
    out << "hardcore_mode=" << s.hardcoreMode << "\n";
    out << "die_again_mode=" << s.dieAgainMode << "\n";
    out << "adaptive_director=" << s.adaptiveDirector << "\n";
    out << "\n# Traps\n";
    out << "trap_density=" << s.trapDensity << "\n";
    out << "spike_tempo=" << s.spikeTempo << "\n";
    out << "open_floor_mode=" << s.openFloorMode << "\n";
}

struct SavedGameState
{
    int levelNumber = 1;
    int lives = 3;
    int coins = 0;
    int score = 0;
    int deathCount = 0;
    float runTimer = 0.0f;
    int hasGun = 0;
    float gunTimer = 0.0f;
    float soul = 0.0f;
    int equippedWeaponIndex = 0;
    int weaponLoadoutLevel = -1;
};

static bool loadSavedGameState(const std::string &path, SavedGameState &s)
{
    std::ifstream in(path);
    if (!in)
        return false;

    SavedGameState loaded;
    bool hasAnyField = false;
    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        std::size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;

        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        if (key == "run_time" || key == "gun_timer" || key == "soul")
        {
            float num = 0.0f;
            if (!parseFloatValue(value, num))
                continue;
            if (key == "run_time")
                loaded.runTimer = num;
            else if (key == "gun_timer")
                loaded.gunTimer = num;
            else
                loaded.soul = num;
        }
        else
        {
            int num = 0;
            if (!parseIntValue(value, num))
                continue;

            if (key == "level")
                loaded.levelNumber = num;
            else if (key == "lives")
                loaded.lives = num;
            else if (key == "coins")
                loaded.coins = num;
            else if (key == "score")
                loaded.score = num;
            else if (key == "deaths")
                loaded.deathCount = num;
            else if (key == "has_gun")
                loaded.hasGun = num;
            else if (key == "weapon_index")
                loaded.equippedWeaponIndex = num;
            else if (key == "weapon_level")
                loaded.weaponLoadoutLevel = num;
            else
                continue;
        }

        hasAnyField = true;
    }

    if (!hasAnyField)
        return false;

    loaded.levelNumber = std::max(1, loaded.levelNumber);
    loaded.lives = clampInt(loaded.lives, 1, MAX_LIVES);
    loaded.coins = std::max(0, loaded.coins);
    loaded.score = std::max(0, loaded.score);
    loaded.deathCount = std::max(0, loaded.deathCount);
    loaded.runTimer = std::max(0.0f, loaded.runTimer);
    loaded.hasGun = loaded.hasGun ? 1 : 0;
    loaded.gunTimer = std::max(0.0f, loaded.gunTimer);
    loaded.soul = std::max(0.0f, std::min(SOUL_MAX, loaded.soul));
    loaded.equippedWeaponIndex = std::max(0, loaded.equippedWeaponIndex);
    loaded.weaponLoadoutLevel = std::max(-1, loaded.weaponLoadoutLevel);

    s = loaded;
    return true;
}

static bool saveSavedGameState(const std::string &path, const SavedGameState &s)
{
    std::ofstream out(path, std::ios::trunc);
    if (!out)
        return false;

    out << "# Eclipse Knight run save\n";
    out << "level=" << s.levelNumber << "\n";
    out << "lives=" << s.lives << "\n";
    out << "coins=" << s.coins << "\n";
    out << "score=" << s.score << "\n";
    out << "deaths=" << s.deathCount << "\n";
    out << "run_time=" << s.runTimer << "\n";
    out << "has_gun=" << s.hasGun << "\n";
    out << "gun_timer=" << s.gunTimer << "\n";
    out << "soul=" << s.soul << "\n";
    out << "weapon_index=" << s.equippedWeaponIndex << "\n";
    out << "weapon_level=" << s.weaponLoadoutLevel << "\n";
    out.flush();
    return out.good();
}
////////////////////////////////////////////////////////////////////////////
// Procedural Sound Generator
