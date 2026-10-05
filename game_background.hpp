#pragma once
#include "game_common.hpp"

class ParallaxBackground
{
    struct Layer
    {
        std::vector<sf::RectangleShape> shapes;
        float speed;
        float baseY;
        float variance;
    };

    struct Star
    {
        sf::Vector2f position;
        float radius;
        float twinkleSpeed;
        float phase;
        sf::Uint8 baseAlpha;
    };

    std::vector<Layer> layers;
    std::vector<Star> stars;
    std::vector<sf::RectangleShape> fogBands;
    int qualityLevel;
    float ambienceTime;

    void rebuild()
    {
        layers.clear();
        stars.clear();
        fogBands.clear();

        int layerCount = 2 + qualityLevel;
        for (int l = 0; l < layerCount; l++)
        {
            Layer layer;
            layer.speed = 9.0f + l * 12.0f + qualityLevel * 2.4f;
            layer.baseY = HEIGHT - 290.0f + l * 58.0f;
            layer.variance = 36.0f + l * 16.0f;

            int shapeCount = 5 + qualityLevel * 2 + l * 2;
            for (int i = 0; i < shapeCount; i++)
            {
                sf::RectangleShape shape;
                float w = 145.0f + static_cast<float>(rand() % 250) + qualityLevel * 18.0f;
                float h = 82.0f + static_cast<float>(rand() % 220) + l * 12.0f;
                float jitter = (static_cast<float>(rand() % 1000) / 999.0f - 0.5f) * layer.variance * 2.0f;
                float x = static_cast<float>(i * (WIDTH / std::max(1, shapeCount))) + static_cast<float>(rand() % 180);

                shape.setSize(sf::Vector2f(w, h));
                shape.setPosition(x, layer.baseY + jitter);
                shape.setFillColor(sf::Color(14 + l * 8 + qualityLevel * 4,
                                             22 + l * 10 + qualityLevel * 5,
                                             34 + l * 12 + qualityLevel * 6,
                                             static_cast<sf::Uint8>(130 + l * 18 + qualityLevel * 11)));
                layer.shapes.push_back(shape);
            }
            layers.push_back(layer);
        }

        int starCount = 8 + qualityLevel * 26;
        for (int i = 0; i < starCount; i++)
        {
            Star s;
            s.position = sf::Vector2f(static_cast<float>(rand() % WIDTH),
                                      static_cast<float>(rand() % static_cast<int>(HEIGHT * 0.72f)));
            s.radius = 0.7f + static_cast<float>(rand() % 120) / 100.0f + qualityLevel * 0.05f;
            s.twinkleSpeed = 0.8f + static_cast<float>(rand() % 220) / 100.0f;
            s.phase = static_cast<float>(rand() % 628) / 100.0f;
            s.baseAlpha = static_cast<sf::Uint8>(std::min(240, 48 + rand() % 125 + qualityLevel * 10));
            stars.push_back(s);
        }

        if (qualityLevel >= 2)
        {
            int fogCount = 2 + qualityLevel;
            for (int i = 0; i < fogCount; i++)
            {
                sf::RectangleShape band(sf::Vector2f(420.0f + rand() % 380, 34.0f + rand() % 46));
                float x = static_cast<float>(rand() % (WIDTH + 300) - 150);
                float y = HEIGHT * 0.16f + i * 88.0f + static_cast<float>(rand() % 55);
                band.setPosition(x, y);
                band.setFillColor(sf::Color(90, 130, 180, qualityLevel == 3 ? 32 : 24));
                fogBands.push_back(band);
            }
        }
    }

public:
    ParallaxBackground() : qualityLevel(1), ambienceTime(0.0f)
    {
        rebuild();
    }

    void setQuality(int newQualityLevel)
    {
        int clamped = clampInt(newQualityLevel, 0, 3);
        if (clamped == qualityLevel)
        {
            return;
        }
        qualityLevel = clamped;
        rebuild();
    }

