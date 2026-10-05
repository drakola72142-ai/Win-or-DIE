#pragma once
#include "game_player.hpp"

enum class EnemyState
{
    Idle,
    Patrol,
    Chase,
    Attack,
    Stagger
};

////////////////////////////////////////////////////////////////////////////////////
// Enemy class
class Enemy
{
public:
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color;
    float size;
    float startX;
    float patrolDistance;
    bool alive;
    int type; // 0: walker, 1: flyer, 2: jumper, 3: titan boss, 4: inferno behemoth
    float jumpTimer;

    // New fields for smarter AI
    EnemyState state = EnemyState::Patrol;
    int baseHealth = 3;
    int health = 3;
    bool enraged = false;
    float detectionRadius = 300.0f;
    float attackRange = 40.0f;
    float attackCooldown = 0.0f;
    float staggerTimer = 0.0f;
    float speed = 120.0f;
    float attackPrepTimer = 0.0f; // telegraph/prepare time
    float patrolDir = 1.0f;
    float flyTimer = 0.0f;
    float baseSpeed = 120.0f;
    float baseDetectionRadius = 300.0f;
    float baseAttackRange = 40.0f;
    float directorBias = 1.0f;
    bool chargingShot = false;
    bool pendingRangedShot = false;
    sf::Vector2f rangedShotOrigin;
    sf::Vector2f rangedShotVelocity;
    int rangedShotDamage = 0;

    Enemy(float x, float y, int t = 0)
        : position(x, y), size(25), startX(x), patrolDistance(150),
          alive(true), type(t), jumpTimer(0)
    {
        switch (type)
        {
        case 0: // Walker
            color = sf::Color(220, 60, 60);
            break;
        case 1: // Flyer
            color = sf::Color(255, 120, 40);
            break;
        case 2: // Jumper
            color = sf::Color(200, 60, 200);
            break;
        case 3: // Titan boss
            color = sf::Color(255, 80, 95);
            size = 56.0f;
            patrolDistance = 340.0f;
            speed = 165.0f;
            detectionRadius = 680.0f;
            attackRange = 92.0f;
            baseHealth = 56;
            break;
        case 4: // Inferno behemoth
            color = sf::Color(255, 120, 45);
            size = 68.0f;
            patrolDistance = 260.0f;
            speed = 128.0f;
            detectionRadius = 760.0f;
            attackRange = 440.0f;
            baseHealth = 88;
            break;
        }

        velocity = sf::Vector2f(speed * 0.5f, 0.0f);
        baseSpeed = speed;
        baseDetectionRadius = detectionRadius;
        baseAttackRange = attackRange;
        health = baseHealth;
    }

    // call when damaged
    void takeDamage(int dmg, const sf::Vector2f &knockback)
    {
        sf::Vector2f appliedKnockback = knockback;
        if (type == 3 || type == 4)
        {
            appliedKnockback *= (type == 4 ? 0.24f : 0.32f);
        }
        health -= dmg;
        position += appliedKnockback;
        velocity = appliedKnockback;
        state = EnemyState::Stagger;
        staggerTimer = (type == 3 || type == 4) ? 0.12f : 0.25f;
        if (health <= 0)
            alive = false;
    }

    void setDirectorBias(float v)
    {
        directorBias = std::max(0.7f, std::min(1.7f, v));
    }

    bool consumeRangedShot(sf::Vector2f &origin, sf::Vector2f &velocityOut, int &damageOut)
    {
        if (!pendingRangedShot)
            return false;
        pendingRangedShot = false;
        origin = rangedShotOrigin;
        velocityOut = rangedShotVelocity;
        damageOut = rangedShotDamage;
        return true;
    }

