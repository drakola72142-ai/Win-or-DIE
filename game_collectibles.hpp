#pragma once
#include "game_common.hpp"

class Coin
{
public:
    sf::Vector2f position;
    float size;
    sf::Color color;
    float rotation;
    bool collected;
    int value;
    bool cursed;
    float pulseTimer;

    Coin(float x, float y, int v = 1, bool isCursed = false)
        : position(x, y), size(15), rotation(0), collected(false), value(v), cursed(isCursed), pulseTimer(static_cast<float>(rand() % 100) / 12.0f)
    {
        if (cursed)
        {
            color = sf::Color(245, 90, 135);
            size = 17;
        }
        else
        {
            color = (v == 1) ? sf::Color(206, 190, 120) : sf::Color(150, 180, 255);
        }
        if (v > 1 && !cursed)
            size = 20;
    }

    void update(float dt)
    {
        rotation += 180 * dt;
        pulseTimer += dt * 4.0f;
    }

    void draw(sf::RenderWindow &window)
    {
        if (collected)
            return;

        // Glow
        sf::CircleShape glow(size * 1.5f);
        glow.setPosition(position - sf::Vector2f(size * 0.5f, size * 0.5f));
        sf::Uint8 glowAlpha = cursed ? static_cast<sf::Uint8>(75 + (0.5f + 0.5f * std::sin(pulseTimer)) * 85.0f) : 50;
        glow.setFillColor(sf::Color(color.r, color.g, color.b, glowAlpha));
        window.draw(glow);

        // Coin
        sf::CircleShape coin(size);
        coin.setPosition(position - sf::Vector2f(size, size));
        coin.setFillColor(color);
        coin.setOutlineThickness(2);
        coin.setOutlineColor(sf::Color(230, 235, 250));
        coin.setPointCount(8);
        window.draw(coin);

        if (cursed)
        {
            sf::RectangleShape slashA(sf::Vector2f(size * 1.4f, 2.2f));
            slashA.setOrigin(slashA.getSize() * 0.5f);
            slashA.setPosition(position);
            slashA.setRotation(35.0f);
            slashA.setFillColor(sf::Color(255, 235, 245, 220));
            window.draw(slashA);

            sf::RectangleShape slashB(sf::Vector2f(size * 1.4f, 2.2f));
            slashB.setOrigin(slashB.getSize() * 0.5f);
            slashB.setPosition(position);
            slashB.setRotation(-35.0f);
            slashB.setFillColor(sf::Color(255, 235, 245, 220));
            window.draw(slashB);
        }
    }

    sf::FloatRect getBounds()
    {
        return sf::FloatRect(position.x - size, position.y - size, size * 2, size * 2);
    }
};
///////////////////////////////////////////////////////////////////////////////////////////
// Heart collectible (adds a life)
class Heart
{
public:
    sf::Vector2f position;
    float size;
    bool collected;
    sf::Color color;
    float floatTimer;

    Heart(float x, float y, float s = 14.0f)
        : position(x, y), size(s), collected(false),
          color(255, 80, 120), floatTimer(static_cast<float>(rand() % 100) / 10.0f) {}

    void update(float dt)
    {
        floatTimer += dt * 2.0f;
    }

    void draw(sf::RenderWindow &window)
    {
        if (collected)
            return;

        float bob = std::sin(floatTimer) * 4.0f;
        sf::Vector2f drawPos = position + sf::Vector2f(0, bob);

        // Glow
        sf::CircleShape glow(size * 0.9f);
        glow.setPosition(drawPos - sf::Vector2f(size * 0.9f, size * 0.9f));
        glow.setFillColor(sf::Color(color.r, color.g, color.b, 60));
        window.draw(glow);

        float r = size * 0.55f;

        // Left lobe
        sf::CircleShape left(r);
        left.setPosition(drawPos.x - r * 1.1f, drawPos.y - r * 0.9f);
        left.setFillColor(color);
        left.setOutlineThickness(2);
        left.setOutlineColor(sf::Color::White);
        window.draw(left);

        // Right lobe
        sf::CircleShape right(r);
        right.setPosition(drawPos.x + r * 0.1f, drawPos.y - r * 0.9f);
        right.setFillColor(color);
        right.setOutlineThickness(2);
        right.setOutlineColor(sf::Color::White);
        window.draw(right);

        // Bottom triangle
        sf::ConvexShape bottom;
        bottom.setPointCount(3);
        bottom.setPoint(0, sf::Vector2f(drawPos.x - r * 1.4f, drawPos.y));
        bottom.setPoint(1, sf::Vector2f(drawPos.x + r * 1.4f, drawPos.y));
        bottom.setPoint(2, sf::Vector2f(drawPos.x, drawPos.y + r * 1.8f));
        bottom.setFillColor(color);
        bottom.setOutlineThickness(2);
        bottom.setOutlineColor(sf::Color::White);
        window.draw(bottom);
    }

    sf::FloatRect getBounds()
    {
        return sf::FloatRect(position.x - size, position.y - size, size * 2, size * 2);
    }
};
///////////////////////////////////////////////////////////////////////////////////////////
// Gun power-up (temporary)
class GunPickup
{
public:
    sf::Vector2f position;
    float size;
    bool collected;
    sf::Color color;
    float pulseTimer;

    GunPickup(float x, float y, float s = 12.0f)
        : position(x, y), size(s), collected(false),
          color(255, 140, 60), pulseTimer(static_cast<float>(rand() % 100) / 10.0f) {}

    void update(float dt)
    {
        pulseTimer += dt * 3.0f;
    }

    void draw(sf::RenderWindow &window)
    {
        if (collected)
            return;

        float glow = 40 + (std::sin(pulseTimer) * 0.5f + 0.5f) * 60;
        sf::CircleShape aura(size * 1.2f);
        aura.setPosition(position - sf::Vector2f(size * 1.2f, size * 1.2f));
        aura.setFillColor(sf::Color(color.r, color.g, color.b, (int)glow));
        window.draw(aura);

        // Gun body
        sf::RectangleShape body(sf::Vector2f(size * 1.4f, size * 0.6f));
        body.setPosition(position.x - size * 0.7f, position.y - size * 0.3f);
        body.setFillColor(color);
        body.setOutlineThickness(2);
        body.setOutlineColor(sf::Color::White);
        window.draw(body);

        // Barrel
        sf::RectangleShape barrel(sf::Vector2f(size * 0.7f, size * 0.25f));
        barrel.setPosition(position.x + size * 0.6f, position.y - size * 0.2f);
        barrel.setFillColor(sf::Color(255, 200, 120));
        window.draw(barrel);

        // Handle
        sf::RectangleShape handle(sf::Vector2f(size * 0.35f, size * 0.7f));
        handle.setPosition(position.x - size * 0.2f, position.y + size * 0.2f);
        handle.setFillColor(sf::Color(60, 60, 70));
        handle.setOutlineThickness(1);
        handle.setOutlineColor(sf::Color::White);
        window.draw(handle);
    }

    sf::FloatRect getBounds()
    {
        return sf::FloatRect(position.x - size, position.y - size, size * 2, size * 2);
    }
};
///////////////////////////////////////////////////////////////////////////////////////////
// Bullet fired by player
