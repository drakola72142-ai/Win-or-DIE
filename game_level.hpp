#pragma once
#include "game_effects.hpp"
#include "game_platform.hpp"
#include "game_collectibles.hpp"
#include "game_weapons.hpp"
#include "game_enemies.hpp"
#include "game_traps.hpp"

class Level
{
public:
    std::vector<std::unique_ptr<Platform>> platforms;
    std::vector<std::unique_ptr<Coin>> coins;
    std::vector<std::unique_ptr<Heart>> hearts;
    std::vector<std::unique_ptr<GunPickup>> guns;
    std::vector<std::unique_ptr<Enemy>> enemies;
    std::vector<EnemyFireball> enemyFireballs;
    std::vector<SpikeTrap> spikeTraps;
    std::vector<sf::FloatRect> benches;
    sf::Color backgroundColor;
    int levelNumber;
    float difficultyMultiplier;
    float baseDifficultyMultiplier;
    float enemyHpMultiplier;
    float enemySpeedMultiplier;
    float trapIntensityMultiplier;
    float baseTrapIntensityMultiplier;
    float directorBias;
    float appliedDirectorBias;
    float levelElapsedTime;
    float progressiveRamp;
    std::string difficultyTitle;
    std::string chapterTitle;
    std::string objectiveText;

    Level(int num) : levelNumber(num), difficultyMultiplier(1.0f), baseDifficultyMultiplier(1.0f),
                     enemyHpMultiplier(1.0f), enemySpeedMultiplier(1.0f),
                     trapIntensityMultiplier(1.0f), baseTrapIntensityMultiplier(1.0f),
                     directorBias(1.0f), appliedDirectorBias(1.0f), levelElapsedTime(0.0f), progressiveRamp(0.0f),
                     difficultyTitle("INIT"), chapterTitle("INIT"), objectiveText("INIT")
    {
        backgroundColor = sf::Color(20, 20, 50);
        generateLevel();
    }