    // Updated signature: we now accept player info too
    void update(float dt, const Player &player, std::vector<std::unique_ptr<Platform>> &platforms)
    {
        if (!alive)
            return;
        pendingRangedShot = false;
        bool prepJustFinished = false;

        // timers
        if (attackCooldown > 0.0f)
            attackCooldown -= dt;
        if (staggerTimer > 0.0f)
        {
            staggerTimer -= dt;
            if (staggerTimer <= 0.0f)
                state = EnemyState::Patrol;
        }
        if (attackPrepTimer > 0.0f)
        {
            attackPrepTimer -= dt;
            if (attackPrepTimer <= 0.0f)
            {
                attackPrepTimer = 0.0f;
                prepJustFinished = true;
            }
        }

        // distance to player
        float dx = player.position.x - position.x;
        float dy = player.position.y - position.y;
        float dist = std::hypot(dx, dy);

        // Simple line-of-sight check (avoid chasing if player is far vertically)
        float visionY = 140.0f;
        if (type == 3)
            visionY = 260.0f;
        else if (type == 4)
            visionY = 320.0f;
        bool canSee = (std::abs(dy) < visionY);

        // Enrage when player has gun
        bool isBoss = (type == 3 || type == 4);
        float power = isBoss ? 1.0f : (player.hasGun ? 1.35f : 1.0f);
        float biasFactor = isBoss ? (0.88f + (directorBias - 1.0f) * 0.55f) : directorBias;
        speed = baseSpeed * power * biasFactor;
        detectionRadius = baseDetectionRadius * power * (0.92f + 0.32f * biasFactor);
        attackRange = baseAttackRange * power * (0.94f + 0.24f * biasFactor);
        if (!isBoss && player.hasGun && !enraged)
        {
            health += 2;
            enraged = true;
        }

        // State transitions (unless staggered)
        if (state != EnemyState::Stagger)
        {
            if (dist < attackRange)
            {
                state = EnemyState::Attack;
            }
            else if (dist < detectionRadius && canSee)
            {
                state = EnemyState::Chase;
            }
            else
            {
                state = EnemyState::Patrol;
            }
        }

        // Movement goals
        float desiredX = 0.0f;
        float accel = 8.0f;
        float maxSpeed = speed;

        // Behaviours
        if (state == EnemyState::Patrol)
        {
            if (position.x < startX - patrolDistance)
                patrolDir = 1.0f;
            else if (position.x > startX + patrolDistance)
                patrolDir = -1.0f;
            desiredX = patrolDir * speed * 0.6f;
            maxSpeed = speed * 0.7f;
        }
        else if (state == EnemyState::Chase)
        {
            float dir = (player.position.x > position.x) ? 1.0f : -1.0f;
            desiredX = dir * speed;
            maxSpeed = speed * 1.1f;
        }
        else if (state == EnemyState::Attack)
        {
            if (type == 4)
            {
                float dir = (player.position.x > position.x) ? 1.0f : -1.0f;
                if (std::abs(dx) < 260.0f)
                    desiredX = -dir * speed * 0.92f;
                else if (std::abs(dx) > 460.0f)
                    desiredX = dir * speed * 0.62f;
                else
                    desiredX = dir * speed * 0.16f;
                maxSpeed = speed * 0.95f;

                if (!chargingShot && attackCooldown <= 0.0f && canSee)
                {
                    chargingShot = true;
                    attackPrepTimer = HARDCORE_MODE ? 0.32f : (DIE_AGAIN_MODE ? 0.38f : 0.46f);
                }

                if (chargingShot && attackPrepTimer <= 0.0f)
                {
                    chargingShot = false;
                    attackPrepTimer = 0.0f;
                    attackCooldown = HARDCORE_MODE ? 0.88f : (DIE_AGAIN_MODE ? 1.05f : 1.25f);

                    sf::Vector2f target = player.position + player.velocity * 0.20f;
                    sf::Vector2f dirVec = target - position;
                    float len = std::hypot(dirVec.x, dirVec.y);
                    if (len < 0.001f)
                    {
                        dirVec = sf::Vector2f((player.position.x >= position.x) ? 1.0f : -1.0f, 0.0f);
                        len = 1.0f;
                    }
                    dirVec /= len;

                    float shotSpeed = (HARDCORE_MODE ? 560.0f : (DIE_AGAIN_MODE ? 500.0f : 450.0f)) * (0.90f + directorBias * 0.22f);
                    rangedShotOrigin = position + sf::Vector2f(dirVec.x * size * 0.9f, -size * 0.15f);
                    rangedShotVelocity = dirVec * shotSpeed;
                    rangedShotDamage = DIE_AGAIN_MODE ? 4 : (HARDCORE_MODE ? 4 : 3);
                    pendingRangedShot = true;
                }
            }
            else if (type == 3)
            {
                if (attackCooldown <= 0.0f)
                {
                    float dir = (player.position.x > position.x) ? 1.0f : -1.0f;
                    velocity.x = dir * speed * 3.9f;
                    velocity.y = -260.0f;
                    attackCooldown = 0.95f;
                }
                desiredX = (player.position.x > position.x ? 1.0f : -1.0f) * speed * 0.55f;
                maxSpeed = speed * 2.3f;
            }
            else
            {
                chargingShot = false;
                if (prepJustFinished)
                {
                    attackCooldown = 1.0f;
                }
                else if (attackPrepTimer <= 0.0f)
                {
                    if (attackCooldown <= 0.0f)
                    {
                        attackPrepTimer = 0.25f;
                    }
                }
                desiredX = 0.0f;
                maxSpeed = speed * 0.3f;
            }
        }
        else
        {
            chargingShot = false;
        }

        // Smooth horizontal movement
        velocity.x += (desiredX - velocity.x) * accel * dt;
        if (velocity.x > maxSpeed)
            velocity.x = maxSpeed;
        if (velocity.x < -maxSpeed)
            velocity.x = -maxSpeed;

        bool onGround = false;

        if (type == 0 || type == 2 || type == 3 || type == 4)
        {
            velocity.y += GRAVITY * dt;
        }
        else if (type == 1)
        {
            // flyer smooth bobbing
            flyTimer += dt;
            float desiredY = std::sin(flyTimer * 2.5f) * 80.0f;
            velocity.y += (desiredY - velocity.y) * (accel * 0.5f) * dt;
        }

        // Predict next position
        sf::Vector2f nextPos = position + velocity * dt;

        // Ground collision for walkers/jumpers
        if (type == 0 || type == 2 || type == 3 || type == 4)
        {
            for (auto &platform : platforms)
            {
                if (platform->broken)
                    continue;

                if (velocity.y > 0)
                {
                    float previousBottom = position.y + size;
                    float topThreshold = platform->bounds.top + std::max(6.0f, size * 0.30f);
                    sf::FloatRect enemyBounds(nextPos.x - size, nextPos.y - size, size * 2, size * 2);
                    if (enemyBounds.intersects(platform->bounds) && previousBottom <= topThreshold)
                    {
                        nextPos.y = platform->bounds.top - size;
                        velocity.y = 0.0f;
                        onGround = true;
                        break;
                    }
                }
            }
        }

        // Jumper logic (more believable timing)
        if (type == 2)
        {
            jumpTimer += dt;
            if (onGround)
            {
                bool playerAbove = (player.position.y + 20.0f < position.y);
                if (jumpTimer > 1.1f || (state == EnemyState::Chase && playerAbove && jumpTimer > 0.6f))
                {
                    velocity.y = -520.0f;
                    jumpTimer = 0.0f;
                }
            }
        }
        else if (type == 3)
        {
            jumpTimer += dt;
            if (onGround && state == EnemyState::Chase && jumpTimer > 1.15f)
            {
                velocity.y = -460.0f;
                jumpTimer = 0.0f;
            }
        }

        // Apply position
        position = nextPos;

        // Keep flyers inside screen with gentle bounce
        if (type == 1)
        {
            if (position.x - size < 0)
            {
                position.x = size;
                velocity.x *= -0.6f;
            }
            if (position.x + size > WIDTH)
            {
                position.x = WIDTH - size;
                velocity.x *= -0.6f;
            }
            if (position.y - size < 0)
            {
                position.y = size;
                velocity.y *= -0.4f;
            }
            if (position.y + size > HEIGHT)
            {
                position.y = HEIGHT - size;
                velocity.y *= -0.4f;
            }
        }
        else
        {
            if (position.x < size)
            {
                position.x = size;
                velocity.x *= -0.3f;
                patrolDir = 1.0f;
            }
            if (position.x > WIDTH - size)
            {
                position.x = WIDTH - size;
                velocity.x *= -0.3f;
                patrolDir = -1.0f;
            }
        }

        // death floor
        if (position.y > HEIGHT + 100)
            alive = false;
    }

