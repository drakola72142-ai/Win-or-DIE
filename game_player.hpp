#pragma once
#include "game_effects.hpp"
#include "game_platform.hpp"

class Player
{
public:
    static constexpr float COMBO_WINDOW = 3.6f;

    sf::Vector2f position;
    sf::Vector2f velocity;
    float size;
    sf::Color color;
    bool onGround;
    bool facingRight;
    int lives;
    int coins;
    float invincibleTimer;
    float animTimer;
    bool doubleJumpAvailable;
    float dashCooldown;
    bool isDashing;
    float dashTimer;
    int score;
    bool hasGun;
    float gunTimer;
    float gunCooldown;
    bool gunCharging;
    float gunCharge;
    bool slashing;
    float slashTimer;
    float slashCooldown;
    bool slashDown;
    float slashFxTimer;
    int comboCount;
    float comboTimer;
    float comboMultiplier;
    sf::Vector2f respawnPoint;
    float coyoteTimer;
    float jumpBufferTimer;
    bool wallSliding;
    int wallDirection;
    float soul;
    bool focusing;
    float focusTimer;
    int styleVariant;

    Player() : position(100, 300), velocity(0, 0), size(20),
               color(0, 255, 200), onGround(false), facingRight(true),
               lives(3), coins(0), invincibleTimer(0), animTimer(0),
               doubleJumpAvailable(true), dashCooldown(0), isDashing(false),
               dashTimer(0), score(0), hasGun(false), gunTimer(0), gunCooldown(0),
               gunCharging(false), gunCharge(0),
               slashing(false), slashTimer(0), slashCooldown(0), slashDown(false), slashFxTimer(0),
               comboCount(0), comboTimer(0), comboMultiplier(1.0f),
               respawnPoint(100, 300), coyoteTimer(0), jumpBufferTimer(0),
               wallSliding(false), wallDirection(0), soul(0.0f), focusing(false),
               focusTimer(0.0f), styleVariant(0) {}

    void update(float dt, std::vector<std::unique_ptr<Platform>> &platforms, ParticleSystem &particles)
    {
        invincibleTimer -= dt;
        dashCooldown = std::max(0.0f, dashCooldown - dt);
        animTimer += dt;
        gunCooldown = std::max(0.0f, gunCooldown - dt);
        slashCooldown = std::max(0.0f, slashCooldown - dt);
        slashFxTimer = std::max(0.0f, slashFxTimer - dt);
        coyoteTimer = std::max(0.0f, coyoteTimer - dt);
        jumpBufferTimer = std::max(0.0f, jumpBufferTimer - dt);

        bool focusHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::C);
        if (focusing)
        {
            if (!focusHeld || !onGround || soul < FOCUS_COST || lives >= MAX_LIVES)
            {
                focusing = false;
                focusTimer = 0.0f;
            }
            else
            {
                focusTimer += dt;
                velocity.x *= 0.78f;
                if (focusTimer >= FOCUS_TIME)
                {
                    soul = std::max(0.0f, soul - FOCUS_COST);
                    lives = std::min(MAX_LIVES, lives + 1);
                    invincibleTimer = std::max(invincibleTimer, 0.55f);
                    focusTimer = 0.0f;
                }
            }
        }
        else if (focusHeld && onGround && soul >= FOCUS_COST && lives < MAX_LIVES && !isDashing)
        {
            focusing = true;
            focusTimer = 0.0f;
            gunCharging = false;
        }

        if (slashing)
        {
            slashTimer -= dt;
            if (slashTimer <= 0)
            {
                slashing = false;
                slashDown = false;
            }
        }
        // Combo decay timer
        if (comboCount > 0)
        {
            comboTimer -= dt;
            if (comboTimer <= 0)
            {
                comboCount = 0;
                comboMultiplier = 1.0f;
            }
        }

        if (gunTimer > 0)
        {
            gunTimer -= dt;
            if (gunTimer <= 0)
            {
                gunTimer = 0;
                hasGun = false;
                gunCharging = false;
                gunCharge = 0;
            }
        }
        if (gunCharging)
        {
            gunCharge += dt;
            if (gunCharge > GUN_CHARGE_MAX)
                gunCharge = GUN_CHARGE_MAX;
        }
        if (focusing)
        {
            gunCharging = false;
            gunCharge = 0.0f;
        }

