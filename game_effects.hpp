#pragma once
#include "game_common.hpp"

class SoundGen
{
private:
    std::map<std::string, sf::SoundBuffer> buffers;
    std::map<std::string, sf::Sound> sounds;

    void makeTone(sf::SoundBuffer &buf, float freq, float dur)
    {
        const unsigned RATE = 44100;
        const unsigned COUNT = static_cast<unsigned>(RATE * dur);
        std::vector<sf::Int16> samples(COUNT);

        for (unsigned i = 0; i < COUNT; i++)
        {
            float t = static_cast<float>(i) / RATE;
            float val = 0.25f * sin(2 * PI * freq * t);
            float env = 1.0f - (static_cast<float>(i) / COUNT);
            samples[i] = static_cast<sf::Int16>(val * env * 30000);
        }
        buf.loadFromSamples(&samples[0], COUNT, 1, RATE);
    }

public:
    SoundGen()
    {
        sf::SoundBuffer jBuf;
        makeTone(jBuf, 440.0f, 0.12f);
        buffers["jump"] = jBuf;
        sounds["jump"].setBuffer(buffers["jump"]);

        sf::SoundBuffer cBuf;
        makeTone(cBuf, 880.0f, 0.15f);
        buffers["coin"] = cBuf;
        sounds["coin"].setBuffer(buffers["coin"]);

        sf::SoundBuffer dBuf;
        makeTone(dBuf, 300.0f, 0.08f);
        buffers["dash"] = dBuf;
        sounds["dash"].setBuffer(buffers["dash"]);

        sf::SoundBuffer hBuf;
        makeTone(hBuf, 150.0f, 0.18f);
        buffers["hit"] = hBuf;
        sounds["hit"].setBuffer(buffers["hit"]);

        sf::SoundBuffer pBuf;
        makeTone(pBuf, 660.0f, 0.25f);
        buffers["powerup"] = pBuf;
        sounds["powerup"].setBuffer(buffers["powerup"]);

        sf::SoundBuffer sBuf;
        makeTone(sBuf, 520.0f, 0.09f);
        buffers["slash"] = sBuf;
        sounds["slash"].setBuffer(buffers["slash"]);
        setMasterVolume(85.0f);
    }

    void play(const std::string &name)
    {
        if (sounds.count(name))
            sounds[name].play();
    }

    void setMasterVolume(float volume)
    {
        float clamped = std::max(0.0f, std::min(100.0f, volume));
        for (auto &kv : sounds)
        {
            kv.second.setVolume(clamped);
        }
    }
};

////////////////////////////////////////////////////////////////////////////
// Enhanced Particle System with various effects
class ParticleSystem
{
public:
    struct Particle
    {
        sf::Vector2f position;
        sf::Vector2f velocity;
        sf::Color color;
        float lifetime;
        float maxLifetime;
        float size;
        float rotation;
        float rotationSpeed;
        int type; // 0: circle, 1: square, 2: trail
    };

    std::vector<Particle> particles;

    void addExplosion(sf::Vector2f pos, sf::Color color, int count = 40)
    {
        int scaledCount = std::max(1, static_cast<int>(std::round(count * QUALITY_MULT)));
        for (int i = 0; i < scaledCount; i++)
        {
            Particle p;
            p.position = pos;
            float angle = (rand() % 360) * PI / 180.0f;
            float speed = 100 + rand() % 300;
            p.velocity = sf::Vector2f(cos(angle) * speed, sin(angle) * speed);
            p.color = color;
            p.lifetime = 0;
            p.maxLifetime = 0.5f + (rand() % 100) / 100.0f;
            p.size = 3 + rand() % 5;
            p.rotation = rand() % 360;
            p.rotationSpeed = -180 + rand() % 360;
            p.type = rand() % 3;
            particles.push_back(p);
        }
    }

    void addJumpDust(sf::Vector2f pos)
    {
        int dustCount = std::max(4, static_cast<int>(std::round(15.0f * QUALITY_MULT)));
        for (int i = 0; i < dustCount; i++)
        {
            Particle p;
            p.position = pos + sf::Vector2f(-10 + rand() % 20, 0);
            p.velocity = sf::Vector2f(-50 + rand() % 100, -100 - rand() % 100);
            p.color = sf::Color(100, 200, 255, 200);
            p.lifetime = 0;
            p.maxLifetime = 0.4f;
            p.size = 2 + rand() % 3;
            p.rotation = 0;
            p.rotationSpeed = 0;
            p.type = 0;
            particles.push_back(p);
        }
    }

    void addRunDust(sf::Vector2f pos, bool facingRight)
    {
        Particle p;
        p.position = pos;
        p.velocity = sf::Vector2f(facingRight ? -100 : 100, -50);
        p.color = sf::Color(150, 200, 255, 150);
        p.lifetime = 0;
        p.maxLifetime = 0.3f;
        p.size = 2;
        p.rotation = 0;
        p.rotationSpeed = 0;
        p.type = 0;
        particles.push_back(p);
    }

    void addCollectEffect(sf::Vector2f pos, sf::Color color)
    {
        int scaledCount = std::max(8, static_cast<int>(std::round(30.0f * QUALITY_MULT)));
        for (int i = 0; i < scaledCount; i++)
        {
            Particle p;
            p.position = pos;
            float angle = (rand() % 360) * PI / 180.0f;
            float speed = 150 + rand() % 150;
            p.velocity = sf::Vector2f(cos(angle) * speed, sin(angle) * speed);
            p.color = color;
            p.lifetime = 0;
            p.maxLifetime = 0.8f;
            p.size = 3 + rand() % 4;
            p.rotation = rand() % 360;
            p.rotationSpeed = -360 + rand() % 720;
            p.type = 1;
            particles.push_back(p);
        }
    }

    void update(float dt)
    {
        for (auto it = particles.begin(); it != particles.end();)
        {
            it->lifetime += dt;
            if (it->lifetime >= it->maxLifetime)
            {
                it = particles.erase(it);
            }
            else
            {
                it->position += it->velocity * dt;
                it->velocity.y += GRAVITY * 0.5f * dt;
                it->rotation += it->rotationSpeed * dt;
                ++it;
            }
        }
    }

    void draw(sf::RenderWindow &window)
    {
        for (auto &p : particles)
        {
            float alpha = 255 * (1 - p.lifetime / p.maxLifetime);
            sf::Color drawColor(p.color.r, p.color.g, p.color.b, (int)alpha);

            if (p.type == 0)
            {
                sf::CircleShape circle(p.size);
                circle.setPosition(p.position);
                circle.setFillColor(drawColor);
                window.draw(circle);
            }
            else if (p.type == 1)
            {
                sf::RectangleShape rect(sf::Vector2f(p.size * 2, p.size * 2));
                rect.setOrigin(p.size, p.size);
                rect.setPosition(p.position);
                rect.setRotation(p.rotation);
                rect.setFillColor(drawColor);
                window.draw(rect);
            }
            else if (p.type == 2)
            {
                sf::RectangleShape trail(sf::Vector2f(p.size * 3.0f, std::max(1.0f, p.size)));
                trail.setOrigin(trail.getSize() * 0.5f);
                trail.setPosition(p.position);
                trail.setRotation(std::atan2(p.velocity.y, p.velocity.x) * 180.0f / PI);
                trail.setFillColor(drawColor);
                window.draw(trail);
            }
        }
    }
};

////////////////////////////////////////////////////////////////////////////
// Platform class
