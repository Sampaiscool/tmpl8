#include "precomp.h"
#include "player.h"
#include "managers/renderManager.h"
#include "collision.h"
#include "assets.h"

// template.cpp keeps a key state array up to date but never declares it in a
// header, so we do it here rather than keeping a second copy.
extern bool IsKeyDown( const uint key );

#define MARCO ASSETS "assets/Marco/"

namespace Tmpl8
{
    namespace
    {
        constexpr bool infiniteHealth = true;

        constexpr int maxHealth = 3;
        constexpr float invulnerableTime = 1.0f;
        constexpr float flashInterval = 0.07f; // seconds per blink

        constexpr int magazineSize = 5;

        constexpr float reloadTime = 0.95f;

        // Seconds between shots while held.
        constexpr float fireInterval = 0.12f;

        constexpr float walkSpeed = 180.0f;
        constexpr float runSpeed = 300.0f;
        constexpr float crouchSpeed = 80.0f;

        constexpr float gravity = 900.0f;
        constexpr float jumpSpeed = 320.0f;
        constexpr float maxFallSpeed = 600.0f;

        // A normal body frame.
        constexpr float bodyFrameWidth = 34.0f;
        constexpr float spriteBaseHeight = 34.0f;

        // The hitbox, from the top left of the sprite.
        constexpr float hitboxOffsetX = 10.0f;
        constexpr float hitboxOffsetY = 4.0f;
        constexpr float hitboxWidth = 14.0f;
        constexpr float hitboxHeight = 30.0f;

        // The reload sheet's figure sits 3 left and 6 high of the other torsos.
        // I had to do this because I the spites keep getting messed up
        constexpr float reloadNudgeX = 3.0f;
        constexpr float reloadNudgeY = 6.0f;

        // this is where the barrel actualy sits for the bullets to come out of
        constexpr float shootMuzzleX = 37.0f;
        constexpr float shootMuzzleY = 49.0f;  // shootTorsoMarco.png, 51x64
        constexpr float crouchMuzzleY = 15.0f; // crouchShootMarco.png, 51x34
    }

    // Load clips for legs and torso.
    // The first 2 arguments are for the path.
    // The last 2 are: how many frames the clip has + the speed of the animation
    Player::Player()
    {
        health = maxHealth;
        ammo = magazineSize;

        // Idle pose: subtle breathing / standing stance.
        idle = { legs.AddClip ( MARCO "idleLegsMarco.png",  1, 0.10f ),
                 torso.AddClip( MARCO "idleTorsoMarco.png", 4, 0.15f ), 2.0f, -11.0f };

        // Normal walk cycle.
        walk = { legs.AddClip ( MARCO "runLegsMarco.png",  6, 0.10f ),
                 torso.AddClip( MARCO "runTorsoMarco.png", 6, 0.10f ), 5.0f, -11.0f };

        // Sprinting cycle when shift is held down.
        run  = { legs.AddClip ( MARCO "runFastLegsMarco.png",  6, 0.07f ),
                 torso.AddClip( MARCO "runFastTorsoMarco.png", 6, 0.07f ), 5.0f, -11.0f };

        // Jump plays once; fall is extracted from its last 2 frames and loops.
        jump = { legs.AddClip ( MARCO "jumpLegsMarco.png",  6, 0.10f, false ),
                 torso.AddClip( MARCO "jumpTorsoMarco.png", 6, 0.10f, false ), 5.0f, -17.0f };

        fall = { legs.AddSubClip ( jump.legs,  4, 2, 0.12f, true ),
                 torso.AddSubClip( jump.torso, 4, 2, 0.12f, true ), 5.0f, -17.0f };

        // Crouching is one combined body sheet (torso layer is set to -1).
        crouch     = { legs.AddClip( MARCO "crouch2Marco.png",    4, 0.12f ), -1 };
        crouchWalk = { legs.AddClip( MARCO "crouchWalkMarco.png", 7, 0.10f ), -1 };

        // Crouch shooting: the first 4 frames fire the shot, remainder lowers the gun back.
        const int crouchShootSheet = legs.AddClip( MARCO "crouchShootMarco.png", 10, 0.06f );
        crouchShoot    = { legs.AddSubClip( crouchShootSheet, 0, 4, 0.06f, true  ), -1 };
        crouchShootEnd = { legs.AddSubClip( crouchShootSheet, 4, 6, 0.08f, false ), -1 };

        // Upper body shooting animations while standing/moving.
        const int shootSheet = torso.AddClip( MARCO "shootTorsoMarco.png", 10, 0.06f );
        shootTorso   = torso.AddSubClip( shootSheet, 0, 4, 0.06f, true  );
        shootRelease = torso.AddSubClip( shootSheet, 4, 6, 0.08f, false );

        // Torso reload sequence.
        reloadTorso = torso.AddClip( MARCO "reloadTorsoMarco.png", 19, 0.05f, false );
    }