    void generateLevel()
    {
        platforms.clear();
        coins.clear();
        hearts.clear();
        guns.clear();
        enemies.clear();
        enemyFireballs.clear();
        spikeTraps.clear();
        benches.clear();

        auto addSpikeTrap = [&](float x, float y, float w, float rise, int spikes, float trigger)
        {
            spikeTraps.emplace_back(x, y, w, rise, spikes, trigger);
        };
        auto addEvenInfernoBoss = [&]()
        {
            if (levelNumber % 2 != 0)
                return;

            Platform *bossPlatform = nullptr;
            for (auto &p : platforms)
            {
                if (p->type == 1 || p->bounds.width < 110.0f)
                    continue;
                if (p->bounds.top > HEIGHT - 60.0f)
                    continue;

                if (!bossPlatform)
                {
                    bossPlatform = p.get();
                    continue;
                }

                bool betterHeight = p->bounds.top < bossPlatform->bounds.top;
                bool betterRight = std::abs(p->bounds.top - bossPlatform->bounds.top) < 20.0f &&
                                   p->bounds.left > bossPlatform->bounds.left;
                if (betterHeight || betterRight)
                    bossPlatform = p.get();
            }

            float x = WIDTH * 0.72f;
            float y = HEIGHT - 108.0f;
            if (bossPlatform)
            {
                x = bossPlatform->bounds.left + bossPlatform->bounds.width * 0.5f;
                y = bossPlatform->bounds.top - 68.0f;
            }
            x = std::max(84.0f, std::min(static_cast<float>(WIDTH) - 84.0f, x));
            y = std::max(88.0f, y);

            enemies.push_back(std::make_unique<Enemy>(x, y, 4));
        };

        // Base bench near the starting area
        benches.push_back(sf::FloatRect(140.0f, HEIGHT - 80.0f, 70.0f, 30.0f));

        // Ground
        platforms.push_back(std::make_unique<Platform>(0, HEIGHT - 40, WIDTH, 40, 0));

        // Level 1 - Tutorial style
        if (levelNumber == 1)
        {
            backgroundColor = sf::Color(20, 20, 50);
            // Starting platforms
            platforms.push_back(std::make_unique<Platform>(300, 600, 150, 20, 0));
            platforms.push_back(std::make_unique<Platform>(550, 500, 150, 20, 0));
            platforms.push_back(std::make_unique<Platform>(800, 400, 150, 20, 3)); // Bounce

            // Moving platform
            platforms.push_back(std::make_unique<Platform>(400, 300, 120, 20, 1));

            // Upper platforms
            platforms.push_back(std::make_unique<Platform>(150, 200, 100, 20, 0));
            platforms.push_back(std::make_unique<Platform>(900, 250, 120, 20, 4));
            platforms.push_back(std::make_unique<Platform>(1100, 150, 150, 20, 0));

            // Coins
            for (int i = 0; i < 10; i++)
            {
                coins.push_back(std::make_unique<Coin>(300 + i * 100, 550, 1));
            }
            coins.push_back(std::make_unique<Coin>(875, 320, 2));
            coins.push_back(std::make_unique<Coin>(1175, 100, 2));

            // Hearts
            hearts.push_back(std::make_unique<Heart>(575, 460));
            hearts.push_back(std::make_unique<Heart>(1100, 110));
            guns.push_back(std::make_unique<GunPickup>(420, 260));

            // Enemies
            enemies.push_back(std::make_unique<Enemy>(600, 450, 0));
            enemies.push_back(std::make_unique<Enemy>(900, 200, 1));

            coins.push_back(std::make_unique<Coin>(950, 215, 1, true));
        }
        // Level 2 - More challenging
        else if (levelNumber == 2)
        {
            backgroundColor = sf::Color(18, 18, 55);
            platforms.push_back(std::make_unique<Platform>(200, 650, 120, 20, 2)); // Breakable
            platforms.push_back(std::make_unique<Platform>(400, 550, 100, 20, 1)); // Moving
            platforms.push_back(std::make_unique<Platform>(650, 450, 120, 20, 4));
            platforms.push_back(std::make_unique<Platform>(900, 350, 100, 20, 3)); // Bounce
            platforms.push_back(std::make_unique<Platform>(500, 200, 150, 20, 1)); // Moving
            platforms.push_back(std::make_unique<Platform>(1100, 500, 120, 20, 0));
            platforms.push_back(std::make_unique<Platform>(1250, 300, 100, 20, 0));

            // More coins
            for (int i = 0; i < 15; i++)
            {
                coins.push_back(std::make_unique<Coin>(250 + i * 70, 600 - (i % 3) * 50, 1));
            }
            coins.push_back(std::make_unique<Coin>(700, 400, 2));
            coins.push_back(std::make_unique<Coin>(950, 280, 2));
            coins.push_back(std::make_unique<Coin>(1300, 250, 2));

            // Hearts
            hearts.push_back(std::make_unique<Heart>(500, 160));
            hearts.push_back(std::make_unique<Heart>(950, 300));
            guns.push_back(std::make_unique<GunPickup>(900, 320));

            // More enemies
            enemies.push_back(std::make_unique<Enemy>(400, 500, 0));
            enemies.push_back(std::make_unique<Enemy>(700, 400, 1));
            enemies.push_back(std::make_unique<Enemy>(1100, 450, 2));
            enemies.push_back(std::make_unique<Enemy>(600, 300, 1));

            coins.push_back(std::make_unique<Coin>(530, 505, 1, true));
            addSpikeTrap(620, HEIGHT - 40, 130, 64, 7, 98);
            addSpikeTrap(905, 350, 100, 56, 6, 82);
        }
        // Level 3 - Neon Climb
        else if (levelNumber == 3)
        {
            backgroundColor = sf::Color(15, 25, 45);
            for (int i = 0; i < 8; i++)
            {
                int x = (i % 2 == 0) ? 200 : 850;
                int y = 620 - i * 70;
                int type = (i % 3 == 0) ? 1 : ((i % 3 == 1) ? 0 : 3);
                platforms.push_back(std::make_unique<Platform>(x, y, 160, 20, type));
                coins.push_back(std::make_unique<Coin>(x + 60, y - 40, 1));
            }

            platforms.push_back(std::make_unique<Platform>(520, 420, 120, 20, 2));
            platforms.push_back(std::make_unique<Platform>(420, 260, 120, 20, 4));
            coins.push_back(std::make_unique<Coin>(560, 380, 2));
            coins.push_back(std::make_unique<Coin>(460, 220, 2));

            hearts.push_back(std::make_unique<Heart>(920, 120));
            hearts.push_back(std::make_unique<Heart>(350, 220));
            guns.push_back(std::make_unique<GunPickup>(520, 330));

            enemies.push_back(std::make_unique<Enemy>(300, 580, 0));
            enemies.push_back(std::make_unique<Enemy>(880, 520, 0));
            enemies.push_back(std::make_unique<Enemy>(900, 250, 1));
            enemies.push_back(std::make_unique<Enemy>(300, 250, 2));

            coins.push_back(std::make_unique<Coin>(420, 220, 1, true));
            addSpikeTrap(520, 420, 120, 62, 7, 90);
        }
        // Level 4 - Kinetic Hall
        else if (levelNumber == 4)
        {
            backgroundColor = sf::Color(20, 15, 40);
            for (int i = 0; i < 5; i++)
            {
                int x = 150 + i * 220;
                int y = 620 - (i % 2) * 90;
                platforms.push_back(std::make_unique<Platform>(x, y, 140, 20, 1));
                coins.push_back(std::make_unique<Coin>(x + 50, y - 40, 1));
            }

            platforms.push_back(std::make_unique<Platform>(350, 360, 120, 20, 3));
            platforms.push_back(std::make_unique<Platform>(850, 300, 120, 20, 3));
            platforms.push_back(std::make_unique<Platform>(600, 460, 120, 20, 2));
            platforms.push_back(std::make_unique<Platform>(1000, 520, 120, 20, 2));

            platforms.push_back(std::make_unique<Platform>(250, 200, 120, 20, 0));
            platforms.push_back(std::make_unique<Platform>(650, 170, 140, 20, 4));
            platforms.push_back(std::make_unique<Platform>(1050, 210, 140, 20, 0));

            coins.push_back(std::make_unique<Coin>(260, 160, 2));
            coins.push_back(std::make_unique<Coin>(700, 130, 2));
            coins.push_back(std::make_unique<Coin>(1100, 170, 2));

            hearts.push_back(std::make_unique<Heart>(650, 120));
            hearts.push_back(std::make_unique<Heart>(1050, 160));
            guns.push_back(std::make_unique<GunPickup>(600, 240));

            enemies.push_back(std::make_unique<Enemy>(350, 520, 0));
            enemies.push_back(std::make_unique<Enemy>(650, 420, 1));
            enemies.push_back(std::make_unique<Enemy>(980, 480, 2));
            enemies.push_back(std::make_unique<Enemy>(400, 260, 0));
            enemies.push_back(std::make_unique<Enemy>(900, 260, 1));

            coins.push_back(std::make_unique<Coin>(720, 145, 1, true));
            addSpikeTrap(350, 360, 120, 58, 7, 88);
        }
        // Level 5 - Breaker Run
        else if (levelNumber == 5)
        {
            backgroundColor = sf::Color(30, 20, 20);
            for (int i = 0; i < 10; i++)
            {
                int x = (i % 2 == 0) ? 220 : 820;
                int y = 640 - i * 55;
                int type = (i % 3 == 0) ? 4 : ((i % 2 == 0) ? 2 : 0);
                platforms.push_back(std::make_unique<Platform>(x, y, 140, 20, type));
                coins.push_back(std::make_unique<Coin>(x + 60, y - 35, 1));
            }

            platforms.push_back(std::make_unique<Platform>(520, 560, 120, 20, 3));
            platforms.push_back(std::make_unique<Platform>(520, 420, 140, 20, 1));
            coins.push_back(std::make_unique<Coin>(560, 380, 2));

            hearts.push_back(std::make_unique<Heart>(900, 150));
            hearts.push_back(std::make_unique<Heart>(300, 260));
            guns.push_back(std::make_unique<GunPickup>(520, 500));

            enemies.push_back(std::make_unique<Enemy>(250, 600, 0));
            enemies.push_back(std::make_unique<Enemy>(820, 520, 2));
            enemies.push_back(std::make_unique<Enemy>(520, 520, 1));
            enemies.push_back(std::make_unique<Enemy>(300, 300, 0));
            enemies.push_back(std::make_unique<Enemy>(900, 260, 1));
            enemies.push_back(std::make_unique<Enemy>(520, 380, 2));

            coins.push_back(std::make_unique<Coin>(520, 350, 1, true));
            addSpikeTrap(520, 560, 120, 64, 7, 90);
        }
        // Level 6 - Sky Bridges
        else if (levelNumber == 6)
        {
            backgroundColor = sf::Color(10, 30, 35);
            platforms.push_back(std::make_unique<Platform>(200, 610, 140, 20, 3));
            platforms.push_back(std::make_unique<Platform>(900, 610, 140, 20, 3));

            platforms.push_back(std::make_unique<Platform>(350, 450, 120, 20, 1));
            platforms.push_back(std::make_unique<Platform>(850, 430, 120, 20, 1));
            platforms.push_back(std::make_unique<Platform>(600, 520, 120, 20, 2));

            platforms.push_back(std::make_unique<Platform>(150, 360, 140, 20, 0));
            platforms.push_back(std::make_unique<Platform>(450, 300, 140, 20, 0));
            platforms.push_back(std::make_unique<Platform>(750, 260, 140, 20, 4));
            platforms.push_back(std::make_unique<Platform>(1050, 220, 140, 20, 0));

            coins.push_back(std::make_unique<Coin>(170, 320, 1));
            coins.push_back(std::make_unique<Coin>(470, 260, 1));
            coins.push_back(std::make_unique<Coin>(770, 220, 2));
            coins.push_back(std::make_unique<Coin>(1070, 180, 2));

            hearts.push_back(std::make_unique<Heart>(1050, 170));
            hearts.push_back(std::make_unique<Heart>(450, 250));
            guns.push_back(std::make_unique<GunPickup>(760, 220));

            enemies.push_back(std::make_unique<Enemy>(220, 580, 0));
            enemies.push_back(std::make_unique<Enemy>(600, 460, 1));
            enemies.push_back(std::make_unique<Enemy>(880, 380, 1));
            enemies.push_back(std::make_unique<Enemy>(750, 220, 1));
            enemies.push_back(std::make_unique<Enemy>(300, 320, 2));

            coins.push_back(std::make_unique<Coin>(760, 180, 1, true));
            addSpikeTrap(600, 520, 120, 60, 7, 88);
        }
        // Level 7 - Factory
        else if (levelNumber == 7)
        {
            backgroundColor = sf::Color(25, 25, 35);
            for (int i = 0; i < 6; i++)
            {
                int x = 120 + i * 190;
                int y = 610 - (i % 2) * 70;
                platforms.push_back(std::make_unique<Platform>(x, y, 130, 20, 1));
                coins.push_back(std::make_unique<Coin>(x + 45, y - 40, 1));
            }

            platforms.push_back(std::make_unique<Platform>(300, 420, 120, 20, 2));
            platforms.push_back(std::make_unique<Platform>(700, 380, 120, 20, 2));
            platforms.push_back(std::make_unique<Platform>(1000, 340, 120, 20, 4));
            platforms.push_back(std::make_unique<Platform>(550, 520, 120, 20, 3));

            platforms.push_back(std::make_unique<Platform>(200, 200, 140, 20, 0));
            platforms.push_back(std::make_unique<Platform>(620, 180, 140, 20, 0));
            platforms.push_back(std::make_unique<Platform>(980, 200, 140, 20, 0));

            coins.push_back(std::make_unique<Coin>(230, 160, 2));
            coins.push_back(std::make_unique<Coin>(650, 140, 2));
            coins.push_back(std::make_unique<Coin>(1010, 160, 2));

            hearts.push_back(std::make_unique<Heart>(620, 130));
            hearts.push_back(std::make_unique<Heart>(980, 150));
            guns.push_back(std::make_unique<GunPickup>(450, 420));

            enemies.push_back(std::make_unique<Enemy>(200, 560, 0));
            enemies.push_back(std::make_unique<Enemy>(480, 520, 1));
            enemies.push_back(std::make_unique<Enemy>(820, 520, 2));
            enemies.push_back(std::make_unique<Enemy>(300, 360, 0));
            enemies.push_back(std::make_unique<Enemy>(700, 320, 1));
            enemies.push_back(std::make_unique<Enemy>(1000, 300, 2));
            enemies.push_back(std::make_unique<Enemy>(620, 140, 1));

            coins.push_back(std::make_unique<Coin>(1000, 165, 1, true));
            addSpikeTrap(550, 520, 120, 62, 7, 86);
        }
        // Level 8 - Final Gauntlet
        else if (levelNumber == 8)
        {
            backgroundColor = sf::Color(15, 10, 20);
            for (int i = 0; i < 7; i++)
            {
                int x = 150 + i * 170;
                int y = 620 - (i % 3) * 80;
                int type = (i == 4) ? 4 : ((i % 3 == 0) ? 1 : ((i % 3 == 1) ? 2 : 0));
                platforms.push_back(std::make_unique<Platform>(x, y, 130, 20, type));
                coins.push_back(std::make_unique<Coin>(x + 45, y - 40, 1));
            }

            platforms.push_back(std::make_unique<Platform>(500, 420, 120, 20, 3));
            platforms.push_back(std::make_unique<Platform>(800, 320, 120, 20, 3));
            platforms.push_back(std::make_unique<Platform>(1000, 160, 140, 20, 0));

            coins.push_back(std::make_unique<Coin>(520, 380, 2));
            coins.push_back(std::make_unique<Coin>(820, 280, 2));
            coins.push_back(std::make_unique<Coin>(1050, 120, 2));

            hearts.push_back(std::make_unique<Heart>(1050, 110));
            hearts.push_back(std::make_unique<Heart>(800, 260));
            hearts.push_back(std::make_unique<Heart>(300, 300));
            guns.push_back(std::make_unique<GunPickup>(520, 420));

            enemies.push_back(std::make_unique<Enemy>(220, 560, 0));
            enemies.push_back(std::make_unique<Enemy>(420, 520, 2));
            enemies.push_back(std::make_unique<Enemy>(620, 520, 1));
            enemies.push_back(std::make_unique<Enemy>(820, 460, 0));
            enemies.push_back(std::make_unique<Enemy>(980, 420, 1));
            enemies.push_back(std::make_unique<Enemy>(600, 340, 2));
            enemies.push_back(std::make_unique<Enemy>(820, 260, 1));
            enemies.push_back(std::make_unique<Enemy>(1050, 120, 2));

            coins.push_back(std::make_unique<Coin>(920, 250, 1, true));
            addSpikeTrap(800, 320, 120, 66, 8, 90);
            addSpikeTrap(1000, 160, 140, 66, 8, 84);
        }
        // Level 9+ - Procedural (endless)
        else
        {
            backgroundColor = sf::Color(18, 18, 40);
            int steps = 10 + rand() % 4;
            for (int i = 0; i < steps; i++)
            {
                int x = 140 + (i % 2) * 600 + rand() % 80;
                int y = 640 - i * 55;
                int type = rand() % 5;
                platforms.push_back(std::make_unique<Platform>(x, y, 140, 20, type));
                coins.push_back(std::make_unique<Coin>(x + 55, y - 35, rand() % 2 + 1));
                if ((rand() % 100) < 28)
                {
                    coins.push_back(std::make_unique<Coin>(x + 15 + rand() % 100, y - 58, 1, true));
                }
                if (i % 4 == 0)
                {
                    coins.push_back(std::make_unique<Coin>(x + 90, y - 70, 2));
                }
                if (i % 3 == 1)
                {
                    addSpikeTrap(x, y, 140, 52.0f + static_cast<float>(rand() % 22), 5 + rand() % 4, 82.0f + rand() % 35);
                }
            }

            hearts.push_back(std::make_unique<Heart>(250 + rand() % 900, 180 + rand() % 350));
            hearts.push_back(std::make_unique<Heart>(250 + rand() % 900, 180 + rand() % 350));
            guns.push_back(std::make_unique<GunPickup>(300 + rand() % 700, 220 + rand() % 300));

            for (int i = 0; i < 6; i++)
            {
                enemies.push_back(std::make_unique<Enemy>(
                    220 + i * 170, 520 - i * 30, rand() % 3));
            }
        }

        if (OPEN_FLOOR_MODE > 0 && platforms.size() > 3)
        {
            int openingFloors = (OPEN_FLOOR_MODE == 2 ? 3 : 2);
            if (TRAP_DENSITY_LEVEL == 0)
                openingFloors = std::max(1, openingFloors - 1);
            else if (TRAP_DENSITY_LEVEL == 2)
                openingFloors += 2;
            if (levelNumber <= 2)
                openingFloors = std::max(1, openingFloors - 1);
            int openingAttempts = 0;
            while (openingFloors > 0 && openingAttempts < 110)
            {
                openingAttempts++;
                int idx = 1 + rand() % static_cast<int>(platforms.size() - 1);
                Platform *p = platforms[idx].get();
                if (p->type == 1 || p->type == 3 || p->type == 5)
                    continue;
                if (p->bounds.width < 92.0f || p->bounds.top > HEIGHT - 62.0f)
                    continue;
                if (p->bounds.left < 270.0f && p->bounds.top > HEIGHT - 170.0f)
                    continue;
                p->setType(5);
                openingFloors--;
            }
        }

        if (DIE_AGAIN_MODE && platforms.size() > 3)
        {
            int extraCursedCoins = std::min(5, 1 + levelNumber / 2);
            if (TRAP_DENSITY_LEVEL == 0)
                extraCursedCoins = std::max(1, extraCursedCoins - 1);
            else if (TRAP_DENSITY_LEVEL == 2)
                extraCursedCoins = std::min(7, extraCursedCoins + 2);
            int cursedAttempts = 0;
            while (extraCursedCoins > 0 && cursedAttempts < 60)
            {
                cursedAttempts++;
                int idx = 1 + rand() % static_cast<int>(platforms.size() - 1);
                Platform *p = platforms[idx].get();
                if (p->bounds.top > HEIGHT - 65.0f || p->bounds.width < 95.0f)
                    continue;
                float cx = p->bounds.left + 18.0f + static_cast<float>(rand() % static_cast<int>(std::max(24.0f, p->bounds.width - 36.0f)));
                float cy = p->bounds.top - 42.0f - static_cast<float>(rand() % 18);
                coins.push_back(std::make_unique<Coin>(cx, cy, 1, true));
                extraCursedCoins--;
            }

            int ambushTraps = std::min(6, 2 + levelNumber / 2);
            if (TRAP_DENSITY_LEVEL == 0)
                ambushTraps = std::max(1, ambushTraps - 1);
            else if (TRAP_DENSITY_LEVEL == 2)
                ambushTraps = std::min(9, ambushTraps + 2);
            int trapAttempts = 0;
            while (ambushTraps > 0 && trapAttempts < 80)
            {
                trapAttempts++;
                int idx = 1 + rand() % static_cast<int>(platforms.size() - 1);
                Platform *p = platforms[idx].get();
                if (p->type == 1 || p->bounds.width < 88.0f || p->bounds.top > HEIGHT - 55.0f)
                    continue;
                if (p->bounds.left < 260.0f && p->bounds.top > HEIGHT - 140.0f)
                    continue;
                float trapWidth = std::min(p->bounds.width, 90.0f + static_cast<float>(rand() % 55));
                float wiggle = std::max(0.0f, p->bounds.width - trapWidth);
                float trapX = p->bounds.left + wiggle * (static_cast<float>(rand() % 100) / 100.0f);
                addSpikeTrap(trapX, p->bounds.top, trapWidth,
                             56.0f + static_cast<float>(rand() % 24), 5 + rand() % 4, 66.0f + rand() % 30);
                ambushTraps--;
            }
        }

        if (HARDCORE_MODE && platforms.size() > 2)
        {
            int bonusTraps = std::min(4, 1 + levelNumber / 3);
            if (TRAP_DENSITY_LEVEL == 0)
                bonusTraps = std::max(1, bonusTraps - 1);
            else if (TRAP_DENSITY_LEVEL == 2)
                bonusTraps = std::min(6, bonusTraps + 1);
            int attempts = 0;
            while (bonusTraps > 0 && attempts < 20)
            {
                attempts++;
                int idx = 1 + rand() % static_cast<int>(platforms.size() - 1);
                Platform *p = platforms[idx].get();
                if (p->type == 1 || p->bounds.width < 90.0f || p->bounds.top > HEIGHT - 55.0f)
                    continue;
                addSpikeTrap(p->bounds.left, p->bounds.top, p->bounds.width,
                             58.0f + static_cast<float>(rand() % 25), 6 + rand() % 4, 76.0f + rand() % 28);
                bonusTraps--;
            }
        }

        addEvenInfernoBoss();
        applyDifficultyCurve();
    }

