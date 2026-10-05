#pragma once
#include "game_common.hpp"

class Platform
{
public:
    sf::FloatRect bounds;
    sf::Color color;
    int type; // 0: normal, 1: moving, 2: breakable, 3: bounce, 4: fake/troll, 5: opening floor gate
    sf::Vector2f velocity;
    sf::Vector2f startPos;
    float moveDistance;
    bool broken;
    float breakTimer;
    bool fakeTriggered;
    float fakeTimer;
    float gateTimer;
    bool gateOpen;
    float gateVisual;
    float gateDamageCooldown;

    Platform(float x, float y, float w, float h, int t = 0)
        : bounds(x, y, w, h), type(t), broken(false), breakTimer(0), fakeTriggered(false), fakeTimer(0),
          gateTimer(0.0f), gateOpen(false), gateVisual(0.0f), gateDamageCooldown(0.0f)
    {
        startPos = sf::Vector2f(x, y);
        moveDistance = 200;
        setType(t);
    }

    void setType(int t)
    {
        type = t;
        broken = false;
        breakTimer = 0.0f;
        fakeTriggered = false;
        fakeTimer = 0.0f;
        gateTimer = static_cast<float>(rand() % 100) / 140.0f;
        gateOpen = false;
        gateVisual = 0.0f;
        gateDamageCooldown = 0.0f;
        velocity = sf::Vector2f(0.0f, 0.0f);

        switch (type)
        {
        case 0:
            color = sf::Color(56, 69, 88);
            break;
        case 1:
            color = sf::Color(88, 110, 148);
            velocity = sf::Vector2f(100, 0);
            break;
        case 2:
            color = sf::Color(95, 60, 72);
            break;
        case 3:
            color = sf::Color(94, 132, 162);
            break;
        case 4:
            color = sf::Color(132, 88, 134);
            break;
        case 5:
            color = sf::Color(85, 62, 72);
            break;
        }

        if (type == 1)
        {
            velocity = sf::Vector2f(100, 0);
        }
    }

    void update(float dt)
    {
        gateDamageCooldown = std::max(0.0f, gateDamageCooldown - dt);
        if (type == 1 && !broken)
        {
            bounds.left += velocity.x * dt;
            if (bounds.left > startPos.x + moveDistance || bounds.left < startPos.x - moveDistance)
            {
                velocity.x *= -1;
            }
        }

        if (type == 5)
        {
            float tempoMul = 1.0f;
            if (SPIKE_TEMPO_LEVEL == 0)
                tempoMul = 0.82f;
            else if (SPIKE_TEMPO_LEVEL == 2)
                tempoMul = 1.35f;

            float closedWindow = (DIE_AGAIN_MODE ? 1.00f : 1.22f) / tempoMul;
            float openWindow = (DIE_AGAIN_MODE ? 0.70f : 0.52f) / std::max(0.7f, tempoMul);
            if (OPEN_FLOOR_MODE >= 2)
            {
                closedWindow *= 0.78f;
                openWindow *= 1.24f;
            }
            if (HARDCORE_MODE)
            {
                closedWindow *= 0.86f;
                openWindow *= 1.18f;
            }

            gateTimer += dt;
            if (!gateOpen)
            {
                if (gateTimer >= closedWindow)
                {
                    gateOpen = true;
                    gateTimer = 0.0f;
                }
            }
            else if (gateTimer >= openWindow)
            {
                gateOpen = false;
                gateTimer = 0.0f;
            }

            broken = gateOpen;
            fakeTriggered = gateOpen;
            float targetVisual = gateOpen ? 1.0f : 0.0f;
            gateVisual += (targetVisual - gateVisual) * std::min(1.0f, dt * 9.0f);
            return;
        }

        if (type == 4 && fakeTriggered && !broken)
        {
            fakeTimer += dt;
            float tempoMul = 1.0f;
            if (SPIKE_TEMPO_LEVEL == 0)
                tempoMul = 0.86f;
            else if (SPIKE_TEMPO_LEVEL == 2)
                tempoMul = 1.22f;
            float fakeBreakDelay = (HARDCORE_MODE ? 0.16f : (DIE_AGAIN_MODE ? 0.24f : 0.35f)) / tempoMul;
            if (fakeTimer > fakeBreakDelay)
            {
                broken = true;
                breakTimer = 0.0f;
            }
        }

        if (broken && type == 2)
        {
            breakTimer += dt;
            if (breakTimer > 2.0f)
            {
                broken = false;
                breakTimer = 0;
            }
        }

        if (broken && type == 4)
        {
            breakTimer += dt;
            float tempoMul = 1.0f;
            if (SPIKE_TEMPO_LEVEL == 0)
                tempoMul = 0.9f;
            else if (SPIKE_TEMPO_LEVEL == 2)
                tempoMul = 1.28f;
            float rebuildTime = (HARDCORE_MODE ? 3.4f : (DIE_AGAIN_MODE ? 2.8f : 2.3f)) / tempoMul;
            if (breakTimer > rebuildTime)
            {
                broken = false;
                breakTimer = 0.0f;
                fakeTriggered = false;
                fakeTimer = 0.0f;
            }
        }
    }