    int Player::GetMaxHealth() const { return maxHealth; }

    void Player::Unload()
    {
        legs.Unload();
        torso.Unload();
    }

    // One frame: read input, handle horizontal/vertical movement and physics,
    // evaluate player state to select the correct animation pose.
    void Player::Update(float deltaTime, const List<Collider>& colliders)
    {
        // Rebuilt from keys every frame so releasing stops movement instantly.
        // Vertical movement is governed entirely by gravity and jump velocity.
        float inputX = 0.0f;
        if (IsKeyDown( GLFW_KEY_A )) inputX -= 1.0f;
        if (IsKeyDown( GLFW_KEY_D )) inputX += 1.0f;

        const bool runDown = IsKeyDown( GLFW_KEY_LEFT_SHIFT ) || IsKeyDown( GLFW_KEY_RIGHT_SHIFT );
        const bool crouching = onGround && IsKeyDown( GLFW_KEY_S );

        const float moveSpeed = crouching ? crouchSpeed : (runDown ? runSpeed : walkSpeed);

        // Retain current facing direction when input drops back to zero.
        if (inputX < 0.0f) facingRight = false;
        else if (inputX > 0.0f) facingRight = true;

        // Initiate jump by applying negative vertical velocity once on ground press.
        const bool jumpDown = IsKeyDown( GLFW_KEY_SPACE ) || IsKeyDown( GLFW_KEY_W );
        if (jumpDown && !jumpHeld && onGround) velocityY = -jumpSpeed;
        jumpHeld = jumpDown;

        // Accumulate gravity each frame. Clamped to prevent tunneling through floors.
        velocityY = fminf( velocityY + gravity * deltaTime, maxFallSpeed );

        MoveAndCollide(inputX * moveSpeed * deltaTime, velocityY * deltaTime, colliders);

        /*
        Pick the pose hierarchy:
        1. Airborne state overrides all ground movement. Switching from jump to fall
           happens automatically when velocityY turns positive.
        2. Grounded states check for crouch vs standing horizontal movement.
        */
        Pose want;
        if (!onGround)          want = (velocityY < 0.0f) ? jump : fall;
        else if (crouching)     want = (inputX != 0.0f) ? crouchWalk : crouch;
        else if (inputX != 0.0f)want = runDown ? run : walk;
        else                    want = idle;

        UpdateWeapon(deltaTime, crouching, want);

        if (invulnerableTimer > 0.0f) invulnerableTimer -= deltaTime;

        // Play and advance leg animation frames.
        legs.Play(legsPlay, want.legs);
        legs.Update(legsPlay, deltaTime);

        // Advance torso layer if current pose features a separate upper body.
        if (want.torso >= 0)
        {
            torso.Play(torsoPlay, want.torso);
            torso.Update(torsoPlay, deltaTime);
        }

        // Wind-down animations are single-shot. Query the active layer responsible.
        if (releasing && (crouching ? legs.IsFinished(legsPlay) : torso.IsFinished(torsoPlay)))
        {
            releasing = false;
        }

        current = want; // Save chosen pose so Draw() can render it.
    }

    // Handles firing inputs, weapon cooldown timers, ammo consumption, and reload state.
    void Player::UpdateWeapon(float deltaTime, bool crouching, Pose& want)
    {
        // Block firing when out of ammo or reloading.
        const bool fireDown = IsKeyDown( GLFW_KEY_F ) && !reloading && ammo > 0;

        if (fireDown) { shooting = true; releasing = false; }
        else if (shooting) { shooting = false; releasing = true; }

        // Override upper body pose for firing while moving/standing, or full pose while crouching.
        if (shooting || releasing)
        {
            if (crouching) want = shooting ? crouchShoot : crouchShootEnd;
            else           want.torso = shooting ? shootTorso : shootRelease;
        }

        firedThisFrame = false;

        if (shooting)
        {
            fireTimer -= deltaTime;
            if (fireTimer <= 0.0f)
            {
                firedThisFrame = true;
                fireTimer += fireInterval;

                // Deplete ammo clip and initiate reload upon hitting zero.
                --ammo;

                if (ammo <= 0)
                {
                    ammo = 0;
                    reloading = true;
                    reloadTimer = reloadTime;

                    shooting = false;
                    releasing = false;
                }
            }
        }
        else
        {
            fireTimer = 0.0f;
        }

        // Process active reload timer.
        if (reloading)
        {
            reloadTimer -= deltaTime;

            // Display torso reload animation unless crouched.
            if (!crouching) want.torso = reloadTorso;

            if (reloadTimer <= 0.0f)
            {
                reloading = false;
                ammo = magazineSize;
            }
        }
    }