    void applyDifficultyCurve()
    {
        float lv = static_cast<float>(std::max(1, levelNumber));
        float baseCurve = 1.0f + std::min(1.45f, (lv - 1.0f) * 0.11f);
        if (levelNumber >= 5)
            baseCurve += 0.05f;
        if (levelNumber >= 8)
            baseCurve += 0.09f;
        if (levelNumber >= 11)
            baseCurve += std::min(0.55f, (levelNumber - 10) * 0.05f);

        float hardcoreBonus = HARDCORE_MODE ? 0.25f : 0.0f;
        float dieAgainBonus = DIE_AGAIN_MODE ? 0.14f : 0.0f;
        difficultyMultiplier = baseCurve + hardcoreBonus + dieAgainBonus;
        enemyHpMultiplier = 1.0f + (difficultyMultiplier - 1.0f) * 0.88f;
        enemySpeedMultiplier = 1.0f + (difficultyMultiplier - 1.0f) * 0.52f;
        float spikeProfile = 1.0f;
        if (SPIKE_TEMPO_LEVEL == 0)
            spikeProfile = 0.82f;
        else if (SPIKE_TEMPO_LEVEL == 2)
            spikeProfile = 1.34f;
        if (OPEN_FLOOR_MODE >= 2)
            spikeProfile += 0.06f;
        trapIntensityMultiplier = (1.0f + (difficultyMultiplier - 1.0f) * 0.68f) * spikeProfile;

        if (difficultyMultiplier >= 2.65f)
            difficultyTitle = "NIGHTMARE";
        else if (difficultyMultiplier >= 2.25f)
            difficultyTitle = "MASTER";
        else if (difficultyMultiplier >= 1.9f)
            difficultyTitle = "EXPERT";
        else if (difficultyMultiplier >= 1.55f)
            difficultyTitle = "VETERAN";
        else if (difficultyMultiplier >= 1.3f)
            difficultyTitle = "CHALLENGER";
        else
            difficultyTitle = "ADVENTURER";

        if (levelNumber == 1)
        {
            chapterTitle = "Chapter I - Awakening";
            objectiveText = "Learn movement flow and establish rhythm.";
        }
        else if (levelNumber == 2)
        {
            chapterTitle = "Chapter II - Titan Gate";
            objectiveText = "Choose a loadout and break the inferno line.";
        }
        else if (levelNumber == 3)
        {
            chapterTitle = "Chapter III - Neon Climb";
            objectiveText = "Master vertical routes and fake platforms.";
        }
        else if (levelNumber == 4)
        {
            chapterTitle = "Chapter IV - Kinetic Hall";
            objectiveText = "Control speed through moving hazard corridors.";
        }
        else if (levelNumber == 5)
        {
            chapterTitle = "Chapter V - Breaker Run";
            objectiveText = "Survive collapse chains and force openings.";
        }
        else if (levelNumber == 6)
        {
            chapterTitle = "Chapter VI - Sky Bridges";
            objectiveText = "Execute precision jumps under ranged pressure.";
        }
        else if (levelNumber == 7)
        {
            chapterTitle = "Chapter VII - Factory";
            objectiveText = "Manage combat density with efficient routing.";
        }
        else if (levelNumber == 8)
        {
            chapterTitle = "Chapter VIII - Final Gauntlet";
            objectiveText = "Hold tempo through full-spectrum threats.";
        }
        else
        {
            chapterTitle = "Chapter Infinity - Endless Trials";
            objectiveText = "Adapt every cycle as threats keep escalating.";
        }

        for (auto &trap : spikeTraps)
        {
            trap.setIntensity(trapIntensityMultiplier);
        }

        for (auto &enemy : enemies)
        {
            int hp = static_cast<int>(std::round(enemy->baseHealth * enemyHpMultiplier));
            if (enemy->type == 3)
                hp += std::max(1, levelNumber / 2);
            else if (enemy->type == 4)
                hp += std::max(3, levelNumber);
            enemy->baseHealth = std::max(1, hp);
            enemy->health = enemy->baseHealth;

            enemy->baseSpeed *= enemySpeedMultiplier;
            enemy->speed = enemy->baseSpeed;
            enemy->baseDetectionRadius *= (1.0f + (difficultyMultiplier - 1.0f) * 0.30f);
            enemy->detectionRadius = enemy->baseDetectionRadius;
            enemy->baseAttackRange *= (1.0f + (difficultyMultiplier - 1.0f) * 0.16f);
            enemy->attackRange = enemy->baseAttackRange;
        }

        float scarcity = std::max(0.0f, difficultyMultiplier - 1.15f);
        if (levelNumber <= 2)
            scarcity *= 0.2f;
        else if (levelNumber <= 4)
            scarcity *= 0.45f;

        int heartTrimChance = static_cast<int>(std::min(55.0f, scarcity * 28.0f));
        for (auto it = hearts.begin(); it != hearts.end();)
        {
            if (hearts.size() > 1 && (rand() % 100) < heartTrimChance)
                it = hearts.erase(it);
            else
                ++it;
        }

        int gunTrimChance = static_cast<int>(std::min(44.0f, scarcity * 21.0f));
        for (auto it = guns.begin(); it != guns.end();)
        {
            if (guns.size() > 1 && (rand() % 100) < gunTrimChance)
                it = guns.erase(it);
            else
                ++it;
        }
    }

