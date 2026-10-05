#pragma once
#include "game_player.hpp"

class SpikeTrap
{
public:
    enum class Phase
    {
        Idle,
        Telegraph,
        Attack,
        Recover
    };

    sf::FloatRect base;
    float maxRise;
    float currentRise;
    float triggerRange;
    int spikeCount;
    float cooldownTimer;
    float phaseTimer;
    float animTimer;
    float scheduleTimer;
    float scheduleBias;
    float intensity;
    Phase phase;

    SpikeTrap(float x, float y, float w, float rise = 64.0f, int count = 6, float trigger = 95.0f)
        : base(x, y, w, 14.0f), maxRise(rise), currentRise(0.0f), triggerRange(trigger),
          spikeCount(std::max(3, count)), cooldownTimer(0.0f), phaseTimer(0.0f), animTimer(0.0f),
          scheduleTimer(static_cast<float>(rand() % 95) / 100.0f),
          scheduleBias(0.88f + static_cast<float>(rand() % 35) / 100.0f),
          intensity(1.0f), phase(Phase::Idle) {}

    void setIntensity(float v)
    {
        intensity = std::max(0.65f, std::min(2.35f, v));
    }

    sf::Vector2f center() const
    {
        return sf::Vector2f(base.left + base.width * 0.5f, base.top);
    }

    bool canTrigger(const Player &player) const
    {
        sf::Vector2f c = center();
        float rangeBoost = 0.9f + intensity * 0.12f;
        bool inX = std::abs(player.position.x - c.x) <= triggerRange * rangeBoost;
        float lowerReach = (DIE_AGAIN_MODE ? 210.0f : 170.0f) + (intensity - 1.0f) * 34.0f;
        float upperReach = (DIE_AGAIN_MODE ? 290.0f : 250.0f) + (intensity - 1.0f) * 45.0f;
        bool inY = player.position.y < base.top + lowerReach && player.position.y > base.top - upperReach;
        return inX && inY;
    }

    void update(float dt, const Player &player)
    {
        animTimer += dt;
        float riseSpeed = (HARDCORE_MODE ? 1060.0f : (DIE_AGAIN_MODE ? 900.0f : 760.0f)) * intensity;
        float recoverSpeed = (HARDCORE_MODE ? 520.0f : (DIE_AGAIN_MODE ? 460.0f : 360.0f)) * (0.9f + intensity * 0.1f);
        float telegraphTime = (HARDCORE_MODE ? 0.1f : (DIE_AGAIN_MODE ? 0.15f : 0.22f)) / std::max(0.7f, intensity);
        float attackDuration = (HARDCORE_MODE ? 0.62f : (DIE_AGAIN_MODE ? 0.54f : 0.42f)) * (0.95f + intensity * 0.12f);
        float cooldownDuration = (HARDCORE_MODE ? 0.62f : (DIE_AGAIN_MODE ? 0.78f : 1.0f)) / std::max(0.75f, intensity);
        float triggerInterval = (HARDCORE_MODE ? 0.90f : (DIE_AGAIN_MODE ? 1.05f : 1.25f)) * scheduleBias / std::max(0.78f, intensity);
        triggerInterval = std::max(0.45f, triggerInterval);

        if (cooldownTimer > 0.0f)
        {
            cooldownTimer = std::max(0.0f, cooldownTimer - dt);
        }

        if (phase == Phase::Idle)
        {
            currentRise = std::max(0.0f, currentRise - dt * 240.0f);
            scheduleTimer += dt;
            bool triggerWindow = false;
            while (scheduleTimer >= triggerInterval)
            {
                scheduleTimer -= triggerInterval;
                triggerWindow = true;
            }

            if (triggerWindow && cooldownTimer <= 0.0f && canTrigger(player))
            {
                phase = Phase::Telegraph;
                float jitter = 0.8f + static_cast<float>(rand() % 35) / 100.0f;
                phaseTimer = telegraphTime * jitter;
            }
            return;
        }

        if (phase == Phase::Telegraph)
        {
            phaseTimer -= dt;
            if (phaseTimer <= 0.0f)
            {
                phase = Phase::Attack;
                phaseTimer = attackDuration;
            }
            return;
        }

        if (phase == Phase::Attack)
        {
            currentRise = std::min(maxRise, currentRise + dt * riseSpeed);
            phaseTimer -= dt;
            if (phaseTimer <= 0.0f)
            {
                phase = Phase::Recover;
                phaseTimer = 0.65f;
            }
            return;
        }

        // Recover
        currentRise = std::max(0.0f, currentRise - dt * recoverSpeed);
        phaseTimer -= dt;
        if (phaseTimer <= 0.0f && currentRise <= 0.1f)
        {
            currentRise = 0.0f;
            phase = Phase::Idle;
            cooldownTimer = cooldownDuration;
        }
    }

    bool isDangerous() const
    {
        float dangerThreshold = (DIE_AGAIN_MODE ? 0.22f : 0.32f) - (intensity - 1.0f) * 0.05f;
        dangerThreshold = std::max(0.12f, dangerThreshold);
        return (phase == Phase::Attack || phase == Phase::Recover) && currentRise > maxRise * dangerThreshold;
    }

    sf::FloatRect hitBounds() const
    {
        return sf::FloatRect(base.left, base.top - currentRise, base.width, std::max(1.0f, currentRise));
    }

    void draw(sf::RenderWindow &window)
    {
        sf::RectangleShape housing(sf::Vector2f(base.width, base.height));
        housing.setPosition(base.left, base.top);
        housing.setFillColor(sf::Color(40, 48, 66));
        housing.setOutlineThickness(2.0f);
        housing.setOutlineColor(sf::Color(145, 160, 188));
        window.draw(housing);

        if (phase == Phase::Telegraph)
        {
            float blink = 0.5f + 0.5f * std::sin(animTimer * 45.0f);
            sf::RectangleShape warn(sf::Vector2f(base.width, base.height));
            warn.setPosition(base.left, base.top);
            warn.setFillColor(sf::Color(255, 90, 120, static_cast<sf::Uint8>(40 + blink * 90.0f)));
            window.draw(warn);

            sf::ConvexShape warnTri;
            warnTri.setPointCount(3);
            warnTri.setPoint(0, sf::Vector2f(base.left + base.width * 0.5f, base.top - 17.0f));
            warnTri.setPoint(1, sf::Vector2f(base.left + base.width * 0.5f - 9.0f, base.top - 2.0f));
            warnTri.setPoint(2, sf::Vector2f(base.left + base.width * 0.5f + 9.0f, base.top - 2.0f));
            warnTri.setFillColor(sf::Color(255, 235, 140, static_cast<sf::Uint8>(120 + blink * 120.0f)));
            window.draw(warnTri);
        }

        if (currentRise <= 0.1f)
            return;

        float step = base.width / static_cast<float>(spikeCount);
        for (int i = 0; i < spikeCount; i++)
        {
            float left = base.left + i * step;
            sf::ConvexShape spike;
            spike.setPointCount(3);
            spike.setPoint(0, sf::Vector2f(left, base.top));
            spike.setPoint(1, sf::Vector2f(left + step, base.top));
            spike.setPoint(2, sf::Vector2f(left + step * 0.5f, base.top - currentRise));
            spike.setFillColor(sf::Color(228, 238, 248));
            spike.setOutlineThickness(1.5f);
            spike.setOutlineColor(sf::Color(255, 110, 150));
            window.draw(spike);
        }
    }
};

//////////////////////////////////////////////////////////////////////////////////////////
// Level Generator
