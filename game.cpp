#include "game_common.hpp"
#include "game_effects.hpp"
#include "game_platform.hpp"
#include "game_collectibles.hpp"
#include "game_weapons.hpp"
#include "game_player.hpp"
#include "game_enemies.hpp"
#include "game_traps.hpp"
#include "game_level.hpp"
#include "game_background.hpp"

// Shared mutable settings are defined once here; the game components live in
// the topic-specific headers above.
float QUALITY_MULT = 1.0f;
int QUALITY_LEVEL = 1;
bool HARDCORE_MODE = false;
const char *WINDOW_TITLE = "ECLIPSE KNIGHT - Global Edition";
bool DIE_AGAIN_MODE = true;
bool ADAPTIVE_DIRECTOR = true;
int TRAP_DENSITY_LEVEL = 1;
int SPIKE_TEMPO_LEVEL = 1;
int OPEN_FLOOR_MODE = 1;

int main()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), WINDOW_TITLE, sf::Style::Default);
    window.setVerticalSyncEnabled(false);
    window.setFramerateLimit(60);
    sf::View worldView(sf::FloatRect(0, 0, WIDTH, HEIGHT));
    applyLetterboxView(worldView, window.getSize().x, window.getSize().y);
    window.setView(worldView);

    sf::Font font;
    loadFontFromCommonLocations(font);

    SoundGen sound; // Sound system!
    Player player;
    ParticleSystem particles;
    ParallaxBackground background;
    std::unique_ptr<Level> currentLevel = std::make_unique<Level>(1);
    std::vector<Bullet> bullets;
    Shade shade;

    sf::Clock clock;
    bool gameOver = false;
    bool levelComplete = false;
    float levelCompleteTimer = 0;
    int highScore = 0;
    float runTimer = 0.0f;
    int deathCount = 0;
    std::vector<Popup> popups;
    auto pushPopup = [&](const sf::Vector2f &pos, const std::string &txt, sf::Color c)
    {
        popups.push_back({pos, sf::Vector2f(0, -70), 1.1f, 1.1f, txt, c});
    };
    float shakeTime = 0.0f, shakeDuration = 0.0f, shakeMagnitude = 0.0f;
    auto triggerShake = [&](float dur, float mag)
    {
        shakeDuration = dur;
        shakeTime = dur;
        shakeMagnitude = std::max(shakeMagnitude, mag);
    };
    auto clamp01 = [](float v) -> float
    {
        return std::max(0.0f, std::min(1.0f, v));
    };
    int levelStartScore = player.score;
    int levelStartCoins = player.coins;
    int levelStartDeaths = deathCount;
    float levelStartRunTime = runTimer;
    int levelStartEnemyCount = currentLevel->enemyCount();
    int levelStartRelicCount = currentLevel->totalRelics();
    int levelHitsTaken = 0;
    float levelBestCombo = 1.0f;
    bool levelSecondWindUsed = false;
    float levelReportTime = 0.0f;
    int levelReportDeaths = 0;
    int levelReportHits = 0;
    int levelReportScoreGain = 0;
    int levelReportCoinsGain = 0;
    float levelReportBestCombo = 1.0f;
    float levelReportPace = 0.0f;
    float levelReportSurvival = 0.0f;
    float levelReportPrecision = 0.0f;
    float levelReportStyle = 0.0f;
    float levelReportTotal = 0.0f;
    std::string levelReportGrade = "B";
    std::string levelReportSummary = "Push pace and stay clean for A/S rank.";
    auto resetLevelSessionStats = [&]()
    {
        levelStartScore = player.score;
        levelStartCoins = player.coins;
        levelStartDeaths = deathCount;
        levelStartRunTime = runTimer;
        levelStartEnemyCount = currentLevel->enemyCount();
        levelStartRelicCount = currentLevel->totalRelics();
        levelHitsTaken = 0;
        levelBestCombo = std::max(1.0f, player.comboMultiplier);
        levelSecondWindUsed = false;
        levelReportTime = 0.0f;
        levelReportDeaths = 0;
        levelReportHits = 0;
        levelReportScoreGain = 0;
        levelReportCoinsGain = 0;
        levelReportBestCombo = 1.0f;
        levelReportPace = 0.0f;
        levelReportSurvival = 0.0f;
        levelReportPrecision = 0.0f;
        levelReportStyle = 0.0f;
        levelReportTotal = 0.0f;
        levelReportGrade = "B";
        levelReportSummary = "Push pace and stay clean for A/S rank.";
    };
    auto finalizeLevelReport = [&]()
    {
        float levelElapsed = std::max(1.0f, runTimer - levelStartRunTime);
        int deathsInLevel = std::max(0, deathCount - levelStartDeaths);
        int hitsInLevel = std::max(0, levelHitsTaken);
        int scoreGain = std::max(0, player.score - levelStartScore);
        int coinsGain = std::max(0, player.coins - levelStartCoins);

        float paceTarget = 55.0f + currentLevel->levelNumber * 12.0f;
        if (currentLevel->levelNumber == 2)
            paceTarget += 15.0f;
        if (HARDCORE_MODE)
            paceTarget += 10.0f;
        if (DIE_AGAIN_MODE)
            paceTarget += 12.0f;

        float precisionBudget = 6.0f + currentLevel->levelNumber * 1.4f;
        float scoreTarget = 1800.0f + currentLevel->levelNumber * 700.0f;
        float comboTarget = 1.8f + currentLevel->levelNumber * 0.16f;

        float paceScore = clamp01(paceTarget / levelElapsed);
        float survivalScore = clamp01(1.0f - static_cast<float>(deathsInLevel) / 3.0f);
        float precisionScore = clamp01(1.0f - static_cast<float>(hitsInLevel) / precisionBudget);
        float comboScore = clamp01((levelBestCombo - 1.0f) / std::max(0.85f, comboTarget - 1.0f));
        float scoreScore = clamp01(static_cast<float>(scoreGain) / scoreTarget + static_cast<float>(coinsGain) * 0.015f);
        float styleScore = clamp01(comboScore * 0.66f + scoreScore * 0.34f);

        float totalScore = paceScore * 0.30f + survivalScore * 0.30f + precisionScore * 0.20f + styleScore * 0.20f;
        if (levelSecondWindUsed)
            totalScore = std::max(0.0f, totalScore - 0.04f);

        levelReportTime = levelElapsed;
        levelReportDeaths = deathsInLevel;
        levelReportHits = hitsInLevel;
        levelReportScoreGain = scoreGain;
        levelReportCoinsGain = coinsGain;
        levelReportBestCombo = levelBestCombo;
        levelReportPace = paceScore;
        levelReportSurvival = survivalScore;
        levelReportPrecision = precisionScore;
        levelReportStyle = styleScore;
        levelReportTotal = totalScore;

        if (totalScore >= 0.93f)
            levelReportGrade = "S";
        else if (totalScore >= 0.84f)
            levelReportGrade = "A";
        else if (totalScore >= 0.72f)
            levelReportGrade = "B";
        else if (totalScore >= 0.58f)
            levelReportGrade = "C";
        else
            levelReportGrade = "D";

        if (levelReportGrade == "S")
            levelReportSummary = "Perfect tempo. Elite execution across the whole stage.";
        else if (levelReportGrade == "A")
            levelReportSummary = "Excellent clear. Tight pace with strong control.";
        else if (levelReportGrade == "B")
            levelReportSummary = "Solid performance. Keep pressure and clean up damage.";
        else if (levelReportGrade == "C")
            levelReportSummary = "Clear secured. Improve survival and route efficiency.";
        else
            levelReportSummary = "Stage survived. Slow down and reset your decision making.";
    };
    std::vector<WeaponLoadout> level2Weapons = {
        {"Balanced Carbine", "All-rounder", "Stable recoil and easy charge control", 3, 30.0f, 1, 0.0f, 1, 2, 2, 0, 1, 1.0f, 1.0f, 1.0f, 0.12f, 1.0f, sf::Color(255, 190, 90), sf::Color(120, 220, 255)},
        {"Storm SMG", "Fast spray", "Ultra-fast fire rhythm for combo chains", 4, 36.0f, 3, 42.0f, 1, 1, 2, 0, 0, 1.08f, 0.9f, 0.9f, 0.07f, 0.78f, sf::Color(170, 235, 255), sf::Color(120, 215, 255)},
        {"Titan Breaker", "Heavy cannon", "Massive stagger and anti-boss pressure", 1, 0.0f, 1, 0.0f, 3, 4, 3, 1, 2, 0.78f, 1.28f, 1.34f, 0.26f, 1.62f, sf::Color(255, 170, 110), sf::Color(255, 110, 70)},
        {"Needle Rail", "Long range pierce", "High velocity bolts with deep penetration", 1, 0.0f, 1, 0.0f, 2, 3, 2, 2, 2, 1.48f, 1.35f, 0.8f, 0.19f, 1.12f, sf::Color(170, 255, 220), sf::Color(120, 255, 235)},
        {"Scatter Fang", "Close shotgun", "Wide spread burst for aggressive pushes", 6, 54.0f, 8, 66.0f, 1, 2, 1, 0, 0, 0.92f, 0.8f, 0.95f, 0.15f, 0.92f, sf::Color(255, 215, 145), sf::Color(255, 175, 120)}};
    int equippedWeaponIndex = 0;
    int weaponSelectIndex = 0;
    int weaponLoadoutLevel = -1;
    bool weaponSelectActive = false;
    float weaponBannerTimer = 0.0f;
    std::string weaponBannerText;
    float shotgunOrbitAngle = 0.0f;
    auto activeWeapon = [&]() -> const WeaponLoadout &
    {
        return level2Weapons[equippedWeaponIndex];
    };
    auto isOrbitShotgun = [&](const WeaponLoadout &w) -> bool
    {
        return w.name == "Scatter Fang";
    };
    auto levelHasGiantEnemy = [&]() -> bool
    {
        for (const auto &enemy : currentLevel->enemies)
        {
            if (!enemy->alive)
                continue;
            if (enemy->type == 3 || enemy->type == 4)
                return true;
        }
        return false;
    };
    auto recommendedWeaponIndex = [&]() -> int
    {
        Enemy *boss = currentLevel->activeBoss();
        if (!boss)
            return equippedWeaponIndex;
        if (boss->type == 4)
        {
            if (boss->health > boss->baseHealth * 0.6f)
                return 2; // Titan Breaker
            if (DIE_AGAIN_MODE)
                return 3; // Needle Rail
            if (deathCount >= 4)
                return 0; // Balanced Carbine
            if (player.comboMultiplier > 2.6f)
                return 1; // Storm SMG
            return 4;     // Scatter Fang
        }
        if (boss->health > boss->baseHealth * 0.55f)
            return 2; // Titan Breaker
        if (deathCount >= 4)
            return 0; // Balanced Carbine
        if (player.comboMultiplier > 2.6f)
            return 1; // Storm SMG
        if (DIE_AGAIN_MODE)
            return 4; // Scatter Fang
        return 3;     // Needle Rail
    };
    auto activateWeaponSelectIfNeeded = [&]()
    {
        if (!gameOver && !levelComplete && levelHasGiantEnemy() && weaponLoadoutLevel != currentLevel->levelNumber)
        {
            weaponSelectActive = true;
            weaponSelectIndex = equippedWeaponIndex;
            player.gunCharging = false;
            player.gunCharge = 0.0f;
        }
    };
    auto equipWeapon = [&](int idx, bool announce)
    {
        equippedWeaponIndex = clampInt(idx, 0, static_cast<int>(level2Weapons.size()) - 1);
        const WeaponLoadout &w = activeWeapon();
        weaponLoadoutLevel = currentLevel->levelNumber;
        weaponSelectActive = false;
        player.hasGun = true;
        player.gunTimer = std::max(player.gunTimer, 180.0f);
        player.gunCooldown = 0.0f;
        player.gunCharging = false;
        player.gunCharge = 0.0f;
        weaponBannerText = "Equipped: " + w.name;
        weaponBannerTimer = 3.4f;
        if (announce)
        {
            pushPopup(player.position + sf::Vector2f(0, -55), w.name, w.chargeColor);
            particles.addCollectEffect(player.position + sf::Vector2f(0, -18), w.tapColor);
            sound.play("powerup");
        }
    };
    float levelIntroTimer = 3.6f;
    std::string levelIntroHeader;
    std::string levelIntroObjective;
    auto refreshLevelIntro = [&]()
    {
        levelIntroHeader = currentLevel->chapterName();
        levelIntroObjective = currentLevel->objectiveHint();
        levelIntroTimer = 3.6f;
    };
    refreshLevelIntro();
    // --- Main Menu ---
    sf::Texture logoTexture;
    bool logoLoaded = logoTexture.loadFromFile("assets/logo.png");
    sf::Sprite logo(logoTexture);
    if (logoLoaded)
    {
        float scale = 1.0f;
        if (logoTexture.getSize().x > 0)
        {
            scale = std::min(1.0f, 420.0f / static_cast<float>(logoTexture.getSize().x));
        }
        logo.setScale(scale, scale);
        sf::FloatRect lb = logo.getGlobalBounds();
        logo.setPosition(WIDTH / 2 - lb.width / 2, 25);
    }

    std::vector<std::string> menuItems = {"Load Game", "Start Game", "Settings", "Controls", "Exit"};
    int menuIndex = 0;
    bool showMenu = true;
    bool showControls = false;
    bool showSettings = false;
    int settingsIndex = 0;
    int settingsTabIndex = 0;
    std::vector<std::string> settingsTabs = {"Performance", "Gameplay", "Traps", "Audio", "Style", "System"};
    std::vector<int> fpsOptions = {30, 60, 120, 240, 0};
    int fpsIndex = 1;
    std::vector<std::string> qualityLabels = {"Low", "Medium", "High", "Ultra"};
    std::vector<float> qualityValues = {0.70f, 1.0f, 1.35f, 1.75f};
    int qualityIndex = 1;
    int masterVolume = 85;
    bool screenShakeEnabled = true;
    bool hardcoreModeEnabled = false;
    bool dieAgainModeEnabled = true;
    bool adaptiveDirectorEnabled = true;
    std::vector<std::string> trapDensityLabels = {"Sparse", "Balanced", "Dense"};
    std::vector<std::string> spikeTempoLabels = {"Calm", "Standard", "Savage"};
    std::vector<std::string> openFloorLabels = {"Off", "Standard", "Die Again"};
    int trapDensityIndex = 1;
    int spikeTempoIndex = 1;
    int openFloorModeIndex = 1;
    std::vector<std::string> characterStyles = {"Knight", "Shade", "Crimson"};
    int characterStyleIndex = 0;
    const std::string settingsPath = "settings.cfg";
    const std::string savePath = "savegame.cfg";
    sf::Clock menuClock;
    float smoothedFps = 60.0f;
    float smoothedFrameMs = 16.67f;
    std::string settingsBannerText = "Auto-saved in settings.cfg";
    float settingsBannerTimer = 0.0f;
    std::string menuBannerText;
    float menuBannerTimer = 0.0f;
    bool levelIntroPending = false;
    bool backToMenuPrompt = false;
    int backToMenuChoice = 1;
    bool returnToMenuRequested = false;
    auto updatePerfStats = [&](float frameDt)
    {
        float safeDt = std::max(0.00001f, frameDt);
        float instantFps = 1.0f / safeDt;
        float instantMs = safeDt * 1000.0f;
        smoothedFps += (instantFps - smoothedFps) * 0.09f;
        smoothedFrameMs += (instantMs - smoothedFrameMs) * 0.09f;
    };

    ExternalSettings ext = loadExternalSettings(settingsPath);
    auto applyExternalSnapshot = [&]()
    {
        qualityIndex = clampInt(ext.qualityIndex, 0, static_cast<int>(qualityValues.size()) - 1);
        masterVolume = clampInt(ext.masterVolume, 0, 100);
        screenShakeEnabled = (ext.screenShake != 0);
        hardcoreModeEnabled = (ext.hardcoreMode != 0);
        dieAgainModeEnabled = (ext.dieAgainMode != 0);
        adaptiveDirectorEnabled = (ext.adaptiveDirector != 0);
        trapDensityIndex = clampInt(ext.trapDensity, 0, static_cast<int>(trapDensityLabels.size()) - 1);
        spikeTempoIndex = clampInt(ext.spikeTempo, 0, static_cast<int>(spikeTempoLabels.size()) - 1);
        openFloorModeIndex = clampInt(ext.openFloorMode, 0, static_cast<int>(openFloorLabels.size()) - 1);
        characterStyleIndex = clampInt(ext.characterStyle, 0, static_cast<int>(characterStyles.size()) - 1);
        int matchedFpsIndex = 1;
        for (int i = 0; i < static_cast<int>(fpsOptions.size()); i++)
        {
            if (fpsOptions[i] == ext.frameLimit)
            {
                matchedFpsIndex = i;
                break;
            }
        }
        fpsIndex = matchedFpsIndex;
    };
    applyExternalSnapshot();

    auto fpsLabel = [&]() -> std::string
    {
        if (fpsOptions[fpsIndex] == 0)
            return "Unlimited";
        return std::to_string(fpsOptions[fpsIndex]) + " FPS";
    };
    auto fpsStatusColor = [&]() -> sf::Color
    {
        int target = fpsOptions[fpsIndex];
        if (target <= 0)
        {
            if (smoothedFps >= 140.0f)
                return sf::Color(100, 245, 175);
            if (smoothedFps >= 90.0f)
                return sf::Color(255, 220, 120);
            return sf::Color(255, 130, 110);
        }

        float ratio = smoothedFps / static_cast<float>(target);
        if (ratio >= 0.96f)
            return sf::Color(100, 245, 175);
        if (ratio >= 0.82f)
            return sf::Color(255, 220, 120);
        return sf::Color(255, 130, 110);
    };
    auto fpsStatusText = [&]() -> std::string
    {
        int target = fpsOptions[fpsIndex];
        if (target <= 0)
        {
            if (smoothedFps >= 140.0f)
                return "Unlocked: Very Smooth";
            if (smoothedFps >= 90.0f)
                return "Unlocked: Good";
            return "Unlocked: Heavy Scene";
        }

        float ratio = smoothedFps / static_cast<float>(target);
        if (ratio >= 0.96f)
            return "Target Locked";
        if (ratio >= 0.82f)
            return "Near Target";
        return "Below Target";
    };
    auto saveCurrentSettings = [&]()
    {
        ext.frameLimit = fpsOptions[fpsIndex];
        ext.qualityIndex = qualityIndex;
        ext.masterVolume = masterVolume;
        ext.screenShake = screenShakeEnabled ? 1 : 0;
        ext.hardcoreMode = hardcoreModeEnabled ? 1 : 0;
        ext.dieAgainMode = dieAgainModeEnabled ? 1 : 0;
        ext.adaptiveDirector = adaptiveDirectorEnabled ? 1 : 0;
        ext.trapDensity = trapDensityIndex;
        ext.spikeTempo = spikeTempoIndex;
        ext.openFloorMode = openFloorModeIndex;
        ext.characterStyle = characterStyleIndex;
        saveExternalSettings(settingsPath, ext);
    };
    auto applyPerformance = [&]()
    {
        QUALITY_MULT = qualityValues[qualityIndex];
        QUALITY_LEVEL = qualityIndex;
        background.setQuality(QUALITY_LEVEL);
        bool previousHardcore = HARDCORE_MODE;
        bool previousDieAgain = DIE_AGAIN_MODE;
        bool previousDirector = ADAPTIVE_DIRECTOR;
        int previousTrapDensity = TRAP_DENSITY_LEVEL;
        int previousSpikeTempo = SPIKE_TEMPO_LEVEL;
        int previousOpenFloor = OPEN_FLOOR_MODE;
        HARDCORE_MODE = hardcoreModeEnabled;
        DIE_AGAIN_MODE = dieAgainModeEnabled;
        ADAPTIVE_DIRECTOR = adaptiveDirectorEnabled;
        TRAP_DENSITY_LEVEL = trapDensityIndex;
        SPIKE_TEMPO_LEVEL = spikeTempoIndex;
        OPEN_FLOOR_MODE = openFloorModeIndex;
        if (previousHardcore != HARDCORE_MODE || previousDieAgain != DIE_AGAIN_MODE || previousDirector != ADAPTIVE_DIRECTOR ||
            previousTrapDensity != TRAP_DENSITY_LEVEL || previousSpikeTempo != SPIKE_TEMPO_LEVEL || previousOpenFloor != OPEN_FLOOR_MODE)
        {
            currentLevel = std::make_unique<Level>(currentLevel->levelNumber);
            levelIntroPending = true;
            resetLevelSessionStats();
        }
        window.setVerticalSyncEnabled(false);
        window.setFramerateLimit(static_cast<unsigned int>(std::max(0, fpsOptions[fpsIndex])));
        sound.setMasterVolume(static_cast<float>(masterVolume));
        player.setStyle(characterStyleIndex);
        saveCurrentSettings();
    };
    auto loadCurrentSettings = [&]()
    {
        ext = loadExternalSettings(settingsPath);
        applyExternalSnapshot();
        applyPerformance();
    };
    auto rowsForTab = [&](int tab) -> int
    {
        if (tab == 0)
            return 2; // Frame Limit, Quality
        if (tab == 1)
            return 4; // Screen Shake, Hardcore, Die Again, Adaptive Director
        if (tab == 2)
            return 3; // Trap Density, Spike Tempo, Open Floor Mode
        if (tab == 3)
            return 1; // Master Volume
        if (tab == 4)
            return 1; // Character Style
        return 1;     // Load Settings
    };
    auto normalizeSettingsSelection = [&]()
    {
        int rows = rowsForTab(settingsTabIndex);
        settingsIndex = clampInt(settingsIndex, 0, std::max(0, rows - 1));
    };
    auto cycleSettingsTab = [&](int dir)
    {
        int tabCount = static_cast<int>(settingsTabs.size());
        settingsTabIndex = (settingsTabIndex + dir + tabCount) % tabCount;
        normalizeSettingsSelection();
    };
    auto changeSetting = [&](int dir)
    {
        if (settingsTabIndex == 0)
        {
            if (settingsIndex == 0)
            {
                fpsIndex = (fpsIndex + dir + static_cast<int>(fpsOptions.size())) % static_cast<int>(fpsOptions.size());
            }
            else if (settingsIndex == 1)
            {
                qualityIndex = (qualityIndex + dir + static_cast<int>(qualityValues.size())) % static_cast<int>(qualityValues.size());
            }
        }
        else if (settingsTabIndex == 1)
        {
            if (settingsIndex == 0)
                screenShakeEnabled = !screenShakeEnabled;
            else if (settingsIndex == 1)
                hardcoreModeEnabled = !hardcoreModeEnabled;
            else if (settingsIndex == 2)
                dieAgainModeEnabled = !dieAgainModeEnabled;
            else if (settingsIndex == 3)
                adaptiveDirectorEnabled = !adaptiveDirectorEnabled;
        }
        else if (settingsTabIndex == 2)
        {
            if (settingsIndex == 0)
                trapDensityIndex = (trapDensityIndex + dir + static_cast<int>(trapDensityLabels.size())) % static_cast<int>(trapDensityLabels.size());
            else if (settingsIndex == 1)
                spikeTempoIndex = (spikeTempoIndex + dir + static_cast<int>(spikeTempoLabels.size())) % static_cast<int>(spikeTempoLabels.size());
            else if (settingsIndex == 2)
                openFloorModeIndex = (openFloorModeIndex + dir + static_cast<int>(openFloorLabels.size())) % static_cast<int>(openFloorLabels.size());
        }
        else if (settingsTabIndex == 3)
        {
            masterVolume = clampInt(masterVolume + dir * 5, 0, 100);
        }
        else if (settingsTabIndex == 4)
        {
            characterStyleIndex = (characterStyleIndex + dir + static_cast<int>(characterStyles.size())) % static_cast<int>(characterStyles.size());
        }
        else
        {
            return;
        }
        applyPerformance();
    };
    normalizeSettingsSelection();
    applyPerformance();
    resetLevelSessionStats();
    auto resetRunForNewGame = [&]()
    {
        player = Player();
        player.setStyle(characterStyleIndex);
        currentLevel = std::make_unique<Level>(1);
        gameOver = false;
        levelComplete = false;
        levelCompleteTimer = 0.0f;
        runTimer = 0.0f;
        deathCount = 0;
        bullets.clear();
        popups.clear();
        particles.particles.clear();
        shade.active = false;
        shade.value = 0;
        weaponLoadoutLevel = -1;
        weaponSelectActive = false;
        equippedWeaponIndex = 0;
        weaponSelectIndex = 0;
        weaponBannerTimer = 0.0f;
        weaponBannerText.clear();
        shotgunOrbitAngle = 0.0f;
        backToMenuPrompt = false;
        backToMenuChoice = 1;
        returnToMenuRequested = false;
        shakeTime = 0.0f;
        shakeDuration = 0.0f;
        shakeMagnitude = 0.0f;
        levelIntroPending = true;
        levelIntroTimer = 0.0f;
        levelIntroHeader.clear();
        levelIntroObjective.clear();
        resetLevelSessionStats();
    };
    auto saveRunProgress = [&]() -> bool
    {
        SavedGameState s;
        s.levelNumber = currentLevel->levelNumber;
        s.lives = player.lives;
        s.coins = player.coins;
        s.score = player.score;
        s.deathCount = deathCount;
        s.runTimer = runTimer;
        s.hasGun = player.hasGun ? 1 : 0;
        s.gunTimer = player.gunTimer;
        s.soul = player.soul;
        s.equippedWeaponIndex = equippedWeaponIndex;
        s.weaponLoadoutLevel = weaponLoadoutLevel;
        return saveSavedGameState(savePath, s);
    };
    auto loadRunProgress = [&]() -> bool
    {
        SavedGameState s;
        if (!loadSavedGameState(savePath, s))
            return false;

        resetRunForNewGame();
        currentLevel = std::make_unique<Level>(std::max(1, s.levelNumber));
        player.setStyle(characterStyleIndex);
        player.respawn();
        player.lives = clampInt(s.lives, 1, MAX_LIVES);
        player.coins = std::max(0, s.coins);
        player.score = std::max(0, s.score);
        deathCount = std::max(0, s.deathCount);
        runTimer = std::max(0.0f, s.runTimer);
        player.soul = std::max(0.0f, std::min(SOUL_MAX, s.soul));
        equippedWeaponIndex = clampInt(s.equippedWeaponIndex, 0, static_cast<int>(level2Weapons.size()) - 1);
        weaponSelectIndex = equippedWeaponIndex;
        player.hasGun = (s.hasGun != 0);
        player.gunTimer = player.hasGun ? std::max(0.0f, s.gunTimer) : 0.0f;
        player.gunCooldown = 0.0f;
        player.gunCharging = false;
        player.gunCharge = 0.0f;
        weaponLoadoutLevel = player.hasGun ? std::max(currentLevel->levelNumber, s.weaponLoadoutLevel) : -1;
        levelIntroPending = true;
        resetLevelSessionStats();
        return true;
    };
    auto playStartStoryCinematic = [&]() -> bool
    {
        struct StoryScene
        {
            std::string title;
            std::string subtitle;
            float duration;
        };

        std::vector<StoryScene> scenes = {
            {"HOME", "Adam Carter. Federal agent. Home is Lily and Joseph.", 5.0f},
            {"FIELD", "Each night mission drags him further into violence.", 4.8f},
            {"PROMISE", "Lily: \"I fear your job will take who you are.\"", 4.8f},
            {"CHOICE", "Can Adam protect his family and stay human?", 4.2f}};

        float totalDuration = 0.0f;
        for (const auto &s : scenes)
            totalDuration += s.duration;

        auto sceneStartTime = [&](int sceneIndex) -> float
        {
            float t = 0.0f;
            for (int i = 0; i < sceneIndex; i++)
                t += scenes[i].duration;
            return t;
        };
        auto sceneIndexAt = [&](float elapsed) -> int
        {
            float cursor = 0.0f;
            for (int i = 0; i < static_cast<int>(scenes.size()); i++)
            {
                cursor += scenes[i].duration;
                if (elapsed < cursor)
                    return i;
            }
            return static_cast<int>(scenes.size()) - 1;
        };

        auto drawPerson = [&](const sf::Vector2f &feetPos,
                              const sf::Color &outfitBase,
                              const sf::Color &accentBase,
                              bool female,
                              bool child,
                              float walkPhase,
                              bool armed,
                              sf::Uint8 alpha = 255)
        {
            auto clampColor = [](int v) -> sf::Uint8
            {
                return static_cast<sf::Uint8>(std::max(0, std::min(255, v)));
            };
            auto shade = [&](const sf::Color &c, int dr, int dg, int db) -> sf::Color
            {
                return sf::Color(clampColor(static_cast<int>(c.r) + dr),
                                 clampColor(static_cast<int>(c.g) + dg),
                                 clampColor(static_cast<int>(c.b) + db),
                                 alpha);
            };

            float scale = child ? 0.76f : 1.0f;
            float dir = (feetPos.x < WIDTH * 0.5f) ? 1.0f : -1.0f;
            float bob = std::sin(walkPhase * 0.8f) * 1.4f * scale;
            sf::Vector2f base(feetPos.x, feetPos.y + bob * 0.25f);

            sf::Color shell = shade(outfitBase, -8, -8, -8);
            sf::Color cloth = shade(outfitBase, 18, 14, 20);
            sf::Color trim = shade(accentBase, 0, 0, 0);
            sf::Color mask = female ? sf::Color(248, 240, 248, alpha) : sf::Color(242, 247, 255, alpha);
            sf::Color eyeCore = shade(accentBase, 28, 36, 40);
            sf::Color boots = sf::Color(12, 16, 24, alpha);

            sf::CircleShape ground(10.5f * scale);
            ground.setOrigin(ground.getRadius(), ground.getRadius());
            ground.setScale(1.45f, 0.42f);
            ground.setPosition(base.x, base.y + 1.0f * scale);
            ground.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(85 * alpha / 255)));
            window.draw(ground);

            float sway = std::sin(walkPhase * 0.92f) * 3.0f * scale * dir;
            sf::ConvexShape mantle;
            mantle.setPointCount(5);
            mantle.setPoint(0, sf::Vector2f(base.x - 14.0f * scale, base.y - 35.0f * scale));
            mantle.setPoint(1, sf::Vector2f(base.x + 14.0f * scale, base.y - 35.0f * scale));
            mantle.setPoint(2, sf::Vector2f(base.x + 13.5f * scale + sway, base.y - 4.0f * scale));
            mantle.setPoint(3, sf::Vector2f(base.x + sway * 0.18f, base.y + 4.0f * scale));
            mantle.setPoint(4, sf::Vector2f(base.x - 13.5f * scale + sway, base.y - 4.0f * scale));
            mantle.setFillColor(sf::Color(cloth.r, cloth.g, cloth.b, static_cast<sf::Uint8>(220 * alpha / 255)));
            mantle.setOutlineThickness(1.7f);
            mantle.setOutlineColor(trim);
            window.draw(mantle);

            float legOffset = std::sin(walkPhase * 1.3f) * 6.0f * scale;
            sf::RectangleShape legL(sf::Vector2f(6.0f * scale, 19.0f * scale));
            legL.setOrigin(legL.getSize().x * 0.5f, 1.0f * scale);
            legL.setPosition(base.x - 5.8f * scale, base.y - 15.0f * scale);
            legL.setRotation(legOffset * 0.55f);
            legL.setFillColor(boots);
            window.draw(legL);

            sf::RectangleShape legR(sf::Vector2f(6.0f * scale, 19.0f * scale));
            legR.setOrigin(legR.getSize().x * 0.5f, 1.0f * scale);
            legR.setPosition(base.x + 5.8f * scale, base.y - 15.0f * scale);
            legR.setRotation(-legOffset * 0.55f);
            legR.setFillColor(boots);
            window.draw(legR);

            sf::CircleShape torso(9.8f * scale);
            torso.setOrigin(torso.getRadius(), torso.getRadius());
            torso.setScale(1.0f, 1.36f);
            torso.setPosition(base.x, base.y - 20.0f * scale);
            torso.setFillColor(shell);
            torso.setOutlineThickness(2.0f);
            torso.setOutlineColor(sf::Color(158, 186, 226, alpha));
            window.draw(torso);

            sf::ConvexShape crest;
            crest.setPointCount(4);
            crest.setPoint(0, sf::Vector2f(base.x, base.y - 23.0f * scale));
            crest.setPoint(1, sf::Vector2f(base.x + 3.6f * scale, base.y - 16.0f * scale));
            crest.setPoint(2, sf::Vector2f(base.x, base.y - 8.5f * scale));
            crest.setPoint(3, sf::Vector2f(base.x - 3.6f * scale, base.y - 16.0f * scale));
            crest.setFillColor(trim);
            window.draw(crest);

            sf::RectangleShape armL(sf::Vector2f(5.8f * scale, 16.0f * scale));
            armL.setOrigin(armL.getSize().x * 0.5f, 2.0f * scale);
            armL.setPosition(base.x - 11.8f * scale, base.y - 28.0f * scale);
            armL.setRotation(-12.0f + legOffset * 0.35f);
            armL.setFillColor(shell);
            window.draw(armL);

            sf::RectangleShape armR(sf::Vector2f(5.8f * scale, 16.0f * scale));
            armR.setOrigin(armR.getSize().x * 0.5f, 2.0f * scale);
            armR.setPosition(base.x + 11.8f * scale, base.y - 28.0f * scale);
            armR.setRotation(12.0f - legOffset * 0.35f);
            armR.setFillColor(shell);
            window.draw(armR);

            sf::CircleShape head(10.8f * scale);
            head.setOrigin(head.getRadius(), head.getRadius());
            head.setScale(1.0f, 1.14f);
            head.setPosition(base.x, base.y - 42.0f * scale);
            head.setFillColor(mask);
            head.setOutlineThickness(1.8f);
            head.setOutlineColor(sf::Color(150, 178, 214, alpha));
            window.draw(head);

            if (!child)
            {
                float hornH = female ? 10.5f : 14.0f;
                float hornW = female ? 7.0f : 8.5f;

                sf::ConvexShape crown;
                crown.setPointCount(3);
                crown.setPoint(0, sf::Vector2f(-3.0f * scale, -2.0f * scale));
                crown.setPoint(1, sf::Vector2f(-hornW * scale, -hornH * scale));
                crown.setPoint(2, sf::Vector2f(1.5f * scale, -1.0f * scale));
                crown.setPosition(base.x - 2.8f * scale, base.y - 52.0f * scale);
                crown.setFillColor(mask);
                crown.setOutlineThickness(1.5f);
                crown.setOutlineColor(sf::Color(146, 170, 206, alpha));
                window.draw(crown);

                crown.setScale(-1.0f, 1.0f);
                crown.setPosition(base.x + 2.8f * scale, base.y - 52.0f * scale);
                window.draw(crown);
            }

            sf::CircleShape eyeAura(2.3f * scale);
            eyeAura.setOrigin(eyeAura.getRadius(), eyeAura.getRadius());
            eyeAura.setFillColor(sf::Color(eyeCore.r, eyeCore.g, eyeCore.b, static_cast<sf::Uint8>(145 * alpha / 255)));
            eyeAura.setPosition(base.x - 4.0f * scale, base.y - 42.8f * scale);
            window.draw(eyeAura);
            eyeAura.setPosition(base.x + 4.0f * scale, base.y - 42.8f * scale);
            window.draw(eyeAura);

            sf::RectangleShape eye(sf::Vector2f(2.1f * scale, 3.5f * scale));
            eye.setOrigin(eye.getSize() * 0.5f);
            eye.setFillColor(sf::Color(16, 22, 35, alpha));
            eye.setPosition(base.x - 4.0f * scale, base.y - 42.8f * scale);
            window.draw(eye);
            eye.setPosition(base.x + 4.0f * scale, base.y - 42.8f * scale);
            window.draw(eye);

            if (female)
            {
                sf::ConvexShape scarf;
                scarf.setPointCount(4);
                scarf.setPoint(0, sf::Vector2f(base.x + 9.0f * scale, base.y - 30.0f * scale));
                scarf.setPoint(1, sf::Vector2f(base.x + 16.0f * scale, base.y - 22.0f * scale));
                scarf.setPoint(2, sf::Vector2f(base.x + 10.0f * scale, base.y - 14.0f * scale));
                scarf.setPoint(3, sf::Vector2f(base.x + 6.0f * scale, base.y - 21.0f * scale));
                scarf.setFillColor(sf::Color(trim.r, trim.g, trim.b, static_cast<sf::Uint8>(200 * alpha / 255)));
                window.draw(scarf);
            }

            sf::RectangleShape bootL(sf::Vector2f(8.0f * scale, 4.0f * scale));
            bootL.setOrigin(bootL.getSize() * 0.5f);
            bootL.setFillColor(boots);
            bootL.setPosition(base.x - 6.0f * scale, base.y + 0.2f * scale);
            window.draw(bootL);
            sf::RectangleShape bootR(sf::Vector2f(8.0f * scale, 4.0f * scale));
            bootR.setOrigin(bootR.getSize() * 0.5f);
            bootR.setFillColor(boots);
            bootR.setPosition(base.x + 6.0f * scale, base.y + 0.2f * scale);
            window.draw(bootR);

            if (armed && !child)
            {
                float weaponAngle = dir > 0 ? -16.0f : 196.0f;
                sf::Vector2f handPos(base.x + dir * (11.0f * scale), base.y - 26.0f * scale);

                sf::RectangleShape grip(sf::Vector2f(9.0f * scale, 2.0f * scale));
                grip.setOrigin(1.0f * scale, 1.0f * scale);
                grip.setPosition(handPos);
                grip.setRotation(weaponAngle);
                grip.setFillColor(sf::Color(76, 102, 140, alpha));
                window.draw(grip);

                sf::ConvexShape guard;
                guard.setPointCount(4);
                guard.setPoint(0, sf::Vector2f(0.0f, -1.8f * scale));
                guard.setPoint(1, sf::Vector2f(2.6f * scale, 0.0f));
                guard.setPoint(2, sf::Vector2f(0.0f, 1.8f * scale));
                guard.setPoint(3, sf::Vector2f(-2.6f * scale, 0.0f));
                guard.setFillColor(trim);
                guard.setPosition(handPos + sf::Vector2f(dir * (3.0f * scale), -0.4f * scale));
                guard.setRotation(weaponAngle);
                window.draw(guard);

                sf::RectangleShape blade(sf::Vector2f(23.0f * scale, 1.8f * scale));
                blade.setOrigin(0.6f * scale, 0.9f * scale);
                blade.setPosition(handPos + sf::Vector2f(dir * (4.0f * scale), -0.4f * scale));
                blade.setRotation(weaponAngle);
                blade.setFillColor(sf::Color(224, 235, 248, alpha));
                blade.setOutlineThickness(0.9f);
                blade.setOutlineColor(sf::Color(122, 152, 194, alpha));
                window.draw(blade);
            }
        };

        auto drawSpeechBubble = [&](const std::string &text, const sf::Vector2f &pos, sf::Uint8 alpha)
        {
            sf::Text say;
            say.setFont(font);
            say.setCharacterSize(18);
            say.setLineSpacing(1.2f);
            say.setFillColor(sf::Color(28, 34, 48, alpha));
            say.setString(wrapText(text, 36));

            sf::FloatRect sb = say.getLocalBounds();
            sf::RectangleShape bubble(sf::Vector2f(sb.width + 26.0f, sb.height + 24.0f));
            bubble.setOrigin(bubble.getSize().x * 0.5f, bubble.getSize().y * 0.5f);
            bubble.setPosition(pos);
            bubble.setFillColor(sf::Color(238, 244, 252, alpha));
            bubble.setOutlineThickness(2.0f);
            bubble.setOutlineColor(sf::Color(170, 195, 220, alpha));
            window.draw(bubble);

            sf::ConvexShape arrow;
            arrow.setPointCount(3);
            arrow.setPoint(0, sf::Vector2f(0, 0));
            arrow.setPoint(1, sf::Vector2f(12, 20));
            arrow.setPoint(2, sf::Vector2f(-5, 14));
            arrow.setPosition(pos.x - 48.0f, pos.y + bubble.getSize().y * 0.48f);
            arrow.setFillColor(sf::Color(238, 244, 252, alpha));
            window.draw(arrow);

            say.setPosition(pos.x - sb.width * 0.5f, pos.y - sb.height * 0.5f - 6.0f);
            window.draw(say);
        };
        auto drawSceneHeader = [&](const std::string &title, sf::Uint8 alpha)
        {
            sf::RectangleShape badge(sf::Vector2f(186.0f, 42.0f));
            badge.setPosition(22.0f, 16.0f);
            badge.setFillColor(sf::Color(8, 18, 34, static_cast<sf::Uint8>(140 * alpha / 255)));
            badge.setOutlineThickness(2.0f);
            badge.setOutlineColor(sf::Color(112, 168, 236, alpha));
            window.draw(badge);

            sf::Text titleTxt;
            titleTxt.setFont(font);
            titleTxt.setCharacterSize(24);
            titleTxt.setStyle(sf::Text::Bold);
            titleTxt.setFillColor(sf::Color(218, 234, 252, alpha));
            titleTxt.setString(title);
            titleTxt.setPosition(42.0f, 20.0f);
            window.draw(titleTxt);
        };

        sf::Clock storyClock;
        sf::Clock frameClock;
        int lastSceneIndex = -1;

        while (window.isOpen())
        {
            float dtIntro = frameClock.restart().asSeconds();
            float elapsed = storyClock.getElapsedTime().asSeconds();
            background.update(dtIntro * 0.30f);

            sf::Event event;
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                {
                    window.close();
                    return false;
                }
                if (event.type == sf::Event::Resized)
                {
                    applyLetterboxView(worldView,
                                       static_cast<unsigned int>(event.size.width),
                                       static_cast<unsigned int>(event.size.height));
                    window.setView(worldView);
                }
                if (event.type == sf::Event::KeyPressed && elapsed > 0.20f)
                {
                    if (event.key.code == sf::Keyboard::Enter ||
                        event.key.code == sf::Keyboard::Space ||
                        event.key.code == sf::Keyboard::Escape)
                    {
                        return true;
                    }
                }
            }

            if (elapsed >= totalDuration)
                return true;

            int sceneIndex = sceneIndexAt(elapsed);
            if (sceneIndex != lastSceneIndex)
            {
                lastSceneIndex = sceneIndex;
                if (sceneIndex == 1)
                    sound.play("dash");
                else if (sceneIndex == 2)
                    sound.play("hit");
                else if (sceneIndex == 3)
                    sound.play("powerup");
                else
                    sound.play("coin");
            }

            float sceneStart = sceneStartTime(sceneIndex);
            float localT = elapsed - sceneStart;
            float sceneDuration = scenes[sceneIndex].duration;
            float fadeIn = clamp01(localT / 0.50f);
            float fadeOut = clamp01((sceneDuration - localT) / 0.65f);
            float sceneAlphaRatio = std::max(0.0f, std::min(fadeIn, fadeOut));
            sf::Uint8 sceneAlpha = static_cast<sf::Uint8>(95 + sceneAlphaRatio * 160.0f);

            const float groundY = HEIGHT * 0.79f;
            window.clear(sf::Color(8, 12, 24));
            background.draw(window);

            float camX = 0.0f;
            float camY = 0.0f;
            if (sceneIndex == 0)
            {
                camX = std::sin(localT * 0.75f) * 8.0f;
                camY = std::sin(localT * 1.12f) * 2.7f;
            }
            else if (sceneIndex == 1)
            {
                camX = ((localT / std::max(0.1f, sceneDuration)) - 0.5f) * 44.0f + std::sin(localT * 5.2f) * 2.4f;
                camY = std::sin(localT * 5.8f) * 1.8f;
            }
            else if (sceneIndex == 2)
            {
                camX = std::sin(localT * 3.5f) * 6.2f;
                camY = std::cos(localT * 4.7f) * 1.8f;
            }
            else
            {
                camX = std::sin(localT * 1.7f) * 7.2f;
                camY = std::cos(localT * 2.2f) * 2.6f;
            }
            sf::View cinematicView = worldView;
            cinematicView.move(camX, camY);
            window.setView(cinematicView);

            if (sceneIndex == 0)
            {
                sf::VertexArray sky(sf::Quads, 4);
                sky[0].position = sf::Vector2f(0, 0);
                sky[1].position = sf::Vector2f(WIDTH, 0);
                sky[2].position = sf::Vector2f(WIDTH, HEIGHT);
                sky[3].position = sf::Vector2f(0, HEIGHT);
                sky[0].color = sf::Color(62, 94, 145);
                sky[1].color = sf::Color(88, 120, 165);
                sky[2].color = sf::Color(32, 52, 86);
                sky[3].color = sf::Color(26, 42, 74);
                window.draw(sky);

                sf::CircleShape sun(42);
                sun.setPosition(110, 70);
                sun.setFillColor(sf::Color(255, 220, 160, 210));
                window.draw(sun);

                for (int i = 0; i < 5; i++)
                {
                    float cloudX = std::fmod(140.0f * i + localT * 24.0f, WIDTH + 260.0f) - 130.0f;
                    sf::CircleShape c1(22.0f + (i % 2) * 6.0f);
                    c1.setFillColor(sf::Color(214, 226, 246, 75));
                    c1.setPosition(cloudX, 92.0f + (i % 3) * 26.0f);
                    window.draw(c1);
                    sf::CircleShape c2(c1.getRadius() * 0.85f);
                    c2.setFillColor(sf::Color(214, 226, 246, 60));
                    c2.setPosition(cloudX + 26.0f, 98.0f + (i % 3) * 26.0f);
                    window.draw(c2);
                }

                sf::RectangleShape house(sf::Vector2f(280, 200));
                house.setPosition(WIDTH * 0.60f, groundY - 200.0f);
                house.setFillColor(sf::Color(74, 88, 112));
                house.setOutlineThickness(3.0f);
                house.setOutlineColor(sf::Color(195, 210, 230));
                window.draw(house);

                sf::ConvexShape roof;
                roof.setPointCount(3);
                roof.setPoint(0, sf::Vector2f(WIDTH * 0.57f, groundY - 200.0f));
                roof.setPoint(1, sf::Vector2f(WIDTH * 0.74f, groundY - 298.0f));
                roof.setPoint(2, sf::Vector2f(WIDTH * 0.91f, groundY - 200.0f));
                roof.setFillColor(sf::Color(52, 64, 90));
                window.draw(roof);

                sf::RectangleShape winA(sf::Vector2f(42, 54));
                winA.setPosition(WIDTH * 0.655f, groundY - 150.0f);
                winA.setFillColor(sf::Color(245, 215, 138, 190));
                window.draw(winA);
                sf::RectangleShape winB(sf::Vector2f(42, 54));
                winB.setPosition(WIDTH * 0.77f, groundY - 150.0f);
                winB.setFillColor(sf::Color(245, 215, 138, 175));
                window.draw(winB);

                sf::RectangleShape lawn(sf::Vector2f(WIDTH, HEIGHT - groundY));
                lawn.setPosition(0, groundY);
                lawn.setFillColor(sf::Color(42, 95, 66));
                window.draw(lawn);

                float walk = localT * 3.0f;
                drawPerson(sf::Vector2f(WIDTH * 0.36f + std::sin(localT * 1.1f) * 3.0f, groundY),
                           sf::Color(40, 52, 76),
                           sf::Color(110, 150, 230),
                           false,
                           false,
                           walk,
                           false,
                           sceneAlpha);
                drawPerson(sf::Vector2f(WIDTH * 0.50f, groundY),
                           sf::Color(90, 54, 84),
                           sf::Color(228, 154, 204),
                           true,
                           false,
                           walk * 0.6f + 1.2f,
                           false,
                           sceneAlpha);
                drawPerson(sf::Vector2f(WIDTH * 0.43f, groundY + 2.0f),
                           sf::Color(60, 80, 120),
                           sf::Color(120, 205, 255),
                           false,
                           true,
                           walk + 0.6f,
                           false,
                           sceneAlpha);

                sf::CircleShape familyGlow(96.0f + std::sin(localT * 2.2f) * 6.0f);
                familyGlow.setOrigin(familyGlow.getRadius(), familyGlow.getRadius());
                familyGlow.setPosition(WIDTH * 0.43f, groundY - 38.0f);
                familyGlow.setFillColor(sf::Color(240, 210, 160, 34));
                window.draw(familyGlow);

                if (localT > 1.1f)
                {
                    sf::Uint8 bubbleAlpha = static_cast<sf::Uint8>(std::min(255.0f, (localT - 1.1f) * 230.0f));
                    drawSpeechBubble("I fear this job will take you from us.", sf::Vector2f(WIDTH * 0.59f, groundY - 170.0f), bubbleAlpha);
                }
            }
            else if (sceneIndex == 1)
            {
                sf::VertexArray sky(sf::Quads, 4);
                sky[0].position = sf::Vector2f(0, 0);
                sky[1].position = sf::Vector2f(WIDTH, 0);
                sky[2].position = sf::Vector2f(WIDTH, HEIGHT);
                sky[3].position = sf::Vector2f(0, HEIGHT);
                sky[0].color = sf::Color(14, 18, 36);
                sky[1].color = sf::Color(24, 24, 46);
                sky[2].color = sf::Color(8, 10, 24);
                sky[3].color = sf::Color(5, 8, 20);
                window.draw(sky);

                for (int i = 0; i < 95; i++)
                {
                    float rx = std::fmod(i * 19.0f + localT * 260.0f, WIDTH + 60.0f) - 30.0f;
                    float ry = std::fmod(i * 33.0f + localT * 410.0f, HEIGHT + 90.0f) - 45.0f;
                    sf::RectangleShape drop(sf::Vector2f(1.5f, 12.0f));
                    drop.setPosition(rx, ry);
                    drop.setRotation(16.0f);
                    drop.setFillColor(sf::Color(146, 188, 240, 105));
                    window.draw(drop);
                }

                for (int i = 0; i < 8; i++)
                {
                    float bx = i * 180.0f - std::fmod(localT * 36.0f, 180.0f);
                    float h = 190.0f + (i % 4) * 44.0f;
                    sf::RectangleShape tower(sf::Vector2f(120, h));
                    tower.setPosition(bx, groundY - h);
                    tower.setFillColor(sf::Color(22, 28, 46));
                    window.draw(tower);
                }

                sf::RectangleShape road(sf::Vector2f(WIDTH, HEIGHT - groundY));
                road.setPosition(0, groundY);
                road.setFillColor(sf::Color(28, 30, 38));
                window.draw(road);

                sf::RectangleShape sirenRed(sf::Vector2f(220.0f, 18.0f));
                sirenRed.setPosition(WIDTH * 0.12f, groundY - 8.0f);
                sirenRed.setFillColor(sf::Color(255, 72, 82, static_cast<sf::Uint8>(65 + 55 * (0.5f + 0.5f * std::sin(localT * 8.0f)))));
                window.draw(sirenRed);
                sf::RectangleShape sirenBlue(sf::Vector2f(220.0f, 18.0f));
                sirenBlue.setPosition(WIDTH * 0.28f, groundY - 8.0f);
                sirenBlue.setFillColor(sf::Color(85, 132, 255, static_cast<sf::Uint8>(65 + 55 * (0.5f + 0.5f * std::sin(localT * 8.0f + 1.7f)))));
                window.draw(sirenBlue);

                float travel = clamp01(localT / std::max(0.8f, sceneDuration - 0.6f));
                float adamX = WIDTH * 0.16f + travel * WIDTH * 0.60f;
                drawPerson(sf::Vector2f(adamX, groundY),
                           sf::Color(34, 46, 72),
                           sf::Color(120, 170, 248),
                           false,
                           false,
                           localT * 8.0f,
                           true,
                           sceneAlpha);

                if (std::fmod(localT, 1.15f) < 0.16f)
                {
                    sf::CircleShape flash(14);
                    flash.setOrigin(14, 14);
                    flash.setPosition(adamX + 38.0f, groundY - 54.0f);
                    flash.setFillColor(sf::Color(255, 208, 130, 210));
                    window.draw(flash);

                    sf::RectangleShape tracer(sf::Vector2f(160.0f, 2.0f));
                    tracer.setPosition(adamX + 42.0f, groundY - 56.0f);
                    tracer.setRotation(-4.0f);
                    tracer.setFillColor(sf::Color(255, 220, 165, 130));
                    window.draw(tracer);
                }
            }
            else if (sceneIndex == 2)
            {
                sf::RectangleShape left(sf::Vector2f(WIDTH * 0.5f, HEIGHT));
                left.setPosition(0, 0);
                left.setFillColor(sf::Color(56, 74, 110));
                window.draw(left);

                sf::RectangleShape right(sf::Vector2f(WIDTH * 0.5f, HEIGHT));
                right.setPosition(WIDTH * 0.5f, 0);
                right.setFillColor(sf::Color(14, 18, 36));
                window.draw(right);

                sf::RectangleShape split(sf::Vector2f(4, HEIGHT));
                split.setPosition(WIDTH * 0.5f - 2, 0);
                split.setFillColor(sf::Color(145, 182, 232, 180));
                window.draw(split);

                for (int i = 0; i < 11; i++)
                {
                    float y = groundY - 180.0f + i * 16.0f;
                    float amp = std::sin(localT * 5.0f + i * 0.65f) * 28.0f;
                    sf::RectangleShape radio(sf::Vector2f(70.0f + std::abs(amp), 2.0f));
                    radio.setOrigin(radio.getSize().x * 0.5f, 1.0f);
                    radio.setPosition(WIDTH * 0.5f, y);
                    radio.setFillColor(sf::Color(140, 182, 240, static_cast<sf::Uint8>(45 + i * 6)));
                    window.draw(radio);
                }

                sf::RectangleShape floorL(sf::Vector2f(WIDTH * 0.5f, HEIGHT - groundY));
                floorL.setPosition(0, groundY);
                floorL.setFillColor(sf::Color(42, 95, 66));
                window.draw(floorL);
                sf::RectangleShape floorR(sf::Vector2f(WIDTH * 0.5f, HEIGHT - groundY));
                floorR.setPosition(WIDTH * 0.5f, groundY);
                floorR.setFillColor(sf::Color(28, 30, 38));
                window.draw(floorR);

                drawPerson(sf::Vector2f(WIDTH * 0.26f, groundY),
                           sf::Color(90, 54, 84),
                           sf::Color(228, 154, 204),
                           true,
                           false,
                           localT * 2.2f,
                           false,
                           sceneAlpha);
                drawPerson(sf::Vector2f(WIDTH * 0.19f, groundY + 2.0f),
                           sf::Color(60, 80, 120),
                           sf::Color(120, 205, 255),
                           false,
                           true,
                           localT * 2.8f,
                           false,
                           sceneAlpha);

                drawPerson(sf::Vector2f(WIDTH * 0.76f, groundY),
                           sf::Color(34, 46, 72),
                           sf::Color(120, 170, 248),
                           false,
                           false,
                           localT * 3.8f,
                           true,
                           sceneAlpha);

                sf::CircleShape tension(80.0f + std::sin(localT * 2.0f) * 8.0f);
                tension.setOrigin(tension.getRadius(), tension.getRadius());
                tension.setPosition(WIDTH * 0.76f, groundY - 52.0f);
                tension.setFillColor(sf::Color(120, 170, 248, 24));
                window.draw(tension);

                sf::RectangleShape line(sf::Vector2f(WIDTH * 0.28f, 2.0f));
                line.setPosition(WIDTH * 0.48f, groundY - 88.0f);
                line.setFillColor(sf::Color(170, 206, 255, 180));
                window.draw(line);

                if (localT > 0.9f)
                {
                    sf::Uint8 bubbleAlpha = static_cast<sf::Uint8>(std::min(255.0f, (localT - 0.9f) * 220.0f));
                    drawSpeechBubble("Promise me you come back whole.", sf::Vector2f(WIDTH * 0.33f, groundY - 170.0f), bubbleAlpha);
                }
            }
            else
            {
                sf::VertexArray sky(sf::Quads, 4);
                sky[0].position = sf::Vector2f(0, 0);
                sky[1].position = sf::Vector2f(WIDTH, 0);
                sky[2].position = sf::Vector2f(WIDTH, HEIGHT);
                sky[3].position = sf::Vector2f(0, HEIGHT);
                sky[0].color = sf::Color(6, 8, 16);
                sky[1].color = sf::Color(8, 10, 20);
                sky[2].color = sf::Color(3, 4, 12);
                sky[3].color = sf::Color(2, 2, 9);
                window.draw(sky);

                sf::CircleShape aura(120.0f + std::sin(localT * 2.6f) * 12.0f);
                aura.setOrigin(aura.getRadius(), aura.getRadius());
                aura.setPosition(WIDTH * 0.5f, groundY - 82.0f);
                aura.setFillColor(sf::Color(82, 118, 184, 40));
                window.draw(aura);

                for (int i = 0; i < 48; i++)
                {
                    float px = std::fmod(i * 37.0f + localT * 24.0f, WIDTH + 40.0f) - 20.0f;
                    float py = std::fmod(i * 23.0f + localT * 31.0f, HEIGHT + 40.0f) - 20.0f;
                    sf::CircleShape mote(1.8f + (i % 3) * 0.45f);
                    mote.setFillColor(sf::Color(132, 176, 238, static_cast<sf::Uint8>(30 + (i % 4) * 16)));
                    mote.setPosition(px, py);
                    window.draw(mote);
                }

                drawPerson(sf::Vector2f(WIDTH * 0.46f, groundY + 4.0f),
                           sf::Color(90, 54, 84),
                           sf::Color(228, 154, 204),
                           true,
                           false,
                           0.0f,
                           false,
                           108);
                drawPerson(sf::Vector2f(WIDTH * 0.56f, groundY + 7.0f),
                           sf::Color(60, 80, 120),
                           sf::Color(120, 205, 255),
                           false,
                           true,
                           0.0f,
                           false,
                           104);
                drawPerson(sf::Vector2f(WIDTH * 0.5f, groundY),
                           sf::Color(24, 35, 58),
                           sf::Color(112, 166, 248),
                           false,
                           false,
                           std::sin(localT * 1.4f) * 0.4f,
                           true,
                           sceneAlpha);

                sf::Text qMark;
                qMark.setFont(font);
                qMark.setCharacterSize(112);
                qMark.setStyle(sf::Text::Bold);
                qMark.setFillColor(sf::Color(172, 208, 255, static_cast<sf::Uint8>(48 + 34 * (0.5f + 0.5f * std::sin(localT * 2.4f)))));
                qMark.setString("?");
                sf::FloatRect qb = qMark.getLocalBounds();
                qMark.setOrigin(qb.left + qb.width * 0.5f, qb.top + qb.height * 0.5f);
                qMark.setPosition(WIDTH * 0.5f, groundY - 174.0f);
                window.draw(qMark);
            }

            window.setView(worldView);

            float barPulse = 1.5f + 1.5f * std::sin(elapsed * 1.7f);
            sf::RectangleShape topBar(sf::Vector2f(WIDTH, 45.0f + barPulse));
            topBar.setPosition(0.0f, 0.0f);
            topBar.setFillColor(sf::Color(0, 0, 0, 245));
            window.draw(topBar);
            sf::RectangleShape bottomBar(sf::Vector2f(WIDTH, 58.0f + barPulse));
            bottomBar.setPosition(0.0f, HEIGHT - (58.0f + barPulse));
            bottomBar.setFillColor(sf::Color(0, 0, 0, 245));
            window.draw(bottomBar);
            drawSceneHeader(scenes[sceneIndex].title, sceneAlpha);

            sf::Text sceneCounter;
            sceneCounter.setFont(font);
            sceneCounter.setCharacterSize(16);
            sceneCounter.setFillColor(sf::Color(182, 204, 230, sceneAlpha));
            sceneCounter.setString(std::to_string(sceneIndex + 1) + "/" + std::to_string(static_cast<int>(scenes.size())));
            sceneCounter.setPosition(WIDTH - 76.0f, 24.0f);
            window.draw(sceneCounter);

            float titleBurst = clamp01((1.15f - localT) / 1.15f);
            if (titleBurst > 0.0f)
            {
                sf::Text centerTitle;
                centerTitle.setFont(font);
                centerTitle.setCharacterSize(52);
                centerTitle.setStyle(sf::Text::Bold);
                centerTitle.setFillColor(sf::Color(215, 232, 255, static_cast<sf::Uint8>(170 * titleBurst)));
                centerTitle.setString(scenes[sceneIndex].title);
                sf::FloatRect ctb = centerTitle.getLocalBounds();
                centerTitle.setOrigin(ctb.left + ctb.width * 0.5f, ctb.top + ctb.height * 0.5f);
                centerTitle.setPosition(WIDTH * 0.5f, 104.0f - (1.0f - titleBurst) * 14.0f);
                window.draw(centerTitle);
            }

            float subtitleReveal = clamp01((localT - 0.12f) / std::max(0.5f, sceneDuration - 0.35f));
            const std::string &fullSubtitle = scenes[sceneIndex].subtitle;
            std::size_t shownChars = static_cast<std::size_t>(fullSubtitle.size() * subtitleReveal);
            std::string shownSubtitle = fullSubtitle.substr(0, shownChars);
            if (shownChars < fullSubtitle.size() && (static_cast<int>(elapsed * 8.0f) % 2 == 0))
                shownSubtitle += "_";

            float subtitlePulse = 1.0f + 0.008f * std::sin(localT * 5.0f);
            sf::RectangleShape subtitlePanel(sf::Vector2f(930.0f * subtitlePulse, 82.0f));
            subtitlePanel.setOrigin(subtitlePanel.getSize().x * 0.5f, subtitlePanel.getSize().y * 0.5f);
            subtitlePanel.setPosition(WIDTH * 0.5f, HEIGHT - 66.0f);
            subtitlePanel.setFillColor(sf::Color(12, 22, 42, 198));
            subtitlePanel.setOutlineThickness(2.0f);
            subtitlePanel.setOutlineColor(sf::Color(108, 160, 232, 150));
            window.draw(subtitlePanel);

            sf::Text subtitle;
            subtitle.setFont(font);
            subtitle.setCharacterSize(24);
            subtitle.setFillColor(sf::Color(212, 228, 248, 235));
            subtitle.setString(wrapText(shownSubtitle, 58));
            sf::FloatRect st = subtitle.getLocalBounds();
            subtitle.setOrigin(st.left + st.width * 0.5f, st.top + st.height * 0.5f);
            subtitle.setPosition(WIDTH * 0.5f, HEIGHT - 68.0f);
            window.draw(subtitle);

            sf::RectangleShape progressBg(sf::Vector2f(420.0f, 6.0f));
            progressBg.setPosition(WIDTH * 0.5f - 210.0f, HEIGHT - 30.0f);
            progressBg.setFillColor(sf::Color(30, 45, 68, 220));
            window.draw(progressBg);

            float progressRatio = clamp01(elapsed / std::max(0.01f, totalDuration));
            sf::RectangleShape progressFill(sf::Vector2f(420.0f * progressRatio, 6.0f));
            progressFill.setPosition(WIDTH * 0.5f - 210.0f, HEIGHT - 30.0f);
            progressFill.setFillColor(sf::Color(124, 216, 255, 240));
            window.draw(progressFill);

            sf::Text skipHint;
            skipHint.setFont(font);
            skipHint.setCharacterSize(15);
            skipHint.setFillColor(sf::Color(168, 186, 210, 220));
            skipHint.setString("Enter / Space / Esc to skip");
            skipHint.setPosition(WIDTH * 0.5f - 108.0f, HEIGHT - 22.0f);
            window.draw(skipHint);

            float edgeT = std::min(localT, sceneDuration - localT);
            float transitionMask = clamp01((0.24f - edgeT) / 0.24f);
            if (transitionMask > 0.0f)
            {
                sf::RectangleShape fadeMask(sf::Vector2f(WIDTH, HEIGHT));
                fadeMask.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(215.0f * transitionMask)));
                window.draw(fadeMask);
            }

            sf::VertexArray leftVig(sf::Quads, 4);
            leftVig[0].position = sf::Vector2f(0, 0);
            leftVig[1].position = sf::Vector2f(180, 0);
            leftVig[2].position = sf::Vector2f(180, HEIGHT);
            leftVig[3].position = sf::Vector2f(0, HEIGHT);
            leftVig[0].color = sf::Color(0, 0, 0, 150);
            leftVig[1].color = sf::Color(0, 0, 0, 0);
            leftVig[2].color = sf::Color(0, 0, 0, 0);
            leftVig[3].color = sf::Color(0, 0, 0, 150);
            window.draw(leftVig);

            sf::VertexArray rightVig(sf::Quads, 4);
            rightVig[0].position = sf::Vector2f(WIDTH - 180, 0);
            rightVig[1].position = sf::Vector2f(WIDTH, 0);
            rightVig[2].position = sf::Vector2f(WIDTH, HEIGHT);
            rightVig[3].position = sf::Vector2f(WIDTH - 180, HEIGHT);
            rightVig[0].color = sf::Color(0, 0, 0, 0);
            rightVig[1].color = sf::Color(0, 0, 0, 150);
            rightVig[2].color = sf::Color(0, 0, 0, 150);
            rightVig[3].color = sf::Color(0, 0, 0, 0);
            window.draw(rightVig);

            for (int i = 0; i < 80; i++)
            {
                float gx = std::fmod(elapsed * 320.0f + i * 97.0f, static_cast<float>(WIDTH));
                float gy = std::fmod(elapsed * 470.0f + i * 57.0f, static_cast<float>(HEIGHT));
                sf::RectangleShape grain(sf::Vector2f(1.0f, 1.0f));
                grain.setPosition(gx, gy);
                grain.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(8 + (i * 17) % 18)));
                window.draw(grain);
            }

            window.display();
        }
        return false;
    };
    float dt = 0.0f;

    sf::VertexArray menuBg(sf::Quads, 4);
    menuBg[0].position = sf::Vector2f(0, 0);
    menuBg[1].position = sf::Vector2f(WIDTH, 0);
    menuBg[2].position = sf::Vector2f(WIDTH, HEIGHT);
    menuBg[3].position = sf::Vector2f(0, HEIGHT);
    menuBg[0].color = sf::Color(10, 10, 30);
    menuBg[1].color = sf::Color(20, 20, 50);
    menuBg[2].color = sf::Color(5, 5, 15);
    menuBg[3].color = sf::Color(10, 10, 25);

    sf::Text titleText;
    titleText.setFont(font);
    titleText.setCharacterSize(56);
    titleText.setFillColor(sf::Color(230, 230, 255));
    titleText.setStyle(sf::Text::Bold);
    titleText.setString("ECLIPSE KNIGHT");
    {
        sf::FloatRect b = titleText.getLocalBounds();
        titleText.setOrigin(b.left + b.width / 2, b.top + b.height / 2);
        titleText.setPosition(WIDTH / 2, logoLoaded ? 140 : 120);
    }

    sf::Text subtitleText;
    subtitleText.setFont(font);
    subtitleText.setCharacterSize(18);
    subtitleText.setFillColor(sf::Color(160, 180, 200));
    subtitleText.setString(DIE_AGAIN_MODE ? "World-Class Global Edition - Die Again Chaos" : "World-Class Global Metroidvania Edition");
    {
        sf::FloatRect b = subtitleText.getLocalBounds();
        subtitleText.setOrigin(b.left + b.width / 2, b.top + b.height / 2);
        subtitleText.setPosition(WIDTH / 2, logoLoaded ? 185 : 160);
    }

    sf::Text hintText;
    hintText.setFont(font);
    hintText.setCharacterSize(16);
    hintText.setFillColor(sf::Color(150, 150, 150));
    hintText.setString("W/S or Arrows to navigate  |  Enter to select");
    {
        sf::FloatRect b = hintText.getLocalBounds();
        hintText.setOrigin(b.left + b.width / 2, b.top + b.height / 2);
        hintText.setPosition(WIDTH / 2, HEIGHT - 50);
    }

    sf::Text controlsTitle;
    controlsTitle.setFont(font);
    controlsTitle.setCharacterSize(46);
    controlsTitle.setFillColor(sf::Color(230, 230, 255));
    controlsTitle.setStyle(sf::Text::Bold);
    controlsTitle.setString("CONTROLS");
    {
        sf::FloatRect b = controlsTitle.getLocalBounds();
        controlsTitle.setOrigin(b.left + b.width / 2, b.top + b.height / 2);
        controlsTitle.setPosition(WIDTH / 2, 140);
    }

    sf::Text controlsBody;
    controlsBody.setFont(font);
    controlsBody.setCharacterSize(22);
    controlsBody.setFillColor(sf::Color(220, 220, 220));
    controlsBody.setString(
        "Move: WASD / Arrows\n"
        "Jump: Space or W/Up (buffered + coyote)\n"
        "Dash: Shift\n"
        "Nail Slash: Z\n"
        "Shoot: X (hold to charge)\n"
        "Focus Heal: Hold C (costs soul)\n"
        "Restart: R\n"
        "Level 2: Armory loadout selection (5 weapon builds).\n"
        "Die Again mode can be toggled from Settings.\n"
        "Settings Hub: Q/E switches categories, includes trap density/floor-gate controls.\n"
        "Collect relics, avoid cursed coins, and survive fake floors/spike traps!");
    {
        sf::FloatRect b = controlsBody.getLocalBounds();
        controlsBody.setOrigin(b.left + b.width / 2, b.top + b.height / 2);
        controlsBody.setPosition(WIDTH / 2, HEIGHT / 2 + 20);
    }

    sf::Text backHint;
    backHint.setFont(font);
    backHint.setCharacterSize(18);
    backHint.setFillColor(sf::Color(160, 160, 160));
    backHint.setString("Press ESC to return");
    {
        sf::FloatRect b = backHint.getLocalBounds();
        backHint.setOrigin(b.left + b.width / 2, b.top + b.height / 2);
        backHint.setPosition(WIDTH / 2, HEIGHT - 70);
    }