    float difficultyValue() const
    {
        return difficultyMultiplier;
    }

    const std::string &difficultyName() const
    {
        return difficultyTitle;
    }

    void setDirectorBias(float v)
    {
        directorBias = std::max(0.72f, std::min(1.55f, v));
    }

    float directorValue() const
    {
        return directorBias;
    }

    std::string directorMood() const
    {
        if (directorBias >= 1.35f)
            return "AGGRESSIVE";
        if (directorBias >= 1.15f)
            return "PRESSURE";
        if (directorBias >= 0.95f)
            return "BALANCED";
        return "RECOVERY";
    }

    const std::string &chapterName() const
    {
        return chapterTitle;
    }

    const std::string &objectiveHint() const
    {
        return objectiveText;
    }

    void update(float dt, Player &player, ParticleSystem &particles, std::vector<Popup> &popups,
               const std::function<void(float, float)> &shake)
    {
        for (auto &platform : platforms)
        {
            platform->update(dt);
        }

        auto pushPopup = [&](const sf::Vector2f &pos, const std::string &txt, sf::Color c)
        {
            popups.push_back({pos, sf::Vector2f(0, -70), 1.2f, 1.2f, txt, c});
        };
        auto applyDamageBurst = [&](int hits)
        {
            hits = std::max(0, hits);
            for (int i = 0; i < hits; i++)
            {
                if (player.lives <= 0)
                    break;
                player.invincibleTimer = 0.0f;
                player.hit();
            }
        };
        float giantWeaponScale = 1.0f;
        if (levelNumber >= 4)
        {
            giantWeaponScale += std::min(1.25f, static_cast<float>(levelNumber - 3) * 0.12f);
            if (HARDCORE_MODE)
                giantWeaponScale += 0.08f;
            if (DIE_AGAIN_MODE && levelNumber >= 6)
                giantWeaponScale += 0.05f;
        }
        giantWeaponScale *= (0.95f + directorBias * 0.08f);
        giantWeaponScale = std::max(1.0f, std::min(2.65f, giantWeaponScale));
        auto scaleGiantHits = [&](int baseHits, int enemyType) -> int
        {
            if (enemyType != 3 && enemyType != 4)
                return std::max(1, baseHits);
            int scaled = static_cast<int>(std::ceil(baseHits * giantWeaponScale));
            return clampInt(scaled, 1, 12);
        };

        for (auto &platform : platforms)
        {
            if (platform->type != 5 || !platform->broken)
                continue;
            if (platform->gateDamageCooldown > 0.0f)
                continue;

            sf::FloatRect openGap(platform->bounds.left + 8.0f, platform->bounds.top - 20.0f,
                                  std::max(16.0f, platform->bounds.width - 16.0f), platform->bounds.height + 24.0f);
            if (!player.getBounds().intersects(openGap))
                continue;

            int livesBefore = player.lives;
            int floorHits = DIE_AGAIN_MODE ? 2 : 1;
            if (OPEN_FLOOR_MODE >= 2)
                floorHits += 1;
            if (HARDCORE_MODE)
                floorHits += 1;
            applyDamageBurst(floorHits);
            if (player.lives < livesBefore)
            {
                player.velocity.y = std::max(player.velocity.y, DIE_AGAIN_MODE ? 460.0f : 380.0f);
                player.position.y += 8.0f;
                pushPopup(sf::Vector2f(player.position.x, platform->bounds.top - 16.0f), "FLOOR OPEN!", sf::Color(255, 155, 175));
                particles.addExplosion(sf::Vector2f(player.position.x, platform->bounds.top + 4.0f), sf::Color(255, 130, 160), 20);
                shake(0.18f, 10.0f);
            }
            platform->gateDamageCooldown = HARDCORE_MODE ? 0.42f : 0.58f;
        }

        for (auto &coin : coins)
        {
            coin->update(dt);
            if (!coin->collected && player.getBounds().intersects(coin->getBounds()))
            {
                coin->collected = true;
                if (coin->cursed)
                {
                    int livesBefore = player.lives;
                    int cursedHits = DIE_AGAIN_MODE ? (directorBias >= 1.14f ? 3 : 2) : (directorBias >= 1.34f ? 2 : 1);
                    applyDamageBurst(cursedHits);
                    if (player.lives < livesBefore)
                    {
                        player.velocity.y = DIE_AGAIN_MODE ? -420.0f : -320.0f;
                        if (player.position.x < coin->position.x)
                            player.velocity.x = DIE_AGAIN_MODE ? -300.0f : -220.0f;
                        else
                            player.velocity.x = DIE_AGAIN_MODE ? 300.0f : 220.0f;
                        pushPopup(coin->position + sf::Vector2f(0, -18),
                                  DIE_AGAIN_MODE ? "FAKE COIN!" : "CURSED",
                                  sf::Color(255, 130, 170));
                        particles.addExplosion(coin->position, sf::Color(255, 90, 140), DIE_AGAIN_MODE ? 28 : 18);
                        shake(DIE_AGAIN_MODE ? 0.22f : 0.16f, DIE_AGAIN_MODE ? 14.0f : 10.0f);
                    }
                }
                else
                {
                    player.coins += coin->value;
                    int gained = player.addScore(coin->value * 100);
                    pushPopup(coin->position, "+" + std::to_string(gained), coin->color);
                    particles.addCollectEffect(coin->position, coin->color);
                }
            }
        }

        for (auto &heart : hearts)
        {
            heart->update(dt);
            if (!heart->collected && player.getBounds().intersects(heart->getBounds()))
            {
                heart->collected = true;
                player.lives = std::min(MAX_LIVES, player.lives + 1);
                int gained = player.addScore(500);
                pushPopup(heart->position, "+" + std::to_string(gained), sf::Color(255, 80, 120));
                particles.addCollectEffect(heart->position, sf::Color(255, 80, 120));
            }
        }

        for (auto &gun : guns)
        {
            gun->update(dt);
            if (!gun->collected && player.getBounds().intersects(gun->getBounds()))
            {
                gun->collected = true;
                player.hasGun = true;
                player.gunTimer = GUN_DURATION;
                player.gunCooldown = 0.0f;
                player.gunCharging = false;
                player.gunCharge = 0.0f;
                int gained = player.addScore(400);
                pushPopup(gun->position, "+" + std::to_string(gained), sf::Color(255, 140, 60));
                particles.addCollectEffect(gun->position, sf::Color(255, 140, 60));
            }
        }

        for (auto &trap : spikeTraps)
        {
            trap.setIntensity(trapIntensityMultiplier * (0.82f + directorBias * 0.40f));
            trap.update(dt, player);
            if (trap.isDangerous() && player.getBounds().intersects(trap.hitBounds()))
            {
                int livesBefore = player.lives;
                int trapHits = DIE_AGAIN_MODE ? (directorBias >= 1.08f ? 2 : 1) : 1;
                applyDamageBurst(trapHits);
                if (player.lives < livesBefore)
                {
                    sf::Vector2f c = trap.center();
                    player.velocity.y = DIE_AGAIN_MODE ? -460.0f : -380.0f;
                    if (player.position.x < c.x)
                        player.velocity.x = DIE_AGAIN_MODE ? -320.0f : -250.0f;
                    else
                        player.velocity.x = DIE_AGAIN_MODE ? 320.0f : 250.0f;

                    particles.addExplosion(c + sf::Vector2f(0, -28.0f), sf::Color(255, 90, 130), DIE_AGAIN_MODE ? 26 : 20);
                    pushPopup(c + sf::Vector2f(0, -48.0f),
                              DIE_AGAIN_MODE ? "AMBUSH!" : "TRAP!",
                              sf::Color(255, 120, 140));
                    float shakeMul = 0.9f + directorBias * 0.28f;
                    shake((DIE_AGAIN_MODE ? 0.24f : 0.2f) * shakeMul, (DIE_AGAIN_MODE ? 15.0f : 12.0f) * shakeMul);
                }
            }
        }

        for (auto it = enemyFireballs.begin(); it != enemyFireballs.end();)
        {
            it->lifetime -= dt;
            it->position += it->velocity * dt;

            bool remove = false;
            if (it->lifetime <= 0.0f ||
                it->position.x < -40.0f || it->position.x > WIDTH + 40.0f ||
                it->position.y < -40.0f || it->position.y > HEIGHT + 40.0f)
            {
                remove = true;
            }

            sf::FloatRect shotBounds(it->position.x - it->size, it->position.y - it->size, it->size * 2.0f, it->size * 2.0f);
            if (!remove && !it->pierceWalls)
            {
                for (auto &platform : platforms)
                {
                    if (platform->broken)
                        continue;
                    if (shotBounds.intersects(platform->bounds))
                    {
                        particles.addExplosion(it->position, sf::Color(255, 130, 70), 10);
                        remove = true;
                        break;
                    }
                }
            }

            if (!remove && shotBounds.intersects(player.getBounds()))
            {
                int livesBefore = player.lives;
                applyDamageBurst(std::max(1, it->damage));
                if (player.lives < livesBefore)
                {
                    float fireImpact = 1.0f + std::max(0, it->damage - 3) * 0.10f;
                    fireImpact = std::min(1.85f, fireImpact);
                    pushPopup(player.position + sf::Vector2f(0, -44), "FIREBALL!", sf::Color(255, 145, 95));
                    particles.addExplosion(player.position + sf::Vector2f(0, -8), sf::Color(255, 110, 70), 24);
                    shake(0.25f * fireImpact, 13.5f * fireImpact);
                    if (player.position.x < it->position.x)
                        player.velocity.x = -340.0f * fireImpact;
                    else
                        player.velocity.x = 340.0f * fireImpact;
                    player.velocity.y = std::min(player.velocity.y, -220.0f * fireImpact);
                }
                remove = true;
            }

            if (remove)
                it = enemyFireballs.erase(it);
            else
                ++it;
        }

        for (auto it = enemies.begin(); it != enemies.end();)
        {
            // Update enemy with player context
            (*it)->setDirectorBias(directorBias);
            (*it)->update(dt, player, platforms);

            if (!(*it)->alive)
            {
                // enemy already dead -> remove and explosion
                particles.addExplosion((*it)->position, sf::Color::Yellow, 25);
                it = enemies.erase(it);
                continue;
            }

            sf::Vector2f fireOrigin;
            sf::Vector2f fireVel;
            int fireDamage = 0;
            if ((*it)->consumeRangedShot(fireOrigin, fireVel, fireDamage))
            {
                EnemyFireball fb;
                fb.position = fireOrigin;
                fb.velocity = fireVel;
                fb.lifetime = 2.9f;
                fb.size = 10.0f;
                fb.damage = fireDamage;
                fb.color = sf::Color(255, 130, 65);
                fb.pierceWalls = false;
                if ((*it)->type == 4 && levelNumber >= 4)
                {
                    float velocityScale = 1.0f + (giantWeaponScale - 1.0f) * 0.72f;
                    float sizeScale = 1.0f + (giantWeaponScale - 1.0f) * 0.45f;
                    float lifeScale = 1.0f + (giantWeaponScale - 1.0f) * 0.36f;
                    fb.velocity *= velocityScale;
                    fb.size *= sizeScale;
                    fb.lifetime *= lifeScale;
                    fb.damage = clampInt(static_cast<int>(std::ceil(fb.damage * (1.0f + (giantWeaponScale - 1.0f) * 0.9f))), 1, 12);
                    fb.pierceWalls = true;
                }
                enemyFireballs.push_back(fb);

                particles.addExplosion(fireOrigin, sf::Color(255, 145, 85), 9);
                pushPopup(fireOrigin + sf::Vector2f(0, -18), "INFERNO SHOT", sf::Color(255, 170, 120));
                shake(0.08f, 4.0f);
            }

            // Collision handling: if player lands on enemy
            if (player.getBounds().intersects((*it)->getBounds()))
            {
                // player above enemy and moving down => stomp
                if (player.velocity.y > 0 && player.position.y < (*it)->position.y)
                {
                    // instead of immediate erase, apply damage + knockback
                    (*it)->takeDamage(1, sf::Vector2f(0, -300.0f));
                    player.addSoul(SOUL_PER_HIT * 0.7f);
                    player.velocity.y = -450.0f;
                    int gained = player.addScore(200);
                    pushPopup((*it)->position, "+" + std::to_string(gained), sf::Color::Yellow);
                    shake(0.18f, 10.0f);
                    particles.addExplosion((*it)->position, sf::Color::Yellow, 20);

                    // if enemy died, remove it next iteration (takeDamage set alive=false)
                    if (!(*it)->alive)
                    {
                        it = enemies.erase(it);
                        continue;
                    }
                }
                else
                {
                    // Player hit from side -> player takes damage
                    int directorContactBonus = (directorBias >= 1.28f) ? 1 : 0;
                    int contactHits = (DIE_AGAIN_MODE ? 2 : 1);
                    if ((*it)->type == 3)
                        contactHits = DIE_AGAIN_MODE ? 3 : 2;
                    else if ((*it)->type == 4)
                        contactHits = DIE_AGAIN_MODE ? 5 : 4;
                    contactHits = scaleGiantHits(contactHits, (*it)->type);
                    contactHits += directorContactBonus;
                    applyDamageBurst(contactHits);
                    if ((*it)->type == 3)
                        pushPopup(player.position + sf::Vector2f(0, -34), "TITAN HIT", sf::Color(255, 120, 140));
                    else if ((*it)->type == 4)
                        pushPopup(player.position + sf::Vector2f(0, -34), "INFERNO CRUSH", sf::Color(255, 145, 95));
                    particles.addExplosion(player.position, sf::Color::Red, 20);
                    shake(0.22f, 12.0f);
                    // small knockback to player
                    if (player.position.x < (*it)->position.x)
                        player.position.x -= 30;
                    else
                        player.position.x += 30;
                }
            }

            ++it;
        }
    }