    void draw(sf::RenderWindow &window)
    {
        if (broken && (type == 2 || type == 4))
            return;

        // Shadow
        sf::RectangleShape shadow(sf::Vector2f(bounds.width, bounds.height));
        shadow.setPosition(bounds.left + 5, bounds.top + 5);
        shadow.setFillColor(sf::Color(0, 0, 0, 50));
        window.draw(shadow);

        // Platform
        sf::RectangleShape platform(sf::Vector2f(bounds.width, bounds.height));
        platform.setPosition(bounds.left, bounds.top);
        platform.setFillColor(color);
        platform.setOutlineThickness(3);
        platform.setOutlineColor(sf::Color(185, 198, 222));

        if (type == 4)
        {
            float pulse = 0.5f + 0.5f * std::sin(fakeTimer * 22.0f);
            sf::Uint8 alpha = static_cast<sf::Uint8>(80 + pulse * 120.0f);
            platform.setOutlineColor(sf::Color(255, 140, 210, alpha));
        }
        else if (type == 5)
        {
            float pulse = 0.5f + 0.5f * std::sin(gateTimer * 9.0f);
            sf::Uint8 alpha = static_cast<sf::Uint8>(95 + pulse * 120.0f);
            platform.setOutlineColor(sf::Color(255, 138, 152, alpha));
            sf::Uint8 fillBoost = static_cast<sf::Uint8>(40 + gateVisual * 65.0f);
            platform.setFillColor(sf::Color(color.r + fillBoost / 3, color.g, color.b, 255));
        }

        // Glow effect
        if (type == 3)
        {
            sf::RectangleShape glow(sf::Vector2f(bounds.width + 10, bounds.height + 10));
            glow.setPosition(bounds.left - 5, bounds.top - 5);
            glow.setFillColor(sf::Color(color.r, color.g, color.b, 50));
            window.draw(glow);
        }
        else if (type == 4)
        {
            sf::RectangleShape glow(sf::Vector2f(bounds.width + 10, bounds.height + 10));
            glow.setPosition(bounds.left - 5, bounds.top - 5);
            sf::Uint8 alpha = fakeTriggered ? 85 : 45;
            glow.setFillColor(sf::Color(220, 100, 220, alpha));
            window.draw(glow);
        }
        else if (type == 5)
        {
            sf::RectangleShape glow(sf::Vector2f(bounds.width + 10, bounds.height + 10));
            glow.setPosition(bounds.left - 5, bounds.top - 5);
            sf::Uint8 alpha = static_cast<sf::Uint8>(55 + gateVisual * 95.0f);
            glow.setFillColor(sf::Color(255, 105, 128, alpha));
            window.draw(glow);
        }

        window.draw(platform);

        if (type == 5)
        {
            float leftDoorW = std::max(0.0f, bounds.width * 0.5f * (1.0f - gateVisual));
            float rightDoorW = leftDoorW;
            float gapW = std::max(0.0f, bounds.width - leftDoorW - rightDoorW);

            sf::RectangleShape leftDoor(sf::Vector2f(leftDoorW, bounds.height));
            leftDoor.setPosition(bounds.left, bounds.top);
            leftDoor.setFillColor(sf::Color(108, 74, 90));
            window.draw(leftDoor);

            sf::RectangleShape rightDoor(sf::Vector2f(rightDoorW, bounds.height));
            rightDoor.setPosition(bounds.left + bounds.width - rightDoorW, bounds.top);
            rightDoor.setFillColor(sf::Color(108, 74, 90));
            window.draw(rightDoor);

            if (gapW > 1.0f)
            {
                sf::RectangleShape gap(sf::Vector2f(gapW, bounds.height));
                gap.setPosition(bounds.left + leftDoorW, bounds.top);
                gap.setFillColor(sf::Color(12, 8, 14, 240));
                window.draw(gap);
            }

            sf::RectangleShape warning(sf::Vector2f(bounds.width, 2.0f));
            warning.setPosition(bounds.left, bounds.top - 2.0f);
            warning.setFillColor(sf::Color(255, 170, 180, static_cast<sf::Uint8>(110 + gateVisual * 120.0f)));
            window.draw(warning);
            return;
        }

        // Grid pattern
        for (int i = 0; i < bounds.width; i += 20)
        {
            sf::RectangleShape line(sf::Vector2f(2, bounds.height));
            line.setPosition(bounds.left + i, bounds.top);
            line.setFillColor(sf::Color(210, 225, 255, 20));
            window.draw(line);
        }

        if (type == 4)
        {
            for (int i = 8; i < static_cast<int>(bounds.width) - 8; i += 26)
            {
                sf::RectangleShape crack(sf::Vector2f(14, 2));
                crack.setPosition(bounds.left + i, bounds.top + bounds.height * 0.55f);
                crack.setFillColor(sf::Color(250, 180, 250, 110));
                crack.setRotation((i % 52 == 0) ? -20.0f : 20.0f);
                window.draw(crack);
            }
        }
    }
};

/////////////////////////////////////////////////////////////////////////////
// Collectible items