    void draw(sf::RenderWindow &window)
    {
        if (!alive)
            return;

        sf::Color bodyColor(35, 35, 45);
        sf::Color outline(220, 220, 220);
        sf::Color eyeColor(255, 60, 60);

        if (type == 4)
        {
            float t = static_cast<float>(health) / std::max(1, baseHealth);
            sf::Color ember(255, 120, 40);
            sf::Color coreColor(255, 235, 160);

            sf::CircleShape aura(size * 2.95f);
            aura.setOrigin(aura.getRadius(), aura.getRadius());
            aura.setPosition(position);
            aura.setFillColor(sf::Color(255, 95, 40, 72));
            window.draw(aura);

            sf::CircleShape outer(size * 1.04f);
            outer.setOrigin(outer.getRadius(), outer.getRadius());
            outer.setPosition(position);
            outer.setFillColor(sf::Color(35, 20, 16));
            outer.setOutlineThickness(4.0f);
            outer.setOutlineColor(sf::Color(255, 170, 95));
            window.draw(outer);

            sf::CircleShape molten(size * 0.74f);
            molten.setOrigin(molten.getRadius(), molten.getRadius());
            molten.setPosition(position.x, position.y + size * 0.05f);
            molten.setFillColor(sf::Color(105, 40, 18));
            molten.setOutlineThickness(2.0f);
            molten.setOutlineColor(sf::Color(255, 120, 40));
            window.draw(molten);

            sf::RectangleShape cannon(sf::Vector2f(size * 0.95f, size * 0.26f));
            cannon.setOrigin(cannon.getSize().x * 0.5f, cannon.getSize().y * 0.5f);
            cannon.setPosition(position.x + size * 0.65f, position.y - size * 0.12f);
            cannon.setFillColor(sf::Color(55, 35, 28));
            cannon.setOutlineThickness(2.0f);
            cannon.setOutlineColor(sf::Color(255, 165, 100));
            window.draw(cannon);

            sf::CircleShape nozzle(size * 0.18f);
            nozzle.setOrigin(nozzle.getRadius(), nozzle.getRadius());
            nozzle.setPosition(position.x + size * 1.08f, position.y - size * 0.12f);
            nozzle.setFillColor(sf::Color(255, 130, 55));
            window.draw(nozzle);

            sf::CircleShape eye(size * 0.14f);
            eye.setOrigin(eye.getRadius(), eye.getRadius());
            eye.setPosition(position.x - size * 0.28f, position.y - size * 0.28f);
            eye.setFillColor(ember);
            window.draw(eye);

            sf::RectangleShape coreBar(sf::Vector2f(size * 1.0f, size * 0.16f));
            coreBar.setOrigin(coreBar.getSize().x * 0.5f, coreBar.getSize().y * 0.5f);
            coreBar.setPosition(position.x, position.y + size * 0.50f);
            coreBar.setFillColor(sf::Color(90, 34, 18));
            coreBar.setOutlineThickness(1.5f);
            coreBar.setOutlineColor(sf::Color(255, 145, 80));
            window.draw(coreBar);

            sf::RectangleShape coreHp(sf::Vector2f(size * 1.0f * t, size * 0.16f));
            coreHp.setPosition(position.x - size * 0.5f, position.y + size * 0.42f);
            coreHp.setFillColor(coreColor);
            window.draw(coreHp);
            return;
        }

        if (type == 3)
        {
            float t = static_cast<float>(health) / std::max(1, baseHealth);

            sf::CircleShape aura(size * 2.8f);
            aura.setOrigin(aura.getRadius(), aura.getRadius());
            aura.setPosition(position);
            aura.setFillColor(sf::Color(255, 80, 105, 65));
            window.draw(aura);

            sf::CircleShape body(size);
            body.setOrigin(body.getRadius(), body.getRadius());
            body.setPosition(position);
            body.setFillColor(sf::Color(30, 20, 30));
            body.setOutlineThickness(4.0f);
            body.setOutlineColor(sf::Color(255, 180, 190));
            window.draw(body);

            for (int i = 0; i < 6; i++)
            {
                float ang = i * (PI * 2.0f / 6.0f);
                sf::ConvexShape spike;
                spike.setPointCount(3);
                sf::Vector2f d(std::cos(ang), std::sin(ang));
                sf::Vector2f r(-d.y, d.x);
                spike.setPoint(0, position + d * (size * 1.02f) + r * 9.0f);
                spike.setPoint(1, position + d * (size * 1.02f) - r * 9.0f);
                spike.setPoint(2, position + d * (size * 1.58f));
                spike.setFillColor(sf::Color(255, 90, 120));
                spike.setOutlineThickness(1.2f);
                spike.setOutlineColor(sf::Color(255, 220, 225));
                window.draw(spike);
            }

            sf::RectangleShape eyeBar(sf::Vector2f(size * 0.95f, size * 0.2f));
            eyeBar.setOrigin(eyeBar.getSize() * 0.5f);
            eyeBar.setPosition(position.x, position.y - size * 0.15f);
            eyeBar.setFillColor(sf::Color(255, 165, 175));
            window.draw(eyeBar);

            sf::RectangleShape core(sf::Vector2f(size * 0.5f * t, size * 0.13f));
            core.setOrigin(core.getSize().x * 0.5f, core.getSize().y * 0.5f);
            core.setPosition(position.x, position.y - size * 0.15f);
            core.setFillColor(sf::Color(255, 235, 245));
            window.draw(core);
            return;
        }

        // Intense glow
        sf::CircleShape glowOuter(size * 2.0f);
        glowOuter.setPosition(position - sf::Vector2f(size * 2.0f, size * 2.0f));
        glowOuter.setFillColor(sf::Color(color.r, color.g, color.b, 40));
        window.draw(glowOuter);

        sf::CircleShape glowInner(size * 1.2f);
        glowInner.setPosition(position - sf::Vector2f(size * 1.2f, size * 1.2f));
        glowInner.setFillColor(sf::Color(color.r, color.g, color.b, 90));
        window.draw(glowInner);

        // Main body
        sf::CircleShape body(size * 0.95f);
        body.setPosition(position - sf::Vector2f(size * 0.95f, size * 0.95f));
        body.setFillColor(bodyColor);
        body.setOutlineThickness(2);
        body.setOutlineColor(outline);
        window.draw(body);

        // Spikes around the top/back
        for (int i = -1; i <= 1; i++)
        {
            sf::ConvexShape spike;
            spike.setPointCount(3);
            spike.setPoint(0, sf::Vector2f(position.x + i * size * 0.5f - 6, position.y - size * 0.8f));
            spike.setPoint(1, sf::Vector2f(position.x + i * size * 0.5f + 6, position.y - size * 0.8f));
            spike.setPoint(2, sf::Vector2f(position.x + i * size * 0.5f, position.y - size * 1.35f));
            spike.setFillColor(color);
            spike.setOutlineThickness(1);
            spike.setOutlineColor(outline);
            window.draw(spike);
        }

        // Horns
        sf::ConvexShape hornL;
        hornL.setPointCount(3);
        hornL.setPoint(0, sf::Vector2f(position.x - size * 0.6f, position.y - size * 0.7f));
        hornL.setPoint(1, sf::Vector2f(position.x - size * 0.2f, position.y - size * 0.8f));
        hornL.setPoint(2, sf::Vector2f(position.x - size * 0.55f, position.y - size * 1.3f));
        hornL.setFillColor(sf::Color(60, 60, 70));
        hornL.setOutlineThickness(1);
        hornL.setOutlineColor(outline);
        window.draw(hornL);

        sf::ConvexShape hornR;
        hornR.setPointCount(3);
        hornR.setPoint(0, sf::Vector2f(position.x + size * 0.2f, position.y - size * 0.8f));
        hornR.setPoint(1, sf::Vector2f(position.x + size * 0.6f, position.y - size * 0.7f));
        hornR.setPoint(2, sf::Vector2f(position.x + size * 0.55f, position.y - size * 1.3f));
        hornR.setFillColor(sf::Color(60, 60, 70));
        hornR.setOutlineThickness(1);
        hornR.setOutlineColor(outline);
        window.draw(hornR);

        // Eyes (red glow)
        sf::CircleShape eyeGlow(size * 0.18f);
        eyeGlow.setFillColor(sf::Color(eyeColor.r, eyeColor.g, eyeColor.b, 160));
        eyeGlow.setPosition(position.x - size * 0.45f, position.y - size * 0.15f);
        window.draw(eyeGlow);
        eyeGlow.setPosition(position.x + size * 0.1f, position.y - size * 0.15f);
        window.draw(eyeGlow);

        sf::CircleShape eye(size * 0.11f);
        eye.setFillColor(eyeColor);
        eye.setPosition(position.x - size * 0.41f, position.y - size * 0.11f);
        window.draw(eye);
        eye.setPosition(position.x + size * 0.14f, position.y - size * 0.11f);
        window.draw(eye);

        // Mouth
        sf::RectangleShape mouth(sf::Vector2f(size * 0.8f, size * 0.18f));
        mouth.setPosition(position.x - size * 0.4f, position.y + size * 0.25f);
        mouth.setFillColor(sf::Color(120, 20, 20));
        mouth.setOutlineThickness(1);
        mouth.setOutlineColor(outline);
        window.draw(mouth);

        // Teeth
        for (int i = 0; i < 3; i++)
        {
            sf::ConvexShape tooth;
            tooth.setPointCount(3);
            float tx = position.x - size * 0.3f + i * size * 0.25f;
            tooth.setPoint(0, sf::Vector2f(tx, position.y + size * 0.25f));
            tooth.setPoint(1, sf::Vector2f(tx + 8, position.y + size * 0.25f));
            tooth.setPoint(2, sf::Vector2f(tx + 4, position.y + size * 0.4f));
            tooth.setFillColor(sf::Color::White);
            window.draw(tooth);
        }

        // Wings for flyer
        if (type == 1)
        {
            sf::ConvexShape wing;
            wing.setPointCount(3);
            wing.setPoint(0, sf::Vector2f(position.x - size * 0.9f, position.y - size * 0.1f));
            wing.setPoint(1, sf::Vector2f(position.x - size * 1.6f, position.y - size * 0.4f));
            wing.setPoint(2, sf::Vector2f(position.x - size * 1.1f, position.y + size * 0.2f));
            wing.setFillColor(sf::Color(50, 50, 60));
            wing.setOutlineThickness(1);
            wing.setOutlineColor(outline);
            window.draw(wing);

            wing.setPoint(0, sf::Vector2f(position.x + size * 0.9f, position.y - size * 0.1f));
            wing.setPoint(1, sf::Vector2f(position.x + size * 1.6f, position.y - size * 0.4f));
            wing.setPoint(2, sf::Vector2f(position.x + size * 1.1f, position.y + size * 0.2f));
            window.draw(wing);
        }
    }

    sf::FloatRect getBounds()
    {
        return sf::FloatRect(position.x - size, position.y - size, size * 2, size * 2);
    }
};

//////////////////////////////////////////////////////////////////////////////////////////
// Spike trap inspired by troll platformers