    void draw(sf::RenderWindow &window)
    {
        for (auto &platform : platforms)
        {
            platform->draw(window);
        }
        for (auto &trap : spikeTraps)
        {
            trap.draw(window);
        }
        for (auto &coin : coins)
        {
            coin->draw(window);
        }
        for (auto &heart : hearts)
        {
            heart->draw(window);
        }
        for (auto &gun : guns)
        {
            gun->draw(window);
        }
        for (auto &shot : enemyFireballs)
        {
            sf::CircleShape glow(shot.size * 1.9f);
            glow.setOrigin(glow.getRadius(), glow.getRadius());
            glow.setPosition(shot.position);
            glow.setFillColor(sf::Color(255, 120, 60, 75));
            window.draw(glow);

            sf::CircleShape fireball(shot.size);
            fireball.setOrigin(fireball.getRadius(), fireball.getRadius());
            fireball.setPosition(shot.position);
            fireball.setFillColor(shot.color);
            fireball.setOutlineThickness(1.5f);
            fireball.setOutlineColor(sf::Color(255, 235, 170));
            window.draw(fireball);
        }
        for (auto &enemy : enemies)
        {
            enemy->draw(window);
        }
    }

    Enemy *activeBoss()
    {
        for (auto &enemy : enemies)
        {
            if (enemy->alive && enemy->type == 4)
                return enemy.get();
        }
        for (auto &enemy : enemies)
        {
            if (enemy->alive && enemy->type == 3)
                return enemy.get();
        }
        return nullptr;
    }

    int totalRelics() const
    {
        int total = 0;
        for (const auto &coin : coins)
        {
            if (!coin->cursed)
                total++;
        }
        return total;
    }

    int remainingRelics() const
    {
        int left = 0;
        for (const auto &coin : coins)
        {
            if (!coin->cursed && !coin->collected)
                left++;
        }
        return left;
    }

    int enemyCount() const
    {
        return static_cast<int>(enemies.size());
    }

    bool isCompleted()
    {
        bool allCoinsCollected = true;
        for (auto &coin : coins)
        {
            if (coin->cursed)
                continue;
            if (!coin->collected)
            {
                allCoinsCollected = false;
                break;
            }
        }
        return allCoinsCollected && enemies.empty();
    }
};
////////////////////////////////////////////////////////////////////////////
// Parallax Background