        if (isDashing)
        {
            dashTimer -= dt;
            if (dashTimer <= 0)
            {
                isDashing = false;
                velocity.x *= 0.5f;
            }
        }

        // Movement
        float speedBoost = 1.0f + std::min(0.25f, (comboMultiplier - 1.0f) * 0.08f);
        float moveSpeed = (isDashing ? 800 : 400) * speedBoost;

        if (!isDashing && !focusing)
        {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::A))
            {
                velocity.x = -moveSpeed;
                facingRight = false;
                if (onGround && (int)(animTimer * 10) % 3 == 0)
                {
                    particles.addRunDust(position + sf::Vector2f(0, size), facingRight);
                }
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) ||
                     sf::Keyboard::isKeyPressed(sf::Keyboard::D))
            {
                velocity.x = moveSpeed;
                facingRight = true;
                if (onGround && (int)(animTimer * 10) % 3 == 0)
                {
                    particles.addRunDust(position + sf::Vector2f(0, size), facingRight);
                }
            }
            else
            {
                velocity.x *= 0.85f;
            }
        }
        else if (focusing)
        {
            velocity.x *= 0.82f;
        }

        if (jumpBufferTimer > 0.0f && !focusing)
        {
            if (onGround || coyoteTimer > 0.0f)
            {
                velocity.y = -620.0f;
                onGround = false;
                coyoteTimer = 0.0f;
                jumpBufferTimer = 0.0f;
                wallSliding = false;
                wallDirection = 0;
            }
            else if (wallSliding && wallDirection != 0)
            {
                velocity.y = WALL_JUMP_H;
                velocity.x = -static_cast<float>(wallDirection) * WALL_JUMP_X;
                facingRight = velocity.x >= 0.0f;
                wallSliding = false;
                jumpBufferTimer = 0.0f;
                doubleJumpAvailable = true;
            }
            else if (doubleJumpAvailable && !onGround)
            {
                velocity.y = -560.0f;
                doubleJumpAvailable = false;
                jumpBufferTimer = 0.0f;
            }
        }

        // Gravity
        if (!isDashing)
        {
            float gravityScale = onGround ? 1.0f : AIR_GRAVITY_MULT;
            if (focusing)
                gravityScale = 0.55f;
            velocity.y += GRAVITY * gravityScale * dt;
            velocity.y = std::min(velocity.y, MAX_FALL_SPEED);
        }

        // Apply velocity
        position += velocity * dt;

        // Platform collision
        bool wasOnGround = onGround;
        bool touchedWall = false;
        int touchedWallDir = 0;
        onGround = false;
        for (auto &platform : platforms)
        {
            if (platform->broken)
                continue;

            sf::FloatRect playerBounds(position.x - size, position.y - size, size * 2, size * 2);

            if (playerBounds.intersects(platform->bounds))
            {
                // Top collision
                if (velocity.y > 0 && position.y - size < platform->bounds.top + 10)
                {
                    position.y = platform->bounds.top - size;
                    velocity.y = 0;
                    onGround = true;
                    doubleJumpAvailable = true;
                    coyoteTimer = COYOTE_TIME;

                    if (platform->type == 3)
                    { // Bounce platform
                        velocity.y = -700;
                        particles.addJumpDust(position + sf::Vector2f(0, size));
                    }
                    else if (platform->type == 2)
                    { // Breakable
                        platform->broken = true;
                        particles.addExplosion(sf::Vector2f(platform->bounds.left + platform->bounds.width / 2,
                                                            platform->bounds.top + platform->bounds.height / 2),
                                               platform->color, 20);
                    }
                    else if (platform->type == 4)
                    { // Fake/troll platform: starts collapse countdown
                        platform->fakeTriggered = true;
                    }
                }
                // Bottom collision
                else if (velocity.y < 0 && position.y + size > platform->bounds.top + platform->bounds.height - 10)
                {
                    position.y = platform->bounds.top + platform->bounds.height + size;
                    velocity.y = 0;
                }
                // Side collisions
                else if (playerBounds.left < platform->bounds.left + platform->bounds.width &&
                         playerBounds.left + playerBounds.width > platform->bounds.left)
                {
                    if (velocity.x > 0)
                    {
                        position.x = platform->bounds.left - size;
                        touchedWall = true;
                        touchedWallDir = 1;
                    }
                    else
                    {
                        position.x = platform->bounds.left + platform->bounds.width + size;
                        touchedWall = true;
                        touchedWallDir = -1;
                    }
                    velocity.x = 0;
                }
            }
        }

        if (wasOnGround && !onGround && velocity.y >= 0.0f)
        {
            coyoteTimer = COYOTE_TIME;
        }

        if (onGround)
        {
            wallSliding = false;
            wallDirection = 0;
        }
        else
        {
            wallSliding = touchedWall && velocity.y > 0.0f && !isDashing;
            wallDirection = touchedWall ? touchedWallDir : 0;
            if (wallSliding)
            {
                velocity.y = std::min(velocity.y, WALL_SLIDE_SPEED);
                doubleJumpAvailable = true;
            }
        }

        // Death check
        if (position.y > HEIGHT + 50)
        {
            hit();
            respawn();
        }

        // Keep in horizontal bounds
        if (position.x < size)
            position.x = size;
        if (position.x > WIDTH - size)
            position.x = WIDTH - size;
    }

    bool jump()
    {
        if (!focusing)
        {
            jumpBufferTimer = JUMP_BUFFER_TIME;
            return onGround || coyoteTimer > 0.0f || doubleJumpAvailable || wallSliding;
        }
        return false;
    }

    void dash()
    {
        if (!focusing && dashCooldown <= 0)
        {
            isDashing = true;
            dashTimer = 0.2f;
            dashCooldown = 1.0f;
            velocity.x = facingRight ? 1000 : -1000;
            velocity.y = 0;
        }
    }

    bool startSlash()
    {
        if (!focusing && slashCooldown <= 0)
        {
            slashing = true;
            slashTimer = SLASH_DURATION;
            slashCooldown = SLASH_COOLDOWN;
            slashFxTimer = SLASH_DURATION + 0.08f;
            slashDown = (!onGround && (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) ||
                                       sf::Keyboard::isKeyPressed(sf::Keyboard::S)));
            return true;
        }
        return false;
    }

    void hit()
    {
        if (invincibleTimer <= 0)
        {
            lives--;
            invincibleTimer = DIE_AGAIN_MODE ? 0.85f : 2.0f;
            comboCount = 0;
            comboMultiplier = 1.0f;
            comboTimer = 0.0f;
            focusing = false;
            focusTimer = 0.0f;
        }
    }

    void respawn()
    {
        position = respawnPoint;
        velocity = sf::Vector2f(0, 0);
        wallSliding = false;
        wallDirection = 0;
        coyoteTimer = 0.0f;
        jumpBufferTimer = 0.0f;
        focusing = false;
        focusTimer = 0.0f;
    }

    void draw(sf::RenderWindow &window)
    {
        // Invincibility effect
        if (invincibleTimer > 0 && (int)(invincibleTimer * 10) % 2 == 0)
        {
            return;
        }

        float bob = std::sin(animTimer * 8.0f) * 1.2f;
        sf::Vector2f base(position.x, position.y + bob * 0.35f);
        float dir = facingRight ? 1.0f : -1.0f;
        sf::Color shell(24, 30, 44);
        sf::Color cloth(48, 82, 138);
        sf::Color trim(108, 172, 245);
        sf::Color mask(242, 247, 255);
        sf::Color eyeCore(138, 222, 255);
        sf::Color boot(10, 14, 21);
        if (styleVariant == 1)
        {
            shell = sf::Color(18, 18, 28);
            cloth = sf::Color(78, 56, 118);
            trim = sf::Color(168, 122, 255);
            mask = sf::Color(240, 238, 255);
            eyeCore = sf::Color(220, 165, 255);
            boot = sf::Color(8, 8, 14);
        }
        else if (styleVariant == 2)
        {
            shell = sf::Color(31, 38, 45);
            cloth = sf::Color(120, 52, 68);
            trim = sf::Color(255, 150, 120);
            mask = sf::Color(250, 244, 236);
            eyeCore = sf::Color(255, 190, 140);
            boot = sf::Color(20, 12, 9);
        }

        // Dash trail
        if (isDashing)
        {
            sf::CircleShape trail(size * 1.15f);
            trail.setOrigin(trail.getRadius(), trail.getRadius());
            trail.setScale(1.8f, 0.9f);
            trail.setPosition(base.x - dir * size * 1.4f, base.y - size * 0.2f);
            trail.setFillColor(sf::Color(trim.r, trim.g, trim.b, 95));
            window.draw(trail);
        }

        // Aura on high combo
        if (comboMultiplier >= 3.0f)
        {
            sf::CircleShape aura(size * 1.8f);
            aura.setOrigin(aura.getRadius(), aura.getRadius());
            aura.setPosition(base);
            aura.setFillColor(sf::Color(145, 120, 255, 60));
            window.draw(aura);
        }
        if (focusing)
        {
            float pulse = 0.5f + 0.5f * std::sin(animTimer * 7.0f);
            sf::CircleShape focusAura(size * (1.2f + pulse * 0.5f));
            focusAura.setOrigin(focusAura.getRadius(), focusAura.getRadius());
            focusAura.setPosition(base);
            focusAura.setFillColor(sf::Color(170, 210, 255, 50));
            focusAura.setOutlineThickness(2.0f);
            focusAura.setOutlineColor(sf::Color(220, 240, 255, 180));
            window.draw(focusAura);
        }

        // Shadow under feet
        sf::CircleShape ground(size * 0.6f);
        ground.setOrigin(ground.getRadius(), ground.getRadius());
        ground.setScale(1.4f, 0.45f);
        ground.setPosition(base.x, base.y + size * 1.03f);
        ground.setFillColor(sf::Color(0, 0, 0, 80));
        window.draw(ground);

        // Mantle
        float sway = std::sin(animTimer * 6.0f) * size * 0.22f * dir;
        sf::ConvexShape mantle;
        mantle.setPointCount(5);
        mantle.setPoint(0, sf::Vector2f(base.x - size * 0.95f, base.y - size * 0.25f));
        mantle.setPoint(1, sf::Vector2f(base.x + size * 0.95f, base.y - size * 0.25f));
        mantle.setPoint(2, sf::Vector2f(base.x + size * 0.88f + sway, base.y + size * 1.65f));
        mantle.setPoint(3, sf::Vector2f(base.x + sway * 0.2f, base.y + size * 1.98f));
        mantle.setPoint(4, sf::Vector2f(base.x - size * 0.88f + sway, base.y + size * 1.65f));
        mantle.setFillColor(sf::Color(cloth.r, cloth.g, cloth.b, 225));
        mantle.setOutlineThickness(2.0f);
        mantle.setOutlineColor(trim);
        window.draw(mantle);

        // Torso
        sf::CircleShape torso(size * 0.64f);
        torso.setOrigin(torso.getRadius(), torso.getRadius());
        torso.setScale(1.0f, 1.42f);
        torso.setPosition(base.x, base.y + size * 0.16f);
        torso.setFillColor(shell);
        torso.setOutlineThickness(2.5f);
        torso.setOutlineColor(sf::Color(150, 185, 230));
        window.draw(torso);

        // Chest crest
        sf::ConvexShape crest;
        crest.setPointCount(4);
        crest.setPoint(0, sf::Vector2f(base.x, base.y - size * 0.18f));
        crest.setPoint(1, sf::Vector2f(base.x + size * 0.22f, base.y + size * 0.16f));
        crest.setPoint(2, sf::Vector2f(base.x, base.y + size * 0.44f));
        crest.setPoint(3, sf::Vector2f(base.x - size * 0.22f, base.y + size * 0.16f));
        crest.setFillColor(sf::Color(88, 140, 230));
        window.draw(crest);

        // Mask / head
        sf::CircleShape head(size * 0.83f);
        head.setOrigin(head.getRadius(), head.getRadius());
        head.setScale(1.0f, 1.16f);
        head.setPosition(base.x, base.y - size * 0.9f);
        head.setFillColor(mask);
        head.setOutlineThickness(2.0f);
        head.setOutlineColor(sf::Color(160, 182, 220));
        window.draw(head);

        // Crown blades
        sf::ConvexShape crown;
        crown.setPointCount(3);
        crown.setPoint(0, sf::Vector2f(-size * 0.2f, -size * 0.2f));
        crown.setPoint(1, sf::Vector2f(-size * 0.84f, -size * 1.2f));
        crown.setPoint(2, sf::Vector2f(-size * 0.04f, -size * 0.02f));
        crown.setPosition(base.x - size * 0.08f, base.y - size * 1.62f);
        crown.setFillColor(mask);
        crown.setOutlineThickness(2.0f);
        crown.setOutlineColor(sf::Color(145, 170, 215));
        window.draw(crown);

        crown.setScale(-1.0f, 1.0f);
        crown.setPosition(base.x + size * 0.08f, base.y - size * 1.62f);
        window.draw(crown);

        // Eyes
        sf::CircleShape eyeAura(size * 0.16f);
        eyeAura.setOrigin(eyeAura.getRadius(), eyeAura.getRadius());
        eyeAura.setFillColor(sf::Color(eyeCore.r, eyeCore.g, eyeCore.b, 140));
        eyeAura.setPosition(base.x - size * 0.3f, base.y - size * 0.98f);
        window.draw(eyeAura);
        eyeAura.setPosition(base.x + size * 0.3f, base.y - size * 0.98f);
        window.draw(eyeAura);

        sf::RectangleShape eye(sf::Vector2f(size * 0.14f, size * 0.22f));
        eye.setOrigin(eye.getSize() * 0.5f);
        eye.setFillColor(sf::Color(18, 30, 48));
        eye.setPosition(base.x - size * 0.3f, base.y - size * 0.98f);
        window.draw(eye);
        eye.setPosition(base.x + size * 0.3f, base.y - size * 0.98f);
        window.draw(eye);

        // Weapon variant
        float weaponAngle = dir > 0 ? -18.0f : 198.0f;
        sf::Vector2f handPos(base.x + dir * (size * 0.96f), base.y - size * 0.24f);
        sf::Color weaponCore(224, 233, 248);
        sf::Color weaponEdge(120, 145, 188);

        if (styleVariant == 1)
        {
            // Crescent scythe
            sf::RectangleShape handle(sf::Vector2f(size * 1.25f, size * 0.16f));
            handle.setOrigin(size * 0.1f, size * 0.08f);
            handle.setPosition(handPos);
            handle.setRotation(weaponAngle - 8.0f);
            handle.setFillColor(sf::Color(74, 60, 92));
            handle.setOutlineThickness(1.0f);
            handle.setOutlineColor(sf::Color(168, 122, 255));
            window.draw(handle);

            sf::ConvexShape crescent;
            crescent.setPointCount(6);
            crescent.setPoint(0, sf::Vector2f(0.0f, -size * 0.14f));
            crescent.setPoint(1, sf::Vector2f(size * 0.9f, -size * 0.48f));
            crescent.setPoint(2, sf::Vector2f(size * 1.55f, -size * 0.18f));
            crescent.setPoint(3, sf::Vector2f(size * 1.45f, size * 0.03f));
            crescent.setPoint(4, sf::Vector2f(size * 0.85f, -size * 0.15f));
            crescent.setPoint(5, sf::Vector2f(size * 0.08f, size * 0.11f));
            crescent.setFillColor(sf::Color(235, 225, 255));
            crescent.setOutlineThickness(1.0f);
            crescent.setOutlineColor(sf::Color(155, 110, 230));
            crescent.setPosition(handPos + sf::Vector2f(dir * (size * 0.35f), -size * 0.02f));
            crescent.setScale(dir > 0 ? 1.0f : -1.0f, 1.0f);
            crescent.setRotation(weaponAngle - 5.0f);
            window.draw(crescent);
        }
        else if (styleVariant == 2)
        {
            // Heavy cleaver
            sf::RectangleShape handle(sf::Vector2f(size * 0.9f, size * 0.18f));
            handle.setOrigin(size * 0.08f, size * 0.09f);
            handle.setPosition(handPos);
            handle.setRotation(weaponAngle);
            handle.setFillColor(sf::Color(92, 42, 34));
            window.draw(handle);

            sf::RectangleShape body(sf::Vector2f(size * 1.55f, size * 0.26f));
            body.setOrigin(size * 0.02f, size * 0.13f);
            body.setPosition(handPos + sf::Vector2f(dir * (size * 0.35f), -size * 0.02f));
            body.setRotation(weaponAngle);
            body.setFillColor(sf::Color(245, 226, 212));
            body.setOutlineThickness(1.0f);
            body.setOutlineColor(sf::Color(160, 104, 92));
            window.draw(body);

            sf::ConvexShape tip;
            tip.setPointCount(3);
            tip.setPoint(0, sf::Vector2f(0.0f, -size * 0.13f));
            tip.setPoint(1, sf::Vector2f(size * 0.5f, 0.0f));
            tip.setPoint(2, sf::Vector2f(0.0f, size * 0.13f));
            tip.setPosition(handPos + sf::Vector2f(dir * (size * 1.9f), -size * 0.03f));
            tip.setScale(dir > 0 ? 1.0f : -1.0f, 1.0f);
            tip.setRotation(weaponAngle);
            tip.setFillColor(sf::Color(255, 205, 168));
            tip.setOutlineThickness(1.0f);
            tip.setOutlineColor(sf::Color(160, 104, 92));
            window.draw(tip);
        }
        else
        {
            // Crystal rapier
            sf::RectangleShape grip(sf::Vector2f(size * 0.75f, size * 0.15f));
            grip.setOrigin(size * 0.1f, size * 0.08f);
            grip.setFillColor(sf::Color(72, 98, 132));
            grip.setPosition(handPos);
            grip.setRotation(weaponAngle);
            window.draw(grip);

            sf::ConvexShape guard;
            guard.setPointCount(4);
            guard.setPoint(0, sf::Vector2f(0.0f, -size * 0.15f));
            guard.setPoint(1, sf::Vector2f(size * 0.25f, 0.0f));
            guard.setPoint(2, sf::Vector2f(0.0f, size * 0.15f));
            guard.setPoint(3, sf::Vector2f(-size * 0.25f, 0.0f));
            guard.setFillColor(trim);
            guard.setPosition(handPos + sf::Vector2f(dir * (size * 0.28f), -size * 0.02f));
            guard.setRotation(weaponAngle);
            window.draw(guard);

            sf::RectangleShape blade(sf::Vector2f(size * 1.95f, size * 0.14f));
            blade.setOrigin(size * 0.02f, size * 0.07f);
            blade.setFillColor(weaponCore);
            blade.setOutlineThickness(1.0f);
            blade.setOutlineColor(weaponEdge);
            blade.setPosition(handPos + sf::Vector2f(dir * (size * 0.42f), -size * 0.03f));
            blade.setRotation(weaponAngle);
            window.draw(blade);

            sf::ConvexShape tip;
            tip.setPointCount(3);
            tip.setPoint(0, sf::Vector2f(0.0f, -size * 0.07f));
            tip.setPoint(1, sf::Vector2f(size * 0.4f, 0.0f));
            tip.setPoint(2, sf::Vector2f(0.0f, size * 0.07f));
            tip.setFillColor(weaponCore);
            tip.setOutlineThickness(1.0f);
            tip.setOutlineColor(weaponEdge);
            tip.setPosition(handPos + sf::Vector2f(dir * (size * 2.36f), -size * 0.03f));
            tip.setScale(dir > 0 ? 1.0f : -1.0f, 1.0f);
            tip.setRotation(weaponAngle);
            window.draw(tip);
        }

        // Slash visual for Z attack
        if (slashing || slashFxTimer > 0.0f)
        {
            float ratio = std::max(0.0f, std::min(1.0f, slashFxTimer / (SLASH_DURATION + 0.08f)));
            sf::Color fx = accentColor();
            fx.a = static_cast<sf::Uint8>(60 + ratio * 160.0f);

            if (slashDown)
            {
                for (int i = 0; i < 3; i++)
                {
                    sf::RectangleShape cut(sf::Vector2f(size * (0.45f + i * 0.16f), size * (1.8f + i * 0.3f)));
                    cut.setOrigin(cut.getSize() * 0.5f);
                    cut.setPosition(base.x, base.y + size * (0.72f + i * 0.2f));
                    cut.setRotation((i % 2 == 0) ? -8.0f : 8.0f);
                    cut.setFillColor(sf::Color(0, 0, 0, 0));
                    sf::Color c = fx;
                    c.a = static_cast<sf::Uint8>(fx.a - i * 30);
                    cut.setOutlineThickness(2.0f - i * 0.35f);
                    cut.setOutlineColor(c);
                    window.draw(cut);
                }
            }
            else
            {
                for (int i = 0; i < 3; i++)
                {
                    sf::CircleShape ring(size * (0.8f + i * 0.28f));
                    ring.setOrigin(ring.getRadius(), ring.getRadius());
                    ring.setPosition(base.x + dir * (size * (0.78f + i * 0.24f)), base.y - size * (0.2f + i * 0.05f));
                    ring.setScale(1.55f, 0.72f);
                    ring.setFillColor(sf::Color(0, 0, 0, 0));
                    sf::Color c = fx;
                    c.a = static_cast<sf::Uint8>(fx.a - i * 32);
                    ring.setOutlineThickness(2.4f - i * 0.5f);
                    ring.setOutlineColor(c);
                    window.draw(ring);
                }
            }
        }

        // Boots
        sf::RectangleShape bootShape(sf::Vector2f(size * 0.45f, size * 0.24f));
        bootShape.setOrigin(bootShape.getSize() * 0.5f);
        bootShape.setFillColor(boot);
        bootShape.setPosition(base.x - size * 0.32f, base.y + size * 1.03f);
        window.draw(bootShape);
        bootShape.setPosition(base.x + size * 0.32f, base.y + size * 1.03f);
        window.draw(bootShape);
    }

    sf::FloatRect getBounds()
    {
        return sf::FloatRect(position.x - size, position.y - size, size * 2, size * 2);
    }

    int addScore(int base)
    {
        comboCount++;
        comboTimer = COMBO_WINDOW;
        comboMultiplier = std::min(6.0f, 1.0f + comboCount * 0.30f);
        int gained = static_cast<int>(std::round(base * comboMultiplier));
        score += gained;
        return gained;
    }

    void addSoul(float value)
    {
        soul = std::min(SOUL_MAX, soul + value);
    }

    sf::Color accentColor() const
    {
        if (styleVariant == 1)
            return sf::Color(198, 138, 255);
        if (styleVariant == 2)
            return sf::Color(255, 162, 118);
        return sf::Color(120, 210, 255);
    }

    const char *comboRank() const
    {
        if (comboMultiplier >= 5.2f)
            return "MYTHIC";
        if (comboMultiplier >= 4.2f)
            return "DOMINATING";
        if (comboMultiplier >= 3.2f)
            return "RUTHLESS";
        if (comboMultiplier >= 2.2f)
            return "SHARP";
        return "FRESH";
    }

    sf::Color comboRankColor() const
    {
        if (comboMultiplier >= 5.2f)
            return sf::Color(255, 210, 120);
        if (comboMultiplier >= 4.2f)
            return sf::Color(255, 145, 90);
        if (comboMultiplier >= 3.2f)
            return sf::Color(255, 120, 200);
        if (comboMultiplier >= 2.2f)
            return sf::Color(120, 220, 255);
        return sf::Color(175, 210, 255);
    }

    void setStyle(int style)
    {
        styleVariant = clampInt(style, 0, 2);
    }

    bool isFocusing() const { return focusing; }
    bool isFever() const { return comboMultiplier >= 3.0f; }
};