    int detailUnits() const
    {
        return static_cast<int>(layers.size() * 18 + stars.size() + fogBands.size() * 14);
    }

    void update(float dt)
    {
        ambienceTime += dt * (0.65f + qualityLevel * 0.15f);

        for (std::size_t layerIndex = 0; layerIndex < layers.size(); layerIndex++)
        {
            Layer &layer = layers[layerIndex];
            for (auto &shape : layer.shapes)
            {
                shape.move(-layer.speed * dt, 0.0f);
                if (shape.getPosition().x + shape.getSize().x < -30.0f)
                {
                    float jitter = (static_cast<float>(rand() % 1000) / 999.0f - 0.5f) * layer.variance * 2.0f;
                    shape.setPosition(WIDTH + 60.0f + static_cast<float>(rand() % 260), layer.baseY + jitter);
                }
            }
        }

        for (std::size_t i = 0; i < fogBands.size(); i++)
        {
            sf::RectangleShape &band = fogBands[i];
            float drift = 10.0f + static_cast<float>(i) * 4.0f;
            band.move(-drift * dt, 0.0f);
            if (band.getPosition().x + band.getSize().x < -120.0f)
            {
                float y = HEIGHT * 0.16f + static_cast<float>(i) * 88.0f + static_cast<float>(rand() % 55);
                band.setPosition(WIDTH + 80.0f + static_cast<float>(rand() % 180), y);
            }

            float wave = 0.5f + 0.5f * std::sin(ambienceTime * (0.7f + static_cast<float>(i) * 0.16f) + static_cast<float>(i));
            float baseAlpha = qualityLevel >= 3 ? 26.0f : 18.0f;
            float range = qualityLevel >= 3 ? 24.0f : 14.0f;
            sf::Color c = band.getFillColor();
            c.a = static_cast<sf::Uint8>(baseAlpha + wave * range);
            band.setFillColor(c);
        }
    }

    void draw(sf::RenderWindow &window)
    {
        for (const Star &star : stars)
        {
            float twinkle = 0.55f + 0.45f * std::sin(ambienceTime * star.twinkleSpeed + star.phase);
            sf::Uint8 alpha = static_cast<sf::Uint8>(std::max(8.0f, std::min(255.0f, star.baseAlpha * twinkle)));
            sf::CircleShape glow(star.radius);
            glow.setOrigin(star.radius, star.radius);
            glow.setPosition(star.position);
            glow.setFillColor(sf::Color(170, 205, 255, alpha));
            window.draw(glow);
        }

        for (auto &layer : layers)
        {
            for (auto &shape : layer.shapes)
            {
                window.draw(shape);
            }
        }

        for (auto &band : fogBands)
        {
            window.draw(band);
        }
    }
};
///////////////////////////////////////////////////////////////////////////
static void drawHeartIcon(sf::RenderWindow &window, const sf::Vector2f &center, float size, const sf::Color &color)
{
    float r = size * 0.55f;

    sf::CircleShape left(r);
    left.setPosition(center.x - r * 1.1f, center.y - r * 0.9f);
    left.setFillColor(color);
    left.setOutlineThickness(1.5f);
    left.setOutlineColor(sf::Color::White);
    window.draw(left);

    sf::CircleShape right(r);
    right.setPosition(center.x + r * 0.1f, center.y - r * 0.9f);
    right.setFillColor(color);
    right.setOutlineThickness(1.5f);
    right.setOutlineColor(sf::Color::White);
    window.draw(right);

    sf::ConvexShape bottom;
    bottom.setPointCount(3);
    bottom.setPoint(0, sf::Vector2f(center.x - r * 1.4f, center.y));
    bottom.setPoint(1, sf::Vector2f(center.x + r * 1.4f, center.y));
    bottom.setPoint(2, sf::Vector2f(center.x, center.y + r * 1.8f));
    bottom.setFillColor(color);
    bottom.setOutlineThickness(1.5f);
    bottom.setOutlineColor(sf::Color::White);
    window.draw(bottom);
}
///////////////////////////////////////////////////////////////////////////