main_menu_loop:
    while (showMenu && window.isOpen())
    {
        float menuDt = menuClock.restart().asSeconds();
        updatePerfStats(menuDt);
        settingsBannerTimer = std::max(0.0f, settingsBannerTimer - menuDt);
        menuBannerTimer = std::max(0.0f, menuBannerTimer - menuDt);
        background.update(menuDt * 0.4f);

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::Resized)
            {
                applyLetterboxView(worldView,
                                   static_cast<unsigned int>(event.size.width),
                                   static_cast<unsigned int>(event.size.height));
                window.setView(worldView);
            }
            if (event.type == sf::Event::KeyPressed)
            {
                if (showSettings)
                {
                    if (event.key.code == sf::Keyboard::Escape ||
                        event.key.code == sf::Keyboard::Backspace)
                    {
                        showSettings = false;
                    }
                    else if (event.key.code == sf::Keyboard::Q ||
                             event.key.code == sf::Keyboard::A ||
                             event.key.code == sf::Keyboard::PageUp)
                    {
                        cycleSettingsTab(-1);
                    }
                    else if (event.key.code == sf::Keyboard::E ||
                             event.key.code == sf::Keyboard::D ||
                             event.key.code == sf::Keyboard::PageDown ||
                             event.key.code == sf::Keyboard::Tab)
                    {
                        cycleSettingsTab(1);
                    }
                    else if (event.key.code == sf::Keyboard::Up ||
                             event.key.code == sf::Keyboard::W)
                    {
                        int rows = rowsForTab(settingsTabIndex);
                        settingsIndex = (settingsIndex + rows - 1) % rows;
                    }
                    else if (event.key.code == sf::Keyboard::Down ||
                             event.key.code == sf::Keyboard::S)
                    {
                        int rows = rowsForTab(settingsTabIndex);
                        settingsIndex = (settingsIndex + 1) % rows;
                    }
                    else if (event.key.code == sf::Keyboard::Left)
                    {
                        changeSetting(-1);
                    }
                    else if (event.key.code == sf::Keyboard::Right)
                    {
                        changeSetting(1);
                    }
                    else if (event.key.code == sf::Keyboard::Enter ||
                             event.key.code == sf::Keyboard::Space)
                    {
                        if (settingsTabIndex == 5 && settingsIndex == 0)
                        {
                            loadCurrentSettings();
                            settingsBannerText = "Loaded from settings.cfg";
                            settingsBannerTimer = 2.8f;
                        }
                        else
                        {
                            showSettings = false;
                        }
                    }
                }
                else if (showControls)
                {
                    if (event.key.code == sf::Keyboard::Escape ||
                        event.key.code == sf::Keyboard::Backspace ||
                        event.key.code == sf::Keyboard::Enter)
                    {
                        showControls = false;
                    }
                }
                else
                {
                    if (event.key.code == sf::Keyboard::Up ||
                        event.key.code == sf::Keyboard::W)
                    {
                        menuIndex = (menuIndex + (int)menuItems.size() - 1) % (int)menuItems.size();
                    }
                    else if (event.key.code == sf::Keyboard::Down ||
                             event.key.code == sf::Keyboard::S)
                    {
                        menuIndex = (menuIndex + 1) % (int)menuItems.size();
                    }
                    else if (event.key.code == sf::Keyboard::Enter ||
                             event.key.code == sf::Keyboard::Space)
                    {
                        if (menuIndex == 0)
                        {
                            if (loadRunProgress())
                            {
                                showControls = false;
                                showSettings = false;
                                showMenu = false;
                                clock.restart();
                            }
                            else
                            {
                                menuBannerText = "No save found. Use ESC > SAVE in-game first.";
                                menuBannerTimer = 3.2f;
                            }
                        }
                        else if (menuIndex == 1)
                        {
                            bool introFinished = playStartStoryCinematic();
                            if (introFinished && window.isOpen())
                            {
                                resetRunForNewGame();
                                showControls = false;
                                showSettings = false;
                                showMenu = false;
                                clock.restart();
                            }
                            else
                            {
                                showMenu = false;
                            }
                        }
                        else if (menuIndex == 2)
                        {
                            showSettings = true;
                            normalizeSettingsSelection();
                        }
                        else if (menuIndex == 3)
                        {
                            showControls = true;
                        }
                        else if (menuIndex == 4)
                        {
                            window.close();
                            showMenu = false;
                        }
                    }
                    else if (event.key.code == sf::Keyboard::Escape)
                    {
                        window.close();
                        showMenu = false;
                    }
                }
            }
        }

        window.clear();
        window.draw(menuBg);
        background.draw(window);

        sf::RectangleShape dim(sf::Vector2f(WIDTH, HEIGHT));
        dim.setFillColor(sf::Color(10, 10, 20, 160));
        window.draw(dim);

        if (logoLoaded)
            window.draw(logo);
        window.draw(titleText);
        window.draw(subtitleText);

        if (showControls)
        {
            window.draw(controlsTitle);
            window.draw(controlsBody);
            window.draw(backHint);
        }
        else if (showSettings)
        {
            sf::Text settingsTitle;
            settingsTitle.setFont(font);
            settingsTitle.setCharacterSize(42);
            settingsTitle.setStyle(sf::Text::Bold);
            settingsTitle.setFillColor(sf::Color(220, 235, 255));
            settingsTitle.setString("GLOBAL SETTINGS HUB");
            sf::FloatRect st = settingsTitle.getLocalBounds();
            settingsTitle.setOrigin(st.left + st.width * 0.5f, st.top + st.height * 0.5f);
            settingsTitle.setPosition(WIDTH * 0.5f, 140.0f);
            window.draw(settingsTitle);

            const float panelW = 890.0f;
            const float panelH = 500.0f;
            const float panelCX = WIDTH * 0.5f;
            const float panelCY = HEIGHT * 0.57f;
            const float panelLeft = panelCX - panelW * 0.5f;
            const float panelTop = panelCY - panelH * 0.5f;

            sf::RectangleShape panel(sf::Vector2f(panelW, panelH));
            panel.setOrigin(panel.getSize() * 0.5f);
            panel.setPosition(panelCX, panelCY);
            panel.setFillColor(sf::Color(8, 18, 34, 215));
            panel.setOutlineThickness(3.0f);
            panel.setOutlineColor(sf::Color(95, 150, 235, 230));
            window.draw(panel);

            float pulse = 0.5f + 0.5f * std::sin(menuClock.getElapsedTime().asSeconds() * 2.8f);
            sf::RectangleShape glow(sf::Vector2f(904, 514));
            glow.setOrigin(glow.getSize() * 0.5f);
            glow.setPosition(panelCX, panelCY);
            glow.setFillColor(sf::Color(50, 95, 170, static_cast<sf::Uint8>(18 + pulse * 22)));
            window.draw(glow);

            float tabY = panelTop + 26.0f;
            float tabAreaW = 780.0f;
            float tabStartX = panelCX - tabAreaW * 0.5f;
            float tabW = tabAreaW / static_cast<float>(settingsTabs.size());
            for (int i = 0; i < static_cast<int>(settingsTabs.size()); i++)
            {
                sf::RectangleShape tab(sf::Vector2f(tabW - 8.0f, 34.0f));
                tab.setPosition(tabStartX + i * tabW, tabY);
                bool selected = (i == settingsTabIndex);
                tab.setFillColor(selected ? sf::Color(70, 130, 225, 210) : sf::Color(30, 46, 72, 200));
                tab.setOutlineThickness(2.0f);
                tab.setOutlineColor(selected ? sf::Color(165, 225, 255) : sf::Color(85, 116, 156));
                window.draw(tab);

                sf::Text tabLabel;
                tabLabel.setFont(font);
                tabLabel.setCharacterSize(17);
                tabLabel.setStyle(sf::Text::Bold);
                tabLabel.setFillColor(selected ? sf::Color(240, 250, 255) : sf::Color(170, 200, 230));
                tabLabel.setString(settingsTabs[i]);
                sf::FloatRect tlb = tabLabel.getLocalBounds();
                tabLabel.setOrigin(tlb.left + tlb.width * 0.5f, tlb.top + tlb.height * 0.5f);
                tabLabel.setPosition(tabStartX + i * tabW + (tabW - 8.0f) * 0.5f, tabY + 17.0f);
                window.draw(tabLabel);
            }

            std::vector<std::string> settingNames;
            std::vector<std::string> settingValues;
            std::vector<std::string> settingHelp;
            if (settingsTabIndex == 0)
            {
                settingNames = {"Frame Limit", "Render Quality"};
                settingValues = {fpsLabel(), qualityLabels[qualityIndex]};
                settingHelp = {"Controls smoothness and latency target.", "Controls world detail and FX density."};
            }
            else if (settingsTabIndex == 1)
            {
                settingNames = {"Screen Shake", "Hardcore Mode", "Die Again Mode", "Adaptive Director"};
                settingValues = {screenShakeEnabled ? "On" : "Off", hardcoreModeEnabled ? "On" : "Off",
                                 dieAgainModeEnabled ? "On" : "Off", adaptiveDirectorEnabled ? "On" : "Off"};
                settingHelp = {"Camera impact feedback during hits/explosions.",
                               "Adds extra challenge multipliers.",
                               "Enables troll-like trap pressure and punishment.",
                               "AI director scales pressure based on your performance."};
            }
            else if (settingsTabIndex == 2)
            {
                settingNames = {"Trap Density", "Spike Tempo", "Opening Floors"};
                settingValues = {trapDensityLabels[trapDensityIndex], spikeTempoLabels[spikeTempoIndex], openFloorLabels[openFloorModeIndex]};
                settingHelp = {"How many trap events are injected per stage.",
                               "How fast and aggressive spike traps behave.",
                               "Adds opening floor gates like Die Again chaos."};
            }
            else if (settingsTabIndex == 3)
            {
                settingNames = {"Master Volume"};
                settingValues = {std::to_string(masterVolume) + "%"};
                settingHelp = {"Global volume for all SFX."};
            }
            else if (settingsTabIndex == 4)
            {
                settingNames = {"Character Style"};
                settingValues = {characterStyles[characterStyleIndex]};
                settingHelp = {"Switches hero visual identity and weapon style."};
            }
            else
            {
                settingNames = {"Load"};
                settingValues = {"Press Enter"};
                settingHelp = {"Loads all options from settings.cfg and applies them instantly."};
            }

            const float listX = panelLeft + 28.0f;
            const float listY = panelTop + 98.0f;
            const float listW = 430.0f;
            const float rowH = 56.0f;

            sf::Text optionHeader;
            optionHeader.setFont(font);
            optionHeader.setCharacterSize(15);
            optionHeader.setStyle(sf::Text::Bold);
            optionHeader.setFillColor(sf::Color(132, 170, 210));
            optionHeader.setString("OPTION");
            optionHeader.setPosition(listX + 10.0f, listY - 32.0f);
            window.draw(optionHeader);

            sf::Text valueHeader;
            valueHeader.setFont(font);
            valueHeader.setCharacterSize(15);
            valueHeader.setStyle(sf::Text::Bold);
            valueHeader.setFillColor(sf::Color(132, 170, 210));
            valueHeader.setString("VALUE");
            sf::FloatRect vh = valueHeader.getLocalBounds();
            valueHeader.setOrigin(vh.left + vh.width, vh.top);
            valueHeader.setPosition(listX + listW - 10.0f, listY - 32.0f);
            window.draw(valueHeader);

            for (int i = 0; i < static_cast<int>(settingNames.size()); i++)
            {
                float rowY = listY + i * rowH;
                bool selected = (i == settingsIndex);

                sf::RectangleShape row(sf::Vector2f(listW, rowH - 10.0f));
                row.setPosition(listX, rowY + 3.0f);
                row.setFillColor(selected ? sf::Color(22, 52, 88, 235) : sf::Color(14, 32, 56, 190));
                row.setOutlineThickness(selected ? 2.0f : 1.0f);
                row.setOutlineColor(selected ? sf::Color(112, 214, 255) : sf::Color(70, 108, 150));
                window.draw(row);

                sf::Text optionText;
                optionText.setFont(font);
                optionText.setCharacterSize(24);
                optionText.setStyle(sf::Text::Bold);
                optionText.setFillColor(selected ? sf::Color(198, 240, 255) : sf::Color(210, 220, 238));
                optionText.setString(settingNames[i]);
                optionText.setPosition(listX + 14.0f, rowY + 11.0f);
                window.draw(optionText);

                sf::Text valueText;
                valueText.setFont(font);
                valueText.setCharacterSize(24);
                valueText.setStyle(sf::Text::Bold);
                valueText.setFillColor(selected ? sf::Color(122, 228, 255) : sf::Color(172, 204, 235));
                valueText.setString(settingValues[i]);
                sf::FloatRect vb = valueText.getLocalBounds();
                valueText.setOrigin(vb.left + vb.width, vb.top);
                valueText.setPosition(listX + listW - 14.0f, rowY + 11.0f);
                window.draw(valueText);
            }

            const float detailX = panelLeft + 478.0f;
            const float detailY = panelTop + 96.0f;
            const float detailW = 384.0f;
            const float detailH = 220.0f;

            sf::RectangleShape detailPanel(sf::Vector2f(detailW, detailH));
            detailPanel.setPosition(detailX, detailY);
            detailPanel.setFillColor(sf::Color(11, 24, 45, 230));
            detailPanel.setOutlineThickness(2.0f);
            detailPanel.setOutlineColor(sf::Color(95, 156, 225, 210));
            window.draw(detailPanel);

            sf::Text detailTitle;
            detailTitle.setFont(font);
            detailTitle.setCharacterSize(21);
            detailTitle.setStyle(sf::Text::Bold);
            detailTitle.setFillColor(sf::Color(196, 228, 255));
            detailTitle.setString(settingNames[settingsIndex]);
            detailTitle.setPosition(detailX + 16.0f, detailY + 12.0f);
            window.draw(detailTitle);

            sf::Text detailValue;
            detailValue.setFont(font);
            detailValue.setCharacterSize(27);
            detailValue.setStyle(sf::Text::Bold);
            detailValue.setFillColor(sf::Color(130, 225, 255));
            detailValue.setString(settingValues[settingsIndex]);
            detailValue.setPosition(detailX + 16.0f, detailY + 43.0f);
            window.draw(detailValue);

            sf::Text detailHelp;
            detailHelp.setFont(font);
            detailHelp.setCharacterSize(16);
            detailHelp.setFillColor(sf::Color(172, 198, 226));
            detailHelp.setLineSpacing(1.25f);
            detailHelp.setString(wrapText(settingHelp[settingsIndex], 36));
            detailHelp.setPosition(detailX + 16.0f, detailY + 86.0f);
            window.draw(detailHelp);

            float sliderRatio = 0.0f;
            sf::Color sliderColor(100, 180, 255);
            if (settingsTabIndex == 0 && settingsIndex == 0)
            {
                sliderRatio = static_cast<float>(fpsIndex) / static_cast<float>(fpsOptions.size() - 1);
                sliderColor = sf::Color(112, 216, 255);
            }
            else if (settingsTabIndex == 0 && settingsIndex == 1)
            {
                sliderRatio = static_cast<float>(qualityIndex) / static_cast<float>(qualityLabels.size() - 1);
                sliderColor = sf::Color(90, 190, 255);
            }
            else if (settingsTabIndex == 1)
            {
                bool on = screenShakeEnabled;
                if (settingsIndex == 1)
                    on = hardcoreModeEnabled;
                else if (settingsIndex == 2)
                    on = dieAgainModeEnabled;
                else if (settingsIndex == 3)
                    on = adaptiveDirectorEnabled;
                sliderRatio = on ? 1.0f : 0.0f;
                sliderColor = on ? sf::Color(110, 240, 170) : sf::Color(255, 170, 130);
            }
            else if (settingsTabIndex == 2)
            {
                if (settingsIndex == 0)
                    sliderRatio = static_cast<float>(trapDensityIndex) / static_cast<float>(trapDensityLabels.size() - 1);
                else if (settingsIndex == 1)
                    sliderRatio = static_cast<float>(spikeTempoIndex) / static_cast<float>(spikeTempoLabels.size() - 1);
                else
                    sliderRatio = static_cast<float>(openFloorModeIndex) / static_cast<float>(openFloorLabels.size() - 1);
                sliderColor = sf::Color(255, 125, 145);
            }
            else if (settingsTabIndex == 3)
            {
                sliderRatio = static_cast<float>(masterVolume) / 100.0f;
                sliderColor = sf::Color(255, 176, 96);
            }
            else if (settingsTabIndex == 4)
            {
                sliderRatio = static_cast<float>(characterStyleIndex) / static_cast<float>(characterStyles.size() - 1);
                sliderColor = sf::Color(198, 170, 255);
            }
            else
            {
                sliderRatio = 1.0f;
                sliderColor = sf::Color(124, 226, 186);
            }

            float sliderW = detailW - 32.0f;
            float sliderY = detailY + detailH - 34.0f;

            sf::RectangleShape sliderBg(sf::Vector2f(sliderW, 12));
            sliderBg.setPosition(detailX + 16.0f, sliderY);
            sliderBg.setFillColor(sf::Color(32, 43, 62));
            window.draw(sliderBg);

            sf::RectangleShape sliderFill(sf::Vector2f(sliderW * sliderRatio, 12));
            sliderFill.setPosition(detailX + 16.0f, sliderY);
            sliderFill.setFillColor(sliderColor);
            window.draw(sliderFill);

            float perfY = panelTop + 370.0f;
            float perfW = 790.0f;
            float perfH = 112.0f;
            float perfLeft = panelCX - perfW * 0.5f;

            sf::RectangleShape perfPanel(sf::Vector2f(perfW, perfH));
            perfPanel.setOrigin(perfPanel.getSize() * 0.5f);
            perfPanel.setPosition(panelCX, perfY);
            perfPanel.setFillColor(sf::Color(10, 22, 42, 220));
            perfPanel.setOutlineThickness(2.0f);
            perfPanel.setOutlineColor(sf::Color(90, 150, 225, 200));
            window.draw(perfPanel);

            sf::Text perfHeader;
            perfHeader.setFont(font);
            perfHeader.setCharacterSize(20);
            perfHeader.setStyle(sf::Text::Bold);
            perfHeader.setFillColor(sf::Color(190, 225, 255));
            perfHeader.setString("LIVE PERFORMANCE");
            perfHeader.setPosition(perfLeft + 16.0f, perfY - 47.0f);
            window.draw(perfHeader);

            std::stringstream fpsLive;
            fpsLive << std::fixed << std::setprecision(1) << smoothedFps;
            std::stringstream frameMsLive;
            frameMsLive << std::fixed << std::setprecision(2) << smoothedFrameMs;
            std::string targetText = fpsOptions[fpsIndex] == 0 ? "Unlimited" : std::to_string(fpsOptions[fpsIndex]) + " FPS";

            sf::Text perfLineA;
            perfLineA.setFont(font);
            perfLineA.setCharacterSize(19);
            perfLineA.setFillColor(sf::Color(210, 226, 245));
            perfLineA.setString("Live FPS: " + fpsLive.str() + "   |   Target: " + targetText);
            perfLineA.setPosition(perfLeft + 16.0f, perfY - 20.0f);
            window.draw(perfLineA);

            sf::Text perfLineB;
            perfLineB.setFont(font);
            perfLineB.setCharacterSize(18);
            perfLineB.setFillColor(fpsStatusColor());
            perfLineB.setString("Frame Time: " + frameMsLive.str() + " ms   |   " + fpsStatusText());
            perfLineB.setPosition(perfLeft + 16.0f, perfY + 6.0f);
            window.draw(perfLineB);

            std::string qualityProfile = "Balanced";
            if (qualityIndex == 0)
                qualityProfile = "Performance";
            else if (qualityIndex == 2)
                qualityProfile = "Cinematic";
            else if (qualityIndex == 3)
                qualityProfile = "Showcase";
            std::string trapProfile = trapDensityLabels[trapDensityIndex] + "/" + spikeTempoLabels[spikeTempoIndex];

            sf::Text perfLineC;
            perfLineC.setFont(font);
            perfLineC.setCharacterSize(16);
            perfLineC.setFillColor(sf::Color(160, 188, 220));
            perfLineC.setString("Render " + std::to_string(background.detailUnits()) + "u"
                                "   |   " + qualityProfile +
                                "   |   " + (hardcoreModeEnabled ? "Hardcore" : "Standard") +
                                "   |   Traps " + trapProfile +
                                "   |   Floors " + openFloorLabels[openFloorModeIndex]);
            perfLineC.setPosition(perfLeft + 16.0f, perfY + 31.0f);
            window.draw(perfLineC);

            sf::Text settingsHint;
            settingsHint.setFont(font);
            settingsHint.setCharacterSize(16);
            settingsHint.setFillColor(sf::Color(170, 185, 215));
            settingsHint.setString("Q/E or A/D: Category   Up/Down: Select\nLeft/Right: Change   Enter: Confirm   Esc: Back");
            sf::FloatRect sh = settingsHint.getLocalBounds();
            settingsHint.setOrigin(sh.left + sh.width * 0.5f, sh.top + sh.height * 0.5f);
            settingsHint.setPosition(panelCX, panelTop + panelH - 40.0f);
            window.draw(settingsHint);

            sf::Text saveHint;
            saveHint.setFont(font);
            saveHint.setCharacterSize(15);
            saveHint.setFillColor(sf::Color(130, 150, 190));
            saveHint.setString(settingsBannerTimer > 0.0f ? settingsBannerText : "Auto-saved in settings.cfg");
            sf::FloatRect sb = saveHint.getLocalBounds();
            saveHint.setOrigin(sb.left + sb.width * 0.5f, sb.top + sb.height * 0.5f);
            saveHint.setPosition(panelCX, panelTop + panelH - 17.0f);
            window.draw(saveHint);
        }
        else
        {
            float baseY = HEIGHT / 2.0f + 20.0f;
            float spacing = 55.0f;
            float pulse = 0.5f + 0.5f * std::sin(menuClock.getElapsedTime().asSeconds() * 4.0f);
            int glow = static_cast<int>(150 + 100 * pulse);

            for (int i = 0; i < (int)menuItems.size(); i++)
            {
                sf::Text item;
                item.setFont(font);
                item.setCharacterSize(32);
                item.setStyle(sf::Text::Bold);
                item.setString(menuItems[i]);
                if (i == menuIndex)
                    item.setFillColor(sf::Color(255, glow, 80));
                else
                    item.setFillColor(sf::Color(210, 210, 210));

                sf::FloatRect b = item.getLocalBounds();
                item.setOrigin(b.left + b.width / 2, b.top + b.height / 2);
                item.setPosition(WIDTH / 2, baseY + i * spacing);
                window.draw(item);

                if (i == menuIndex)
                {
                    sf::ConvexShape pointer;
                    pointer.setPointCount(3);
                    pointer.setPoint(0, sf::Vector2f(WIDTH / 2 - 150, baseY + i * spacing - 8));
                    pointer.setPoint(1, sf::Vector2f(WIDTH / 2 - 150, baseY + i * spacing + 8));
                    pointer.setPoint(2, sf::Vector2f(WIDTH / 2 - 135, baseY + i * spacing));
                    pointer.setFillColor(sf::Color(255, glow, 80));
                    window.draw(pointer);
                }
            }

            window.draw(hintText);
            if (menuBannerTimer > 0.0f)
            {
                sf::Text menuStatus;
                menuStatus.setFont(font);
                menuStatus.setCharacterSize(17);
                menuStatus.setFillColor(sf::Color(255, 180, 130));
                menuStatus.setString(menuBannerText.substr(0, 62));
                sf::FloatRect mb = menuStatus.getLocalBounds();
                menuStatus.setOrigin(mb.left + mb.width * 0.5f, mb.top + mb.height * 0.5f);
                menuStatus.setPosition(WIDTH * 0.5f, HEIGHT - 78.0f);
                window.draw(menuStatus);
            }
        }

        window.display();
    }
    saveCurrentSettings();

    while (window.isOpen())
    {
        dt = clock.restart().asSeconds();
        updatePerfStats(dt);
        if (!backToMenuPrompt)
        {
            if (shakeTime > 0.0f)
            {
                shakeTime -= dt;
                if (shakeTime < 0.0f)
                    shakeTime = 0.0f;
            }
            weaponBannerTimer = std::max(0.0f, weaponBannerTimer - dt);
            levelIntroTimer = std::max(0.0f, levelIntroTimer - dt);
            if (levelIntroPending)
            {
                refreshLevelIntro();
                levelIntroPending = false;
            }
            activateWeaponSelectIfNeeded();
        }

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::Resized)
            {
                applyLetterboxView(worldView,
                                   static_cast<unsigned int>(event.size.width),
                                   static_cast<unsigned int>(event.size.height));
                window.setView(worldView);
            }
            if (event.type == sf::Event::KeyPressed && backToMenuPrompt)
            {
                if (event.key.code == sf::Keyboard::Left ||
                    event.key.code == sf::Keyboard::Up ||
                    event.key.code == sf::Keyboard::A ||
                    event.key.code == sf::Keyboard::W)
                {
                    backToMenuChoice = (backToMenuChoice + 2) % 3;
                }
                else if (event.key.code == sf::Keyboard::Right ||
                         event.key.code == sf::Keyboard::Down ||
                         event.key.code == sf::Keyboard::D ||
                         event.key.code == sf::Keyboard::S ||
                         event.key.code == sf::Keyboard::Tab)
                {
                    backToMenuChoice = (backToMenuChoice + 1) % 3;
                }
                else if (event.key.code == sf::Keyboard::Enter ||
                         event.key.code == sf::Keyboard::Space)
                {
                    if (backToMenuChoice == 0)
                    {
                        returnToMenuRequested = true;
                    }
                    else if (backToMenuChoice == 2)
                    {
                        bool saved = saveRunProgress();
                        weaponBannerText = saved ? "Game Saved" : "Save Failed";
                        weaponBannerTimer = 2.4f;
                        sound.play(saved ? "powerup" : "hit");
                    }
                    backToMenuPrompt = false;
                }
                else if (event.key.code == sf::Keyboard::Escape ||
                         event.key.code == sf::Keyboard::Backspace)
                {
                    backToMenuPrompt = false;
                }
                continue;
            }
            if (weaponSelectActive && !gameOver && !levelComplete)
            {
                if (event.type == sf::Event::TextEntered)
                {
                    sf::Uint32 cp = event.text.unicode;
                    int digit = -1;
                    if (cp >= '1' && cp <= '5')
                        digit = static_cast<int>(cp - '1');
                    else if (cp >= 0x0661 && cp <= 0x0665) // Arabic-Indic digits: ١-٥
                        digit = static_cast<int>(cp - 0x0661);
                    else if (cp >= 0x06F1 && cp <= 0x06F5) // Eastern Arabic digits: ۱-۵
                        digit = static_cast<int>(cp - 0x06F1);

                    if (digit >= 0 && digit < static_cast<int>(level2Weapons.size()))
                    {
                        weaponSelectIndex = digit;
                        equipWeapon(weaponSelectIndex, true);
                    }
                }
                else if (event.type == sf::Event::KeyPressed)
                {
                    if (event.key.code == sf::Keyboard::Up ||
                        event.key.code == sf::Keyboard::W ||
                        event.key.code == sf::Keyboard::Left ||
                        event.key.code == sf::Keyboard::A)
                    {
                        weaponSelectIndex = (weaponSelectIndex + static_cast<int>(level2Weapons.size()) - 1) % static_cast<int>(level2Weapons.size());
                    }
                    else if (event.key.code == sf::Keyboard::Down ||
                             event.key.code == sf::Keyboard::S ||
                             event.key.code == sf::Keyboard::Right ||
                             event.key.code == sf::Keyboard::D)
                    {
                        weaponSelectIndex = (weaponSelectIndex + 1) % static_cast<int>(level2Weapons.size());
                    }
                    else if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num5)
                    {
                        weaponSelectIndex = static_cast<int>(event.key.code - sf::Keyboard::Num1);
                        equipWeapon(weaponSelectIndex, true);
                    }
                    else if (event.key.code >= sf::Keyboard::Numpad1 && event.key.code <= sf::Keyboard::Numpad5)
                    {
                        weaponSelectIndex = static_cast<int>(event.key.code - sf::Keyboard::Numpad1);
                        equipWeapon(weaponSelectIndex, true);
                    }
                    else if (event.key.code == sf::Keyboard::Enter ||
                             event.key.code == sf::Keyboard::Space)
                    {
                        equipWeapon(weaponSelectIndex, true);
                    }
                    else if (event.key.code == sf::Keyboard::Escape ||
                             event.key.code == sf::Keyboard::Backspace ||
                             event.key.code == sf::Keyboard::Tab)
                    {
                        equipWeapon(weaponSelectIndex, true);
                    }
                }
                continue;
            }

            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Escape &&
                    !gameOver && !levelComplete && !weaponSelectActive)
                {
                    backToMenuPrompt = true;
                    backToMenuChoice = 1; // default on "No"
                    player.gunCharging = false;
                    player.gunCharge = 0.0f;
                    continue;
                }
                if (!gameOver && !levelComplete)
                {
                    if (levelHasGiantEnemy() || weaponLoadoutLevel == currentLevel->levelNumber)
                    {
                        if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num5)
                        {
                            equipWeapon(static_cast<int>(event.key.code - sf::Keyboard::Num1), true);
                            continue;
                        }
                        if (event.key.code >= sf::Keyboard::Numpad1 && event.key.code <= sf::Keyboard::Numpad5)
                        {
                            equipWeapon(static_cast<int>(event.key.code - sf::Keyboard::Numpad1), true);
                            continue;
                        }
                        if (event.key.code == sf::Keyboard::Tab)
                        {
                            weaponSelectActive = true;
                            weaponSelectIndex = equippedWeaponIndex;
                            player.gunCharging = false;
                            player.gunCharge = 0.0f;
                            continue;
                        }
                    }

                    if (event.key.code == sf::Keyboard::Space ||
                        event.key.code == sf::Keyboard::Up ||
                        event.key.code == sf::Keyboard::W)
                    {
                        if (player.jump())
                        {
                            particles.addJumpDust(player.position + sf::Vector2f(0, player.size));
                            sound.play("jump"); // Sound effect!
                        }
                    }
                    if (event.key.code == sf::Keyboard::LShift ||
                        event.key.code == sf::Keyboard::RShift)
                    {
                        player.dash();
                        sound.play("dash"); // Sound effect!
                    }
                    if (event.key.code == sf::Keyboard::Z)
                    {
                        if (player.startSlash())
                        {
                            float dir = player.facingRight ? 1.0f : -1.0f;
                            sf::Vector2f fxPos = player.position + sf::Vector2f(dir * player.size * 1.2f, -player.size * 0.2f);
                            particles.addExplosion(fxPos, player.accentColor(), 14);
                            sound.play("slash");
                        }
                    }
                    if (event.key.code == sf::Keyboard::X)
                    {
                        if (!player.isFocusing() && player.hasGun && player.gunCooldown <= 0.0f && !player.gunCharging)
                        {
                            player.gunCharging = true;
                            player.gunCharge = 0.0f;
                        }
                    }
                }

                if (event.key.code == sf::Keyboard::R && (gameOver || levelComplete))
                {
                    if (gameOver)
                    {
                        player = Player();
                        player.setStyle(characterStyleIndex);
                        currentLevel = std::make_unique<Level>(1);
                        levelIntroPending = true;
                        runTimer = 0.0f;
                        deathCount = 0;
                        weaponLoadoutLevel = -1;
                        weaponSelectActive = false;
                        weaponBannerTimer = 0.0f;
                        shotgunOrbitAngle = 0.0f;
                        gameOver = false;
                        bullets.clear();
                        resetLevelSessionStats();
                    }
                    else if (levelComplete)
                    {
                        player.respawn();
                        currentLevel = std::make_unique<Level>(currentLevel->levelNumber + 1);
                        levelIntroPending = true;
                        weaponSelectActive = false;
                        weaponSelectIndex = equippedWeaponIndex;
                        levelComplete = false;
                        levelCompleteTimer = 0;
                        bullets.clear();
                        resetLevelSessionStats();
                    }
                }
            }
            else if (event.type == sf::Event::KeyReleased)
            {
                if (!gameOver && !levelComplete)
                {
                    if (event.key.code == sf::Keyboard::X && player.gunCharging)
                    {
                        const WeaponLoadout &weapon = activeWeapon();
                        float t = std::min(1.0f, player.gunCharge / GUN_CHARGE_MAX);
                        float dir = player.facingRight ? 1.0f : -1.0f;
                        sf::Vector2f forward(dir, 0.0f);
                        sf::Vector2f right(0.0f, 1.0f);
                        sf::Vector2f basePos = player.position + sf::Vector2f(dir * (player.size * 1.1f), -player.size * 0.2f);

                        if (isOrbitShotgun(weapon))
                        {
                            float orbitRadius = player.size * 2.35f;
                            sf::Vector2f orbitDir(std::cos(shotgunOrbitAngle), std::sin(shotgunOrbitAngle));
                            float orbitLen = std::hypot(orbitDir.x, orbitDir.y);
                            if (orbitLen < 0.001f)
                            {
                                orbitDir = sf::Vector2f(dir, 0.0f);
                                orbitLen = 1.0f;
                            }
                            orbitDir /= orbitLen;
                            forward = orbitDir;
                            right = sf::Vector2f(-forward.y, forward.x);
                            basePos = player.position + orbitDir * orbitRadius + sf::Vector2f(0.0f, -player.size * 0.15f);
                            player.facingRight = (forward.x >= 0.0f);
                        }

                        if (t < 0.25f)
                        {
                            int pellets = std::max(1, weapon.tapPellets);
                            float center = (static_cast<float>(pellets) - 1.0f) * 0.5f;
                            for (int i = 0; i < pellets; i++)
                            {
                                float spreadIndex = static_cast<float>(i) - center;
                                Bullet b;
                                b.size = 3.5f * weapon.sizeMul;
                                b.lifetime = BULLET_LIFETIME * weapon.lifetimeMul;
                                b.damage = std::max(1, weapon.tapDamage);
                                b.pierceLeft = std::max(0, weapon.basePierce);
                                b.knockbackX = 200.0f * weapon.knockbackMul;
                                b.knockbackY = -150.0f * weapon.knockbackMul;
                                b.color = weapon.tapColor;
                                b.position = basePos + right * (spreadIndex * 5.0f);
                                b.velocity = forward * (BULLET_SPEED * weapon.speedMul * 0.9f) + right * (spreadIndex * weapon.tapSpreadVel);
                                bullets.push_back(b);
                            }
                        }
                        else
                        {
                            int pellets = std::max(1, weapon.chargedPellets);
                            float center = (static_cast<float>(pellets) - 1.0f) * 0.5f;
                            int chargeDamage = std::max(1, weapon.chargeBaseDamage +
                                                               static_cast<int>(std::round(weapon.chargeBonusDamage * t)));
                            for (int i = 0; i < pellets; i++)
                            {
                                float spreadIndex = static_cast<float>(i) - center;
                                Bullet b;
                                b.size = (4.0f + 6.0f * t) * weapon.sizeMul;
                                b.lifetime = (BULLET_LIFETIME + 0.3f * t) * weapon.lifetimeMul;
                                b.damage = std::max(1, chargeDamage - (pellets > 1 ? 1 : 0));
                                b.pierceLeft = std::max(0, weapon.basePierce + (t > 0.75f ? weapon.chargePierceBonus : 0));
                                b.knockbackX = (220.0f + 60.0f * t) * weapon.knockbackMul;
                                b.knockbackY = -160.0f * weapon.knockbackMul;
                                b.color = (t > 0.65f) ? weapon.chargeColor : weapon.tapColor;
                                b.position = basePos + right * (spreadIndex * 4.0f);
                                b.velocity = forward * (BULLET_SPEED * weapon.speedMul * (1.0f + 0.2f * t)) +
                                             right * (spreadIndex * weapon.chargedSpreadVel);
                                bullets.push_back(b);
                            }
                        }

                        player.gunCharging = false;
                        player.gunCharge = 0.0f;
                        player.gunCooldown = weapon.cooldown;
                    }
                }
            }
        }

        if (returnToMenuRequested)
        {
            returnToMenuRequested = false;
            backToMenuPrompt = false;
            resetRunForNewGame();
            showMenu = true;
            showControls = false;
            showSettings = false;
            menuIndex = 0;
            settingsTabIndex = 0;
            settingsIndex = 0;
            normalizeSettingsSelection();
            menuClock.restart();
            clock.restart();
            goto main_menu_loop;
        }

        if (!gameOver && !levelComplete && !weaponSelectActive && !backToMenuPrompt)
        {
            runTimer += dt;
            background.update(dt);
            if (player.hasGun && isOrbitShotgun(activeWeapon()))
            {
                float orbitSpeed = player.gunCharging ? 5.2f : 3.8f;
                shotgunOrbitAngle += dt * orbitSpeed;
                if (shotgunOrbitAngle > PI * 2.0f)
                    shotgunOrbitAngle -= PI * 2.0f;
            }
            int livesBeforeFrame = player.lives;
            player.update(dt, currentLevel->platforms, particles);
            levelBestCombo = std::max(levelBestCombo, player.comboMultiplier);
            float directorBias = 1.0f;
            if (ADAPTIVE_DIRECTOR)
            {
                float comboPressure = std::max(0.0f, std::min(0.75f, (player.comboMultiplier - 1.0f) * 0.28f));
                float survivalEdge = std::max(-0.35f, std::min(0.42f, (player.lives - 3.0f) * 0.07f));
                float runEscalation = std::max(0.0f, std::min(0.72f, runTimer / 240.0f));
                float scoreEscalation = std::max(0.0f, std::min(0.48f, static_cast<float>(player.score) / 22000.0f));
                float deathRelief = std::max(0.0f, std::min(0.56f, deathCount * 0.035f));
                float levelEscalation = std::max(0.0f, std::min(0.32f, (currentLevel->levelNumber - 1) * 0.03f));
                directorBias = 1.0f + comboPressure + survivalEdge + runEscalation + scoreEscalation + levelEscalation - deathRelief;
                if (currentLevel->activeBoss())
                    directorBias += 0.08f;
                directorBias = std::max(0.78f, std::min(1.52f, directorBias));
            }
            currentLevel->setDirectorBias(directorBias);
            currentLevel->update(dt, player, particles, popups, triggerShake);
            if (player.lives < livesBeforeFrame)
            {
                levelHitsTaken += (livesBeforeFrame - player.lives);
                sound.play("hit");
            }
            float levelElapsed = runTimer - levelStartRunTime;
            if (ADAPTIVE_DIRECTOR && !levelSecondWindUsed && levelElapsed > 42.0f && player.lives <= 1)
            {
                bool pressureState = (currentLevel->directorValue() <= 0.98f) || (currentLevel->difficultyValue() >= 1.75f);
                if (pressureState)
                {
                    player.lives = std::min(MAX_LIVES, player.lives + 1);
                    player.invincibleTimer = std::max(player.invincibleTimer, 1.0f);
                    player.addSoul(FOCUS_COST * 0.5f);
                    levelSecondWindUsed = true;
                    pushPopup(player.position + sf::Vector2f(0, -50), "SECOND WIND", sf::Color(130, 245, 180));
                    particles.addCollectEffect(player.position + sf::Vector2f(0, -12), sf::Color(130, 245, 180));
                    triggerShake(0.10f, 6.0f);
                    sound.play("powerup");
                }
            }
            particles.update(dt);
            for (auto it = popups.begin(); it != popups.end();)
            {
                it->life -= dt;
                it->pos += it->vel * dt;
                it->vel *= 0.98f;
                if (it->life <= 0)
                    it = popups.erase(it);
                else
                    ++it;
            }

            // Update bullets
            for (auto it = bullets.begin(); it != bullets.end();)
            {
                it->lifetime -= dt;
                it->position += it->velocity * dt;
                if (it->lifetime <= 0 ||
                    it->position.x < -20 || it->position.x > WIDTH + 20 ||
                    it->position.y < -20 || it->position.y > HEIGHT + 20)
                {
                    it = bullets.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            // Bullet vs Enemies
            for (auto bIt = bullets.begin(); bIt != bullets.end();)
            {
                bool bulletRemoved = false;
                sf::FloatRect bBounds(bIt->position.x - bIt->size, bIt->position.y - bIt->size,
                                      bIt->size * 2, bIt->size * 2);
                for (auto eIt = currentLevel->enemies.begin(); eIt != currentLevel->enemies.end();)
                {
                    if (bBounds.intersects((*eIt)->getBounds()))
                    {
                        float knockDir = (bIt->velocity.x > 0) ? 1.0f : -1.0f;
                        sound.play("hit");
                        (*eIt)->takeDamage(bIt->damage, sf::Vector2f(bIt->knockbackX * knockDir, bIt->knockbackY));
                        player.addSoul(SOUL_PER_HIT * 0.55f);
                        particles.addExplosion(bIt->position, bIt->color, 12);
                        if (!(*eIt)->alive)
                        {
                            sound.play("coin");
                            int gained = player.addScore(200);
                            pushPopup((*eIt)->position, "+" + std::to_string(gained), sf::Color::Yellow);
                            triggerShake(0.16f, 9.0f);
                            eIt = currentLevel->enemies.erase(eIt);
                        }
                        else
                        {
                            ++eIt;
                        }
                        if (bIt->pierceLeft > 0)
                        {
                            bIt->pierceLeft--;
                            bIt->position.x += knockDir * 8.0f;
                        }
                        else
                        {
                            bulletRemoved = true;
                        }
                        break;
                    }
                    else
                    {
                        ++eIt;
                    }
                }
                if (bulletRemoved)
                    bIt = bullets.erase(bIt);
                else
                    ++bIt;
            }

            // Slash vs Enemies (includes pogo down-slash)
            if (player.slashing)
            {
                sf::FloatRect slashBox;
                if (player.slashDown)
                {
                    slashBox = sf::FloatRect(player.position.x - SLASH_HEIGHT * 0.5f,
                                             player.position.y,
                                             SLASH_HEIGHT,
                                             SLASH_RANGE);
                }
                else
                {
                    float dir = player.facingRight ? 1.0f : -1.0f;
                    slashBox = sf::FloatRect(player.position.x + (dir > 0 ? player.size : -(SLASH_RANGE + player.size)),
                                             player.position.y - SLASH_HEIGHT * 0.5f,
                                             SLASH_RANGE,
                                             SLASH_HEIGHT);
                }

                for (auto eIt = currentLevel->enemies.begin(); eIt != currentLevel->enemies.end();)
                {
                    if (slashBox.intersects((*eIt)->getBounds()))
                    {
                        float dir = player.slashDown ? 0.0f : (player.facingRight ? 1.0f : -1.0f);
                        sound.play("hit");
                        (*eIt)->takeDamage(1, sf::Vector2f(220.0f * dir, -160.0f));
                        player.addSoul(SOUL_PER_HIT);
                        particles.addExplosion((*eIt)->position, sf::Color(150, 220, 255), 10);
                        if (player.slashDown)
                        {
                            player.velocity.y = POGO_BOUNCE;
                            player.onGround = false;
                        }
                        if (!(*eIt)->alive)
                        {
                            sound.play("coin");
                            int gained = player.addScore(220);
                            pushPopup((*eIt)->position, "+" + std::to_string(gained), sf::Color::Yellow);
                            triggerShake(0.14f, 7.5f);
         
                   eIt = currentLevel->enemies.erase(eIt);
                        }
                        else
                        {
                            ++eIt;
                        }
                        player.slashing = false;
                        player.slashDown = false;
                        break;
                    }
                    else
                    {
                        ++eIt;
                    }
                }
            }

            // Shade pickup
            if (shade.active)
            {
                if (shade.value <= 0)
                    shade.active = false;
                else if (player.getBounds().intersects(sf::FloatRect(shade.pos.x - 16, shade.pos.y - 16, 32, 32)))
                {
                    player.score += shade.value;
                    shade.active = false;
                    particles.addCollectEffect(player.position, sf::Color(80, 120, 180));
                    sound.play("powerup");
                }
            }

            // Bench interaction
            for (auto &b : currentLevel->benches)
            {
                sf::FloatRect near(b.left - BENCH_PROMPT_RANGE, b.top - BENCH_PROMPT_RANGE,
                                   b.width + BENCH_PROMPT_RANGE * 2.0f, b.height + BENCH_PROMPT_RANGE * 2.0f);
                if (near.intersects(player.getBounds()))
                {
                    player.respawnPoint = sf::Vector2f(b.left + b.width * 0.5f, b.top - player.size);
                    break;
                }
            }

            if (player.lives <= 0)
            {
                deathCount++;
                int drop = std::max(1, player.score / 2);
                player.score -= drop;
                shade.active = true;
                shade.pos = player.position;
                shade.value = drop;
                player.lives = 3;
                player.invincibleTimer = 1.0f;
                player.respawn();
            }

            if (currentLevel->isCompleted())
            {
                finalizeLevelReport();
                levelComplete = true;
                levelCompleteTimer = 0;
            }
        }
        else if (levelComplete)
        {
            levelCompleteTimer += dt;
            particles.update(dt);
        }
        highScore = std::max(highScore, player.score);

        // Render
        window.clear(currentLevel->backgroundColor);

        sf::View view = worldView;
        if (screenShakeEnabled && shakeTime > 0.0f && shakeDuration > 0.0f)
        {
            float power = shakeTime / shakeDuration;
            float mag = shakeMagnitude * power;
            float offsetX = (static_cast<float>(rand() % 200) / 100.0f - 1.0f) * mag;
            float offsetY = (static_cast<float>(rand() % 200) / 100.0f - 1.0f) * mag;
            view.move(offsetX, offsetY);
        }
        window.setView(view);

        background.draw(window);
        currentLevel->draw(window);
        sf::Uint8 moodAlpha = 56;
        if (QUALITY_LEVEL == 0)
            moodAlpha = 34;
        else if (QUALITY_LEVEL == 2)
            moodAlpha = 72;
        else if (QUALITY_LEVEL >= 3)
            moodAlpha = 88;
        sf::RectangleShape moodOverlay(sf::Vector2f(WIDTH, HEIGHT));
        moodOverlay.setFillColor(sf::Color(8, 12, 22, moodAlpha));
        window.draw(moodOverlay);

        if (QUALITY_LEVEL >= 2)
        {
            float auraRadius = QUALITY_LEVEL >= 3 ? 250.0f : 205.0f;
            float auraPulse = 0.72f + 0.28f * std::sin(menuClock.getElapsedTime().asSeconds() * 1.6f);
            sf::CircleShape aura(auraRadius);
            aura.setOrigin(auraRadius, auraRadius);
            aura.setPosition(player.position.x, player.position.y - player.size * 0.35f);
            aura.setFillColor(sf::Color(90, 140, 220, static_cast<sf::Uint8>((QUALITY_LEVEL >= 3 ? 34.0f : 23.0f) * auraPulse)));
            window.draw(aura);
        }

        if (shade.active)
        {
            sf::CircleShape sh(14.f);
            sh.setOrigin(sh.getRadius(), sh.getRadius());
            sh.setPosition(shade.pos);
            sh.setFillColor(sf::Color(20, 20, 30, 200));
            sh.setOutlineThickness(2);
            sh.setOutlineColor(sf::Color(200, 200, 240));
            window.draw(sh);

            sf::CircleShape eye(3.f);
            eye.setFillColor(sf::Color(220, 220, 255));
            eye.setPosition(shade.pos.x - 6.f, shade.pos.y - 4.f);
            window.draw(eye);
            eye.setPosition(shade.pos.x + 2.f, shade.pos.y - 4.f);
            window.draw(eye);
        }

        player.draw(window);
        if (player.hasGun && player.gunTimer > 0.0f && isOrbitShotgun(activeWeapon()) && !weaponSelectActive)
        {
            sf::Vector2f orbitDir(std::cos(shotgunOrbitAngle), std::sin(shotgunOrbitAngle));
            float orbitRadius = player.size * 2.35f;
            sf::Vector2f gunPos = player.position + orbitDir * orbitRadius + sf::Vector2f(0.0f, -player.size * 0.15f);
            sf::Vector2f perp(-orbitDir.y, orbitDir.x);

            sf::CircleShape orbitRing(orbitRadius);
            orbitRing.setOrigin(orbitRadius, orbitRadius);
            orbitRing.setPosition(player.position);
            orbitRing.setFillColor(sf::Color(0, 0, 0, 0));
            orbitRing.setOutlineThickness(1.5f);
            orbitRing.setOutlineColor(sf::Color(255, 205, 130, 85));
            window.draw(orbitRing);

            sf::CircleShape aura(player.size * 0.62f);
            aura.setOrigin(aura.getRadius(), aura.getRadius());
            aura.setPosition(gunPos);
            aura.setFillColor(sf::Color(255, 150, 80, 95));
            window.draw(aura);

            sf::RectangleShape body(sf::Vector2f(player.size * 1.45f, player.size * 0.38f));
            body.setOrigin(body.getSize() * 0.5f);
            body.setPosition(gunPos);
            float gunAngle = std::atan2(orbitDir.y, orbitDir.x) * 180.0f / PI;
            body.setRotation(gunAngle);
            body.setFillColor(sf::Color(65, 48, 38));
            body.setOutlineThickness(1.5f);
            body.setOutlineColor(sf::Color(255, 205, 135));
            window.draw(body);

            sf::RectangleShape barrel(sf::Vector2f(player.size * 0.62f, player.size * 0.16f));
            barrel.setOrigin(barrel.getSize().x * 0.1f, barrel.getSize().y * 0.5f);
            barrel.setPosition(gunPos + orbitDir * (player.size * 0.72f) + perp * (player.size * 0.03f));
            barrel.setRotation(gunAngle);
            barrel.setFillColor(sf::Color(255, 184, 110));
            window.draw(barrel);
        }
        for (auto &b : bullets)
        {
            if (QUALITY_LEVEL >= 1)
            {
                sf::CircleShape glow(b.size * 1.6f);
                glow.setPosition(b.position - sf::Vector2f(b.size * 1.6f, b.size * 1.6f));
                glow.setFillColor(sf::Color(b.color.r, b.color.g, b.color.b, 80));
                window.draw(glow);
            }

            if (QUALITY_LEVEL >= 3)
            {
                sf::CircleShape aura(b.size * 2.6f);
                aura.setPosition(b.position - sf::Vector2f(b.size * 2.6f, b.size * 2.6f));
                aura.setFillColor(sf::Color(b.color.r, b.color.g, b.color.b, 35));
                window.draw(aura);
            }

            sf::CircleShape bullet(b.size);
            bullet.setPosition(b.position - sf::Vector2f(b.size, b.size));
            bullet.setFillColor(b.color);
            bullet.setOutlineThickness(1);
            bullet.setOutlineColor(sf::Color::White);
            window.draw(bullet);
        }
        particles.draw(window);
        for (auto &p : popups)
        {
            sf::Text t;
            t.setFont(font);
            t.setCharacterSize(18);
            float alpha = 255.0f * std::max(0.0f, p.life / p.maxLife);
            t.setFillColor(sf::Color(p.color.r, p.color.g, p.color.b, static_cast<sf::Uint8>(alpha)));
            t.setString(p.text);
            t.setPosition(p.pos);
            window.draw(t);
        }

        window.setView(worldView);

        // UI
        sf::RectangleShape uiBar(sf::Vector2f(WIDTH, 60));
        uiBar.setFillColor(sf::Color(0, 0, 0, 180));
        window.draw(uiBar);

        sf::Text scoreText;
        scoreText.setFont(font);
        scoreText.setCharacterSize(24);
        scoreText.setFillColor(sf::Color(0, 255, 200));
        scoreText.setStyle(sf::Text::Bold);
        std::stringstream ss;
        ss << "SCORE: " << player.score << "  |  COINS: " << player.coins
           << "  |  LEVEL: " << currentLevel->levelNumber;
        scoreText.setString(ss.str());
        scoreText.setPosition(20, 15);
        window.draw(scoreText);

        std::stringstream perfHudLine;
        perfHudLine << std::fixed << std::setprecision(1) << "FPS " << smoothedFps << "  |  Target "
                    << (fpsOptions[fpsIndex] == 0 ? "Unlimited" : std::to_string(fpsOptions[fpsIndex]))
                    << "  |  " << qualityLabels[qualityIndex];
        sf::Text perfHudText;
        perfHudText.setFont(font);
        perfHudText.setCharacterSize(15);
        perfHudText.setFillColor(fpsStatusColor());
        perfHudText.setString(perfHudLine.str());
        sf::FloatRect pbb = perfHudText.getLocalBounds();
        perfHudText.setOrigin(pbb.left + pbb.width * 0.5f, pbb.top);
        perfHudText.setPosition(WIDTH * 0.53f, 8.0f);
        window.draw(perfHudText);

        int totalCentis = static_cast<int>(runTimer * 100.0f);
        int minutes = totalCentis / 6000;
        int seconds = (totalCentis / 100) % 60;
        int centis = totalCentis % 100;
        std::stringstream diffLevelText;
        diffLevelText << std::fixed << std::setprecision(2) << currentLevel->difficultyValue();
        std::stringstream directorText;
        directorText << std::fixed << std::setprecision(2) << currentLevel->directorValue();
        std::stringstream challengeHud;
        challengeHud << "TIME "
                     << std::setw(2) << std::setfill('0') << minutes << ":"
                     << std::setw(2) << std::setfill('0') << seconds << "."
                     << std::setw(2) << std::setfill('0') << centis
                     << "  |  DEATHS " << deathCount
                     << "  |  " << (HARDCORE_MODE ? "HARDCORE" : "STANDARD")
                     << (DIE_AGAIN_MODE ? " + DIE AGAIN" : "")
                     << "  |  " << currentLevel->difficultyName() << " x" << diffLevelText.str();
        if (ADAPTIVE_DIRECTOR)
        {
            challengeHud << "  |  DIR " << currentLevel->directorMood() << " x" << directorText.str();
        }
        else
        {
            challengeHud << "  |  DIR FIXED";
        }
        sf::Text challengeHudText;
        challengeHudText.setFont(font);
        challengeHudText.setCharacterSize(14);
        sf::Color difficultyColor = sf::Color(160, 200, 230);
        if (currentLevel->difficultyValue() >= 2.3f)
            difficultyColor = sf::Color(255, 130, 130);
        else if (currentLevel->difficultyValue() >= 1.8f)
            difficultyColor = sf::Color(255, 175, 120);
        else if (currentLevel->difficultyValue() >= 1.4f)
            difficultyColor = sf::Color(255, 215, 130);
        challengeHudText.setFillColor(
            HARDCORE_MODE ? sf::Color(255, 145, 145)
                          : (DIE_AGAIN_MODE ? sf::Color(255, 188, 138) : difficultyColor));
        challengeHudText.setString(challengeHud.str());
        sf::FloatRect chb = challengeHudText.getLocalBounds();
        challengeHudText.setOrigin(chb.left + chb.width * 0.5f, chb.top);
        challengeHudText.setPosition(WIDTH * 0.52f, 28.0f);
        window.draw(challengeHudText);

        Enemy *boss = currentLevel->activeBoss();
        if (boss)
        {
            float bossRatio = std::max(0.0f, std::min(1.0f, static_cast<float>(boss->health) / std::max(1, boss->baseHealth)));
            sf::RectangleShape bossBg(sf::Vector2f(430, 16));
            bossBg.setOrigin(bossBg.getSize().x * 0.5f, 0.0f);
            bossBg.setPosition(WIDTH * 0.5f, 56.0f);
            bossBg.setFillColor(sf::Color(26, 10, 16, 230));
            bossBg.setOutlineThickness(2.0f);
            bossBg.setOutlineColor(sf::Color(255, 120, 140));
            window.draw(bossBg);

            sf::RectangleShape bossFill(sf::Vector2f(430 * bossRatio, 16));
            bossFill.setPosition(WIDTH * 0.5f - 215.0f, 56.0f);
            bossFill.setFillColor(sf::Color(255, 75, 95));
            window.draw(bossFill);

            sf::Text bossText;
            bossText.setFont(font);
            bossText.setCharacterSize(15);
            bossText.setStyle(sf::Text::Bold);
            bossText.setFillColor(sf::Color(255, 200, 210));
            if (boss->type == 4)
                bossText.setString("INFERNO BEHEMOTH");
            else if (boss->type == 3)
                bossText.setString("TITAN BOSS");
            else
                bossText.setString("BOSS");
            sf::FloatRect bt = bossText.getLocalBounds();
            bossText.setOrigin(bt.left + bt.width * 0.5f, bt.top + bt.height);
            bossText.setPosition(WIDTH * 0.5f, 53.0f);
            window.draw(bossText);
        }

        int stageRelicsTotal = std::max(1, levelStartRelicCount);
        int stageRelicsCollected = std::max(0, levelStartRelicCount - currentLevel->remainingRelics());
        int stageEnemiesTotal = std::max(1, levelStartEnemyCount);
        int stageEnemiesDefeated = std::max(0, levelStartEnemyCount - currentLevel->enemyCount());
        float stageRelicsRatio = clamp01(static_cast<float>(stageRelicsCollected) / stageRelicsTotal);
        float stageEnemyRatio = clamp01(static_cast<float>(stageEnemiesDefeated) / stageEnemiesTotal);
        int stageSeconds = static_cast<int>(std::max(0.0f, runTimer - levelStartRunTime));
        int stageMin = stageSeconds / 60;
        int stageSec = stageSeconds % 60;

        sf::RectangleShape missionPanel(sf::Vector2f(304, 128));
        missionPanel.setPosition(WIDTH - 318.0f, 74.0f);
        missionPanel.setFillColor(sf::Color(10, 22, 38, 220));
        missionPanel.setOutlineThickness(2.0f);
        missionPanel.setOutlineColor(sf::Color(92, 162, 235, 210));
        window.draw(missionPanel);

        sf::Text missionTitle;
        missionTitle.setFont(font);
        missionTitle.setCharacterSize(17);
        missionTitle.setStyle(sf::Text::Bold);
        missionTitle.setFillColor(sf::Color(200, 228, 255));
        missionTitle.setString("MISSION TRACKER");
        missionTitle.setPosition(WIDTH - 304.0f, 80.0f);
        window.draw(missionTitle);

        sf::Text missionChapter;
        missionChapter.setFont(font);
        missionChapter.setCharacterSize(14);
        missionChapter.setFillColor(sf::Color(156, 196, 232));
        missionChapter.setString(currentLevel->chapterName().substr(0, 28));
        missionChapter.setPosition(WIDTH - 304.0f, 102.0f);
        window.draw(missionChapter);

        sf::Text relicLine;
        relicLine.setFont(font);
        relicLine.setCharacterSize(13);
        relicLine.setFillColor(sf::Color(255, 223, 138));
        relicLine.setString("Relics " + std::to_string(stageRelicsCollected) + "/" + std::to_string(stageRelicsTotal));
        relicLine.setPosition(WIDTH - 304.0f, 122.0f);
        window.draw(relicLine);

        sf::RectangleShape relicBarBg(sf::Vector2f(182, 7));
        relicBarBg.setPosition(WIDTH - 170.0f, 128.0f);
        relicBarBg.setFillColor(sf::Color(36, 50, 74, 220));
        window.draw(relicBarBg);
        sf::RectangleShape relicBar(sf::Vector2f(182 * stageRelicsRatio, 7));
        relicBar.setPosition(WIDTH - 170.0f, 128.0f);
        relicBar.setFillColor(sf::Color(255, 210, 120));
        window.draw(relicBar);

        sf::Text enemyLine;
        enemyLine.setFont(font);
        enemyLine.setCharacterSize(13);
        enemyLine.setFillColor(sf::Color(255, 164, 164));
        enemyLine.setString("Targets " + std::to_string(stageEnemiesDefeated) + "/" + std::to_string(stageEnemiesTotal));
        enemyLine.setPosition(WIDTH - 304.0f, 142.0f);
        window.draw(enemyLine);

        sf::RectangleShape enemyBarBg(sf::Vector2f(182, 7));
        enemyBarBg.setPosition(WIDTH - 170.0f, 148.0f);
        enemyBarBg.setFillColor(sf::Color(36, 50, 74, 220));
        window.draw(enemyBarBg);
        sf::RectangleShape enemyBar(sf::Vector2f(182 * stageEnemyRatio, 7));
        enemyBar.setPosition(WIDTH - 170.0f, 148.0f);
        enemyBar.setFillColor(sf::Color(255, 118, 138));
        window.draw(enemyBar);

        sf::Text missionSupport;
        missionSupport.setFont(font);
        missionSupport.setCharacterSize(13);
        missionSupport.setFillColor(levelSecondWindUsed ? sf::Color(135, 245, 180) : sf::Color(158, 186, 218));
        if (!ADAPTIVE_DIRECTOR)
            missionSupport.setString("Support: FIXED DIRECTOR");
        else
            missionSupport.setString(levelSecondWindUsed ? "Support: SECOND WIND USED" : "Support: READY");
        missionSupport.setPosition(WIDTH - 304.0f, 163.0f);
        window.draw(missionSupport);

        sf::Text missionTime;
        missionTime.setFont(font);
        missionTime.setCharacterSize(13);
        missionTime.setFillColor(sf::Color(168, 198, 228));
        missionTime.setString("Stage Time " + std::to_string(stageMin) + ":" + (stageSec < 10 ? "0" : "") + std::to_string(stageSec));
        missionTime.setPosition(WIDTH - 304.0f, 181.0f);
        window.draw(missionTime);

        // Combo meter
        if (player.comboCount > 0)
        {
            sf::Text comboText;
            comboText.setFont(font);
            comboText.setCharacterSize(18);
            comboText.setFillColor(player.isFever() ? sf::Color(255, 120, 200) : sf::Color(0, 255, 200));
            std::stringstream cs;
            cs << "COMBO x" << std::fixed << std::setprecision(1) << player.comboMultiplier;
            comboText.setString(cs.str());
            comboText.setPosition(20, 40);
            window.draw(comboText);

            float r = std::max(0.0f, std::min(1.0f, player.comboTimer / Player::COMBO_WINDOW));
            sf::RectangleShape comboBg(sf::Vector2f(220, 8));
            comboBg.setPosition(140, 44);
            comboBg.setFillColor(sf::Color(40, 40, 60));
            window.draw(comboBg);

            sf::RectangleShape comboFill(sf::Vector2f(220 * r, 8));
            comboFill.setPosition(140, 44);
            comboFill.setFillColor(player.isFever() ? sf::Color(255, 120, 200) : sf::Color(0, 255, 200));
            window.draw(comboFill);

            float pulse = 0.92f + 0.10f * std::sin(player.animTimer * 10.5f);
            sf::Text rankText;
            rankText.setFont(font);
            rankText.setCharacterSize(30);
            rankText.setStyle(sf::Text::Bold);
            rankText.setString(player.comboRank());
            rankText.setFillColor(player.comboRankColor());
            rankText.setScale(pulse, pulse);
            sf::FloatRect rb = rankText.getLocalBounds();
            rankText.setOrigin(rb.left + rb.width * 0.5f, rb.top + rb.height * 0.5f);
            rankText.setPosition(WIDTH * 0.5f, 40.0f);
            window.draw(rankText);
        }

        // Soul meter
        float soulRatio = std::max(0.0f, std::min(1.0f, player.soul / SOUL_MAX));
        sf::RectangleShape soulBarBg(sf::Vector2f(140, 10));
        soulBarBg.setPosition(WIDTH - 290, 12);
        soulBarBg.setFillColor(sf::Color(26, 33, 47, 220));
        soulBarBg.setOutlineThickness(1);
        soulBarBg.setOutlineColor(sf::Color(120, 140, 170));
        window.draw(soulBarBg);

        sf::RectangleShape soulBar(sf::Vector2f(140 * soulRatio, 10));
        soulBar.setPosition(WIDTH - 290, 12);
        soulBar.setFillColor(sf::Color(160, 210, 255));
        window.draw(soulBar);

        sf::Text soulText;
        soulText.setFont(font);
        soulText.setCharacterSize(13);
        soulText.setFillColor(sf::Color(195, 220, 255));
        soulText.setString("SOUL");
        soulText.setPosition(WIDTH - 335, 8);
        window.draw(soulText);

        if (player.isFocusing())
        {
            float focusRatio = std::max(0.0f, std::min(1.0f, player.focusTimer / FOCUS_TIME));
            sf::RectangleShape focusBg(sf::Vector2f(120, 6));
            focusBg.setPosition(WIDTH - 270, 28);
            focusBg.setFillColor(sf::Color(25, 35, 48));
            window.draw(focusBg);

            sf::RectangleShape focusFill(sf::Vector2f(120 * focusRatio, 6));
            focusFill.setPosition(WIDTH - 270, 28);
            focusFill.setFillColor(sf::Color(185, 230, 255));
            window.draw(focusFill);
        }

        // Lives as heart icons
        if (player.lives > 0)
        {
            float heartSize = 12.0f;
            float heartSpacing = 26.0f;
            float heartRightEdge = WIDTH - 320.0f;
            float heartsStartX = heartRightEdge - (player.lives - 1) * heartSpacing;
            float heartY = 18.0f;
            for (int i = 0; i < player.lives; i++)
            {
                drawHeartIcon(window,
                              sf::Vector2f(heartsStartX + i * heartSpacing, heartY),
                              heartSize,
                              sf::Color(255, 80, 120));
            }
        }

        // Gun timer indicator
        if (player.hasGun && player.gunTimer > 0)
        {
            float ratio = std::max(0.0f, std::min(1.0f, player.gunTimer / GUN_DURATION));
            sf::RectangleShape gunBarBg(sf::Vector2f(100, 8));
            gunBarBg.setPosition(WIDTH - 120, 42);
            gunBarBg.setFillColor(sf::Color(50, 50, 50));
            window.draw(gunBarBg);

            sf::RectangleShape gunBar(sf::Vector2f(100 * ratio, 8));
            gunBar.setPosition(WIDTH - 120, 42);
            gunBar.setFillColor(sf::Color(255, 140, 60));
            window.draw(gunBar);

            sf::Text gunText;
            gunText.setFont(font);
            gunText.setCharacterSize(14);
            gunText.setFillColor(sf::Color(255, 200, 150));
            gunText.setString("GUN");
            gunText.setPosition(WIDTH - 170, 36);
            window.draw(gunText);

            sf::Text gunNameText;
            gunNameText.setFont(font);
            gunNameText.setCharacterSize(12);
            gunNameText.setFillColor(activeWeapon().tapColor);
            gunNameText.setString(activeWeapon().name);
            gunNameText.setPosition(WIDTH - 245, 50);
            window.draw(gunNameText);

            sf::Text gunRoleText;
            gunRoleText.setFont(font);
            gunRoleText.setCharacterSize(11);
            gunRoleText.setFillColor(sf::Color(175, 210, 235));
            gunRoleText.setString(activeWeapon().role);
            gunRoleText.setPosition(WIDTH - 245, 64);
            window.draw(gunRoleText);
        }

        // Charge indicator
        if (player.hasGun && player.gunCharging)
        {
            float c = std::max(0.0f, std::min(1.0f, player.gunCharge / GUN_CHARGE_MAX));
            sf::RectangleShape chargeBg(sf::Vector2f(80, 6));
            chargeBg.setPosition(WIDTH - 110, 54);
            chargeBg.setFillColor(sf::Color(40, 40, 60));
            window.draw(chargeBg);

            sf::RectangleShape chargeBar(sf::Vector2f(80 * c, 6));
            chargeBar.setPosition(WIDTH - 110, 54);
            chargeBar.setFillColor(sf::Color(120, 220, 255));
            window.draw(chargeBar);
        }

        // Dash cooldown indicator
        if (player.dashCooldown > 0)
        {
            sf::RectangleShape dashBar(sf::Vector2f(100, 10));
            dashBar.setPosition(WIDTH - 120, 25);
            dashBar.setFillColor(sf::Color(50, 50, 50));
            window.draw(dashBar);

            sf::RectangleShape dashProgress(sf::Vector2f(100 * (1 - player.dashCooldown), 10));
            dashProgress.setPosition(WIDTH - 120, 25);
            dashProgress.setFillColor(sf::Color::Cyan);
            window.draw(dashProgress);
        }

        // Controls hint
        sf::Text controls;
        controls.setFont(font);
        controls.setCharacterSize(15);
        controls.setFillColor(sf::Color(150, 150, 150));
        controls.setString("WASD/Arrows Move | Space Jump | Shift Dash | Z Slash | Hold C Focus | X Shoot | L2: 1-5 Swap, Tab Armory");
        sf::FloatRect cb = controls.getLocalBounds();
        controls.setPosition(WIDTH * 0.5f - cb.width * 0.5f, HEIGHT - 30);
        window.draw(controls);

        if (levelIntroTimer > 0.0f && !weaponSelectActive && !gameOver && !levelComplete)
        {
            float fadeIn = std::min(1.0f, levelIntroTimer / 0.55f);
            float alphaScale = std::min(1.0f, levelIntroTimer / 3.2f);
            sf::RectangleShape introPanel(sf::Vector2f(760, 86));
            introPanel.setOrigin(introPanel.getSize() * 0.5f);
            introPanel.setPosition(WIDTH * 0.5f, 102.0f);
            introPanel.setFillColor(sf::Color(10, 20, 38, static_cast<sf::Uint8>(115 + alphaScale * 95.0f)));
            introPanel.setOutlineThickness(2.0f);
            introPanel.setOutlineColor(sf::Color(120, 190, 255, static_cast<sf::Uint8>(130 + alphaScale * 110.0f)));
            window.draw(introPanel);

            sf::Text introHead;
            introHead.setFont(font);
            introHead.setCharacterSize(27);
            introHead.setStyle(sf::Text::Bold);
            introHead.setFillColor(sf::Color(220, 236, 255, static_cast<sf::Uint8>(160 + alphaScale * 90.0f)));
            introHead.setString(levelIntroHeader.substr(0, 52));
            sf::FloatRect ihb = introHead.getLocalBounds();
            introHead.setOrigin(ihb.left + ihb.width * 0.5f, ihb.top + ihb.height * 0.5f);
            introHead.setPosition(WIDTH * 0.5f, 84.0f - (1.0f - fadeIn) * 8.0f);
            window.draw(introHead);

            sf::Text introObj;
            introObj.setFont(font);
            introObj.setCharacterSize(18);
            introObj.setFillColor(sf::Color(170, 204, 235, static_cast<sf::Uint8>(145 + alphaScale * 90.0f)));
            introObj.setString(levelIntroObjective.substr(0, 74));
            sf::FloatRect iob = introObj.getLocalBounds();
            introObj.setOrigin(iob.left + iob.width * 0.5f, iob.top + iob.height * 0.5f);
            introObj.setPosition(WIDTH * 0.5f, 120.0f);
            window.draw(introObj);
        }

        if (weaponBannerTimer > 0.0f && !weaponSelectActive)
        {
            float alphaRatio = std::max(0.0f, std::min(1.0f, weaponBannerTimer / 0.8f));
            sf::RectangleShape banner(sf::Vector2f(430, 44));
            banner.setOrigin(banner.getSize() * 0.5f);
            banner.setPosition(WIDTH * 0.5f, 95.0f);
            banner.setFillColor(sf::Color(10, 20, 36, static_cast<sf::Uint8>(120 + alphaRatio * 90.0f)));
            banner.setOutlineThickness(2.0f);
            banner.setOutlineColor(sf::Color(120, 215, 255, static_cast<sf::Uint8>(140 + alphaRatio * 90.0f)));
            window.draw(banner);

            sf::Text bannerText;
            bannerText.setFont(font);
            bannerText.setCharacterSize(22);
            bannerText.setStyle(sf::Text::Bold);
            bannerText.setFillColor(sf::Color(220, 238, 255, static_cast<sf::Uint8>(180 + alphaRatio * 70.0f)));
            bannerText.setString(weaponBannerText);
            sf::FloatRect bb = bannerText.getLocalBounds();
            bannerText.setOrigin(bb.left + bb.width * 0.5f, bb.top + bb.height * 0.5f);
            bannerText.setPosition(WIDTH * 0.5f, 94.0f);
            window.draw(bannerText);
        }

        if (weaponSelectActive && !gameOver && !levelComplete)
        {
            const WeaponLoadout &preview = level2Weapons[weaponSelectIndex];
            int recommendedIndex = recommendedWeaponIndex();
            Enemy *armoryBoss = currentLevel->activeBoss();
            std::string armoryTitle = "BOSS ARMORY - LEVEL " + std::to_string(currentLevel->levelNumber);
            std::string armorySubtitle = "Choose one weapon loadout before the giant threat.";
            if (armoryBoss && armoryBoss->type == 4)
                armorySubtitle = "Choose one weapon loadout before the inferno behemoth.";
            else if (armoryBoss && armoryBoss->type == 3)
                armorySubtitle = "Choose one weapon loadout before the titan boss.";
            float powerScore = clamp01((preview.tapDamage * 0.35f + preview.chargeBaseDamage * 0.4f + preview.chargeBonusDamage * 0.25f) / 3.8f);
            float speedScore = clamp01((1.0f / std::max(0.05f, preview.cooldown)) / 14.0f + preview.speedMul * 0.35f);
            float rangeScore = clamp01(preview.speedMul * 0.55f + preview.lifetimeMul * 0.45f);
            float controlScore = clamp01((1.45f - preview.tapSpreadVel / 80.0f) * 0.45f + (1.45f - preview.chargedSpreadVel / 90.0f) * 0.35f + (1.2f - preview.sizeMul) * 0.2f);
            float pierceScore = clamp01((preview.basePierce + preview.chargePierceBonus * 0.8f) / 3.8f);
            float overallScore = (powerScore + speedScore + rangeScore + controlScore + pierceScore) / 5.0f;
            std::string grade = "B";
            if (overallScore >= 0.88f)
                grade = "S";
            else if (overallScore >= 0.76f)
                grade = "A";
            else if (overallScore >= 0.62f)
                grade = "B+";
            else if (overallScore >= 0.48f)
                grade = "B";
            else
                grade = "C";

            sf::RectangleShape overlay(sf::Vector2f(WIDTH, HEIGHT));
            overlay.setFillColor(sf::Color(3, 7, 14, 230));
            window.draw(overlay);

            sf::RectangleShape panel(sf::Vector2f(1040, 560));
            panel.setOrigin(panel.getSize() * 0.5f);
            panel.setPosition(WIDTH * 0.5f, HEIGHT * 0.52f);
            panel.setFillColor(sf::Color(10, 18, 34, 242));
            panel.setOutlineThickness(3.0f);
            panel.setOutlineColor(sf::Color(105, 175, 255, 240));
            window.draw(panel);

            sf::Text title;
            title.setFont(font);
            title.setCharacterSize(38);
            title.setStyle(sf::Text::Bold);
            title.setFillColor(sf::Color(212, 232, 255));
            title.setString(armoryTitle);
            title.setPosition(WIDTH * 0.5f - 260.0f, HEIGHT * 0.5f - 248.0f);
            window.draw(title);

            sf::Text subtitle;
            subtitle.setFont(font);
            subtitle.setCharacterSize(18);
            subtitle.setFillColor(sf::Color(170, 200, 232));
            subtitle.setString(armorySubtitle);
            subtitle.setPosition(WIDTH * 0.5f - 250.0f, HEIGHT * 0.5f - 206.0f);
            window.draw(subtitle);

            float listX = WIDTH * 0.5f - 470.0f;
            float listY = HEIGHT * 0.5f - 160.0f;
            for (int i = 0; i < static_cast<int>(level2Weapons.size()); i++)
            {
                const WeaponLoadout &w = level2Weapons[i];
                sf::Text line;
                line.setFont(font);
                line.setCharacterSize(22);
                line.setStyle(i == weaponSelectIndex ? sf::Text::Bold : sf::Text::Regular);
                line.setFillColor(i == weaponSelectIndex ? w.chargeColor : sf::Color(205, 214, 230));
                std::string recTag = (i == recommendedIndex) ? "  [RECOMMENDED]" : "";
                line.setString(std::to_string(i + 1) + ". " + w.name + "  -  " + w.role + recTag);
                line.setPosition(listX, listY + i * 46.0f);
                window.draw(line);
            }

            sf::RectangleShape statsPanel(sf::Vector2f(470, 330));
            statsPanel.setPosition(WIDTH * 0.5f + 10.0f, HEIGHT * 0.5f - 165.0f);
            statsPanel.setFillColor(sf::Color(12, 26, 44, 230));
            statsPanel.setOutlineThickness(2.0f);
            statsPanel.setOutlineColor(sf::Color(128, 200, 255, 210));
            window.draw(statsPanel);

            std::vector<std::string> statLines;
            statLines.push_back("Selected: " + preview.name);
            statLines.push_back("Role: " + preview.role);
            statLines.push_back("Perk: " + preview.perk);
            statLines.push_back("Tap Fire: " + std::to_string(preview.tapPellets) + " pellets  |  Damage " + std::to_string(preview.tapDamage));
            statLines.push_back("Tap Spread: " + std::to_string(static_cast<int>(preview.tapSpreadVel)));
            statLines.push_back("Charge Fire: " + std::to_string(preview.chargedPellets) + " shots  |  Base " + std::to_string(preview.chargeBaseDamage));
            statLines.push_back("Charge Bonus: +" + std::to_string(preview.chargeBonusDamage) + " damage");
            statLines.push_back("Pierce: " + std::to_string(preview.basePierce) + "  |  Extra on charge: +" + std::to_string(preview.chargePierceBonus));
            statLines.push_back("Speed x" + std::to_string(preview.speedMul).substr(0, 4) +
                                "  |  Lifetime x" + std::to_string(preview.lifetimeMul).substr(0, 4));
            statLines.push_back("Size x" + std::to_string(preview.sizeMul).substr(0, 4) +
                                "  |  Cooldown " + std::to_string(preview.cooldown).substr(0, 4) + "s");

            for (int i = 0; i < static_cast<int>(statLines.size()); i++)
            {
                sf::Text stat;
                stat.setFont(font);
                stat.setCharacterSize(i == 0 ? 23 : (i == 2 ? 16 : 17));
                stat.setStyle(i == 0 ? sf::Text::Bold : sf::Text::Regular);
                stat.setFillColor(i == 0 ? preview.tapColor : sf::Color(210, 225, 240));
                stat.setString(statLines[i].substr(0, 62));
                stat.setPosition(WIDTH * 0.5f + 24.0f, HEIGHT * 0.5f - 150.0f + i * 29.0f);
                window.draw(stat);
            }

            struct MetricLine
            {
                std::string name;
                float value;
            };
            std::vector<MetricLine> metrics = {
                {"POWER", powerScore},
                {"SPEED", speedScore},
                {"RANGE", rangeScore},
                {"CONTROL", controlScore},
                {"PIERCE", pierceScore}};

            float metricBaseY = HEIGHT * 0.5f + 140.0f;
            for (int i = 0; i < static_cast<int>(metrics.size()); i++)
            {
                sf::Text label;
                label.setFont(font);
                label.setCharacterSize(14);
                label.setFillColor(sf::Color(175, 205, 230));
                label.setString(metrics[i].name);
                label.setPosition(WIDTH * 0.5f + 28.0f, metricBaseY + i * 28.0f);
                window.draw(label);

                sf::RectangleShape barBg(sf::Vector2f(214.0f, 10.0f));
                barBg.setPosition(WIDTH * 0.5f + 136.0f, metricBaseY + 4.0f + i * 28.0f);
                barBg.setFillColor(sf::Color(32, 45, 68, 230));
                window.draw(barBg);

                sf::Color metricColor = sf::Color(120, 205, 255);
                if (metrics[i].value >= 0.82f)
                    metricColor = sf::Color(130, 245, 175);
                else if (metrics[i].value < 0.5f)
                    metricColor = sf::Color(255, 180, 120);
                sf::RectangleShape barFill(sf::Vector2f(214.0f * metrics[i].value, 10.0f));
                barFill.setPosition(WIDTH * 0.5f + 136.0f, metricBaseY + 4.0f + i * 28.0f);
                barFill.setFillColor(metricColor);
                window.draw(barFill);
            }

            sf::Text gradeText;
            gradeText.setFont(font);
            gradeText.setCharacterSize(34);
            gradeText.setStyle(sf::Text::Bold);
            gradeText.setFillColor(preview.chargeColor);
            gradeText.setString("GRADE " + grade);
            gradeText.setPosition(WIDTH * 0.5f + 360.0f, HEIGHT * 0.5f + 140.0f);
            window.draw(gradeText);

            sf::Text hint;
            hint.setFont(font);
            hint.setCharacterSize(18);
            hint.setFillColor(sf::Color(190, 210, 230));
            hint.setString("Use Up/Down or 1-5 to equip instantly. Enter/Esc confirms current choice.");
            hint.setPosition(WIDTH * 0.5f - 285.0f, HEIGHT * 0.5f + 242.0f);
            window.draw(hint);
        }

        if (backToMenuPrompt && !gameOver && !levelComplete)
        {
            sf::RectangleShape pauseOverlay(sf::Vector2f(WIDTH, HEIGHT));
            pauseOverlay.setFillColor(sf::Color(0, 0, 0, 170));
            window.draw(pauseOverlay);

            sf::RectangleShape promptPanel(sf::Vector2f(720, 250));
            promptPanel.setOrigin(promptPanel.getSize() * 0.5f);
            promptPanel.setPosition(WIDTH * 0.5f, HEIGHT * 0.5f);
            promptPanel.setFillColor(sf::Color(10, 22, 40, 245));
            promptPanel.setOutlineThickness(3.0f);
            promptPanel.setOutlineColor(sf::Color(110, 182, 255, 235));
            window.draw(promptPanel);

            sf::Text promptTitle;
            promptTitle.setFont(font);
            promptTitle.setCharacterSize(35);
            promptTitle.setStyle(sf::Text::Bold);
            promptTitle.setFillColor(sf::Color(215, 234, 255));
            promptTitle.setString("Do you want back to menu?");
            sf::FloatRect pt = promptTitle.getLocalBounds();
            promptTitle.setOrigin(pt.left + pt.width * 0.5f, pt.top + pt.height * 0.5f);
            promptTitle.setPosition(WIDTH * 0.5f, HEIGHT * 0.5f - 62.0f);
            window.draw(promptTitle);

            auto drawPromptOption = [&](const std::string &label, int optionIndex, float x)
            {
                bool selected = (backToMenuChoice == optionIndex);
                sf::RectangleShape btn(sf::Vector2f(170, 54));
                btn.setOrigin(btn.getSize() * 0.5f);
                btn.setPosition(x, HEIGHT * 0.5f + 16.0f);
                btn.setFillColor(selected ? sf::Color(85, 155, 245, 240) : sf::Color(34, 54, 82, 225));
                btn.setOutlineThickness(2.0f);
                btn.setOutlineColor(selected ? sf::Color(185, 230, 255) : sf::Color(100, 128, 165));
                window.draw(btn);

                sf::Text txt;
                txt.setFont(font);
                txt.setCharacterSize(30);
                txt.setStyle(sf::Text::Bold);
                txt.setFillColor(selected ? sf::Color(245, 252, 255) : sf::Color(192, 210, 232));
                txt.setString(label);
                sf::FloatRect tb = txt.getLocalBounds();
                txt.setOrigin(tb.left + tb.width * 0.5f, tb.top + tb.height * 0.5f);
                txt.setPosition(x, HEIGHT * 0.5f + 14.0f);
                window.draw(txt);
            };
            drawPromptOption("YES", 0, WIDTH * 0.5f - 220.0f);
            drawPromptOption("NO", 1, WIDTH * 0.5f);
            drawPromptOption("SAVE", 2, WIDTH * 0.5f + 220.0f);

            sf::Text promptHint;
            promptHint.setFont(font);
            promptHint.setCharacterSize(16);
            promptHint.setFillColor(sf::Color(170, 198, 226));
            promptHint.setString("Left/Right or Up/Down to switch  |  Enter to confirm (YES / NO / SAVE)");
            sf::FloatRect ph = promptHint.getLocalBounds();
            promptHint.setOrigin(ph.left + ph.width * 0.5f, ph.top + ph.height * 0.5f);
            promptHint.setPosition(WIDTH * 0.5f, HEIGHT * 0.5f + 88.0f);
            window.draw(promptHint);
        }

        if (gameOver)
        {
            sf::RectangleShape overlay(sf::Vector2f(WIDTH, HEIGHT));
            overlay.setFillColor(sf::Color(0, 0, 0, 200));
            window.draw(overlay);

            sf::Text gameOverText;
            gameOverText.setFont(font);
            gameOverText.setCharacterSize(80);
            gameOverText.setFillColor(sf::Color(255, 50, 100));
            gameOverText.setStyle(sf::Text::Bold);
            gameOverText.setString("GAME OVER");
            gameOverText.setPosition(WIDTH / 2 - 250, HEIGHT / 2 - 150);
            window.draw(gameOverText);

            sf::Text finalScore;
            finalScore.setFont(font);
            finalScore.setCharacterSize(36);
            finalScore.setFillColor(sf::Color::White);
            std::stringstream fs;
            fs << "Final Score: " << player.score << "\nHigh Score: " << highScore;
            finalScore.setString(fs.str());
            finalScore.setPosition(WIDTH / 2 - 150, HEIGHT / 2 - 20);
            window.draw(finalScore);

            sf::Text restart;
            restart.setFont(font);
            restart.setCharacterSize(28);
            restart.setFillColor(sf::Color::Yellow);
            restart.setString("Press R to Restart");
            restart.setPosition(WIDTH / 2 - 150, HEIGHT / 2 + 100);
            window.draw(restart);
        }

        if (levelComplete)
        {
            sf::RectangleShape overlay(sf::Vector2f(WIDTH, HEIGHT));
            overlay.setFillColor(sf::Color(0, 0, 0, 190));
            window.draw(overlay);

            sf::RectangleShape panel(sf::Vector2f(980, 520));
            panel.setOrigin(panel.getSize() * 0.5f);
            panel.setPosition(WIDTH * 0.5f, HEIGHT * 0.52f);
            panel.setFillColor(sf::Color(10, 18, 34, 242));
            panel.setOutlineThickness(3.0f);
            panel.setOutlineColor(sf::Color(92, 170, 255, 230));
            window.draw(panel);

            sf::Text completeHeader;
            completeHeader.setFont(font);
            completeHeader.setCharacterSize(50);
            completeHeader.setStyle(sf::Text::Bold);
            completeHeader.setFillColor(sf::Color(130, 245, 188));
            completeHeader.setString("LEVEL COMPLETE");
            completeHeader.setPosition(WIDTH * 0.5f - 430.0f, HEIGHT * 0.5f - 226.0f);
            window.draw(completeHeader);

            sf::Text chapterLine;
            chapterLine.setFont(font);
            chapterLine.setCharacterSize(20);
            chapterLine.setFillColor(sf::Color(180, 212, 240));
            chapterLine.setString(currentLevel->chapterName());
            chapterLine.setPosition(WIDTH * 0.5f - 426.0f, HEIGHT * 0.5f - 182.0f);
            window.draw(chapterLine);

            sf::Color gradeColor(180, 225, 255);
            if (levelReportGrade == "S")
                gradeColor = sf::Color(255, 230, 140);
            else if (levelReportGrade == "A")
                gradeColor = sf::Color(140, 255, 195);
            else if (levelReportGrade == "B")
                gradeColor = sf::Color(160, 215, 255);
            else if (levelReportGrade == "C")
                gradeColor = sf::Color(255, 190, 130);
            else
                gradeColor = sf::Color(255, 130, 140);

            sf::Text gradeText;
            gradeText.setFont(font);
            gradeText.setCharacterSize(86);
            gradeText.setStyle(sf::Text::Bold);
            gradeText.setFillColor(gradeColor);
            gradeText.setString(levelReportGrade);
            gradeText.setPosition(WIDTH * 0.5f + 392.0f, HEIGHT * 0.5f - 242.0f);
            window.draw(gradeText);

            sf::Text reportSummary;
            reportSummary.setFont(font);
            reportSummary.setCharacterSize(18);
            reportSummary.setFillColor(sf::Color(188, 212, 235));
            reportSummary.setString(levelReportSummary.substr(0, 70));
            reportSummary.setPosition(WIDTH * 0.5f - 426.0f, HEIGHT * 0.5f - 145.0f);
            window.draw(reportSummary);

            int reportTimeSec = static_cast<int>(std::max(0.0f, levelReportTime));
            int reportMin = reportTimeSec / 60;
            int reportSec = reportTimeSec % 60;

            sf::Text reportStatsA;
            reportStatsA.setFont(font);
            reportStatsA.setCharacterSize(18);
            reportStatsA.setFillColor(sf::Color(205, 224, 240));
            reportStatsA.setString("Stage Time: " + std::to_string(reportMin) + ":" + (reportSec < 10 ? "0" : "") + std::to_string(reportSec) +
                                   "   |   Score + " + std::to_string(levelReportScoreGain) +
                                   "   |   Coins + " + std::to_string(levelReportCoinsGain));
            reportStatsA.setPosition(WIDTH * 0.5f - 426.0f, HEIGHT * 0.5f - 108.0f);
            window.draw(reportStatsA);

            std::stringstream comboValue;
            comboValue << std::fixed << std::setprecision(1) << levelReportBestCombo;

            sf::Text reportStatsB;
            reportStatsB.setFont(font);
            reportStatsB.setCharacterSize(18);
            reportStatsB.setFillColor(sf::Color(205, 224, 240));
            reportStatsB.setString("Deaths: " + std::to_string(levelReportDeaths) +
                                   "   |   Hits Taken: " + std::to_string(levelReportHits) +
                                   "   |   Best Combo x" + comboValue.str() +
                                   (levelSecondWindUsed ? "   |   Assist: Second Wind" : ""));
            reportStatsB.setPosition(WIDTH * 0.5f - 426.0f, HEIGHT * 0.5f - 79.0f);
            window.draw(reportStatsB);

            struct ReportMetric
            {
                std::string label;
                float value;
            };
            std::vector<ReportMetric> reportMetrics = {
                {"PACE", levelReportPace},
                {"SURVIVAL", levelReportSurvival},
                {"PRECISION", levelReportPrecision},
                {"STYLE", levelReportStyle},
                {"OVERALL", levelReportTotal}};

            float metricsBaseY = HEIGHT * 0.5f - 28.0f;
            for (int i = 0; i < static_cast<int>(reportMetrics.size()); i++)
            {
                sf::Text metricLabel;
                metricLabel.setFont(font);
                metricLabel.setCharacterSize(17);
                metricLabel.setStyle(sf::Text::Bold);
                metricLabel.setFillColor(sf::Color(178, 210, 238));
                metricLabel.setString(reportMetrics[i].label);
                metricLabel.setPosition(WIDTH * 0.5f - 424.0f, metricsBaseY + i * 64.0f);
                window.draw(metricLabel);

                sf::RectangleShape metricBarBg(sf::Vector2f(710, 16));
                metricBarBg.setPosition(WIDTH * 0.5f - 280.0f, metricsBaseY + 4.0f + i * 64.0f);
                metricBarBg.setFillColor(sf::Color(28, 42, 64, 235));
                window.draw(metricBarBg);

                sf::Color metricColor(120, 205, 255);
                if (reportMetrics[i].value >= 0.86f)
                    metricColor = sf::Color(135, 245, 182);
                else if (reportMetrics[i].value < 0.5f)
                    metricColor = sf::Color(255, 165, 120);
                sf::RectangleShape metricBar(sf::Vector2f(710.0f * clamp01(reportMetrics[i].value), 16));
                metricBar.setPosition(WIDTH * 0.5f - 280.0f, metricsBaseY + 4.0f + i * 64.0f);
                metricBar.setFillColor(metricColor);
                window.draw(metricBar);

                int pct = static_cast<int>(std::round(clamp01(reportMetrics[i].value) * 100.0f));
                sf::Text metricPercent;
                metricPercent.setFont(font);
                metricPercent.setCharacterSize(16);
                metricPercent.setStyle(sf::Text::Bold);
                metricPercent.setFillColor(sf::Color(222, 236, 248));
                metricPercent.setString(std::to_string(pct) + "%");
                metricPercent.setPosition(WIDTH * 0.5f + 442.0f, metricsBaseY + i * 64.0f);
                window.draw(metricPercent);
            }

            sf::Text nextLevel;
            nextLevel.setFont(font);
            nextLevel.setCharacterSize(28);
            nextLevel.setFillColor(sf::Color(220, 238, 255));
            nextLevel.setString("Press R for Next Level");
            nextLevel.setPosition(WIDTH * 0.5f - 165.0f, HEIGHT * 0.5f + 226.0f);
            window.draw(nextLevel);
        }

        window.display();
    }

    return 0;
}