    // Applies hit damage and triggers invulnerability flashing.
    bool Player::TakeDamage(int amount)
    {
        if (amount <= 0) return false;
        if (invulnerableTimer > 0.0f) return false;

        if (!infiniteHealth)
        {
            health -= amount;
            if (health < 0) health = 0;
        }

        invulnerableTimer = invulnerableTime;
        return true;
    }

    // Calculates current world-space bounding box for collisions.
    AABB Player::GetBounds() const
    {
        AABB box;
        box.x = position.x + hitboxOffsetX;
        box.y = position.y + hitboxOffsetY;
        box.w = hitboxWidth;
        box.h = hitboxHeight;
        return box;
    }

    // Pass movement vectors to solver and update ground/ceiling collision state.
    void Player::MoveAndCollide(float moveX, float moveY, const List<Collider>& colliders)
    {
        AABB body;
        body.x = hitboxOffsetX;
        body.y = hitboxOffsetY;
        body.w = hitboxWidth;
        body.h = hitboxHeight;

        const MoveResult result = MoveBody(position, body, moveX, moveY, colliders);

        onGround = result.onGround;

        // Reset vertical velocity whenever hitting a floor or ceiling.
        if (result.onGround || result.hitCeiling) velocityY = 0.0f;
    }

    // Renders legs and torso layers, applying invulnerability blinking and horizontal mirroring.
    void Player::Draw(const RenderManager& renderer) const
    {
        // Blink sprite during invulnerability frames.
        if (invulnerableTimer > 0.0f)
        {
            const int blink = static_cast<int>(invulnerableTimer / flashInterval);
            if ((blink & 1) == 0) return;
        }

        // Draw leg layer (never uses custom torso nudges).
        const float2 legsOrigin = FrameOrigin(legs, legsPlay, 0.0f, 0.0f);

        legs.Draw(legsPlay, renderer,
                  renderer.ToViewX(legsOrigin.x),
                  renderer.ToViewY(legsOrigin.y),
                  !facingRight);

        if (current.torso < 0) return; // Full body sprite active (e.g. crouch).

        // Determine offset nudges for torso layer alignment during action frames.
        const bool firing = (current.torso == shootTorso || current.torso == shootRelease);
        const bool bolting = (current.torso == reloadTorso);

        float nudgeX = 0.0f;
        float nudgeY = 0.0f;

        if (firing) { nudgeX = current.shootX;  nudgeY = current.shootY;  }
        else if (bolting) { nudgeX = reloadNudgeX; nudgeY = reloadNudgeY; }

        const float2 torsoOrigin = FrameOrigin(torso, torsoPlay, nudgeX, nudgeY);

        torso.Draw(torsoPlay, renderer,
                   renderer.ToViewX(torsoOrigin.x),
                   renderer.ToViewY(torsoOrigin.y),
                   !facingRight);
    }

    // Calculates frame rendering top-left coordinate based on sprite dimensions and facing direction.
    float2 Player::FrameOrigin(const AnimationManager& layer, const AnimationPlayer& play,
                               float nudgeX, float nudgeY) const
    {
        const float gap = static_cast<float>(layer.GetFrameWidth(play.clip)) - bodyFrameWidth;

        const float x = facingRight ? (position.x + nudgeX)
                                    : (position.x - gap - nudgeX);

        const float y = position.y + spriteBaseHeight
                      - static_cast<float>(layer.GetFrameHeight(play.clip)) + nudgeY;

        return float2( x, y );
    }

    // Computes world-space position for bullet spawns at gun barrel tip.
    float2 Player::GetMuzzle() const
    {
        const bool crouched = (current.torso < 0);

        const AnimationManager& layer = crouched ? legs : torso;
        const AnimationPlayer& play = crouched ? legsPlay : torsoPlay;

        const float2 origin = FrameOrigin(layer, play,
                                          crouched ? 0.0f : current.shootX,
                                          crouched ? 0.0f : current.shootY);

        // Account for horizontal sprite flip when facing left.
        const float frameWidth = static_cast<float>(layer.GetFrameWidth(play.clip));
        const float offsetX = facingRight ? shootMuzzleX
                                          : (frameWidth - 1.0f - shootMuzzleX);

        return float2( origin.x + offsetX,
                       origin.y + (crouched ? crouchMuzzleY : shootMuzzleY) );
    }
}

#undef MARCO
