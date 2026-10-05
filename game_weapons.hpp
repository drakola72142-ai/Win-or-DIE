#pragma once
#include "game_common.hpp"

struct Bullet
{
    sf::Vector2f position;
    sf::Vector2f velocity;
    float lifetime;
    float size;
    int damage;
    int pierceLeft;
    float knockbackX;
    float knockbackY;
    sf::Color color;
};

struct EnemyFireball
{
    sf::Vector2f position;
    sf::Vector2f velocity;
    float lifetime;
    float size;
    int damage;
    sf::Color color;
    bool pierceWalls = false;
};

struct WeaponLoadout
{
    std::string name;
    std::string role;
    std::string perk;
    int tapPellets;
    float tapSpreadVel;
    int chargedPellets;
    float chargedSpreadVel;
    int tapDamage;
    int chargeBaseDamage;
    int chargeBonusDamage;
    int basePierce;
    int chargePierceBonus;
    float speedMul;
    float lifetimeMul;
    float sizeMul;
    float cooldown;
    float knockbackMul;
    sf::Color tapColor;
    sf::Color chargeColor;
};

struct Shade
{
    bool active = false;
    sf::Vector2f pos;
    int value = 0;
};

// Floating score/feedback text
struct Popup
{
    sf::Vector2f pos;
    sf::Vector2f vel;
    float life;
    float maxLife;
    std::string text;
    sf::Color color;
};
///////////////////////////////////////////////////////////////////////////////////////////
// Player class with advanced mechanics
