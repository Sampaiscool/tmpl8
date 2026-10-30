#pragma once

#include "managers/animationManager.h"
#include "aabb.h"
#include "list.h"

namespace Tmpl8
{
    class RenderManager;

    /*
    Marco. Drawn as TWO layers, legs and torso, each with its own sheets and
    playhead. That is what makes shooting while running or mid jump work
    without a spritesheet for every combination: swapping the torso leaves the
    legs running underneath.
    */
    class Player
    {
    public:
        Player();      // loads the spritesheets
        void Unload(); // and hands them back; no destructor here ever runs

        // colliders are handed in rather than looked up, so he doesn't know unnecessary things
        void Update(float deltaTime, const List<Collider>& colliders);

        void Draw(const RenderManager& renderer) const;

        // His solid box in world pixels. Not the sprite because he does not fill it
        AABB GetBounds() const;

        void SetPosition(float x, float y) { position.x = x; position.y = y; }

        // Did a shot go off this frame? He does not own the bullets.
        bool JustFired() const { return firedThisFrame; }

        // The barrel tip in world pixels, for the pose on screen right now.
        // Only meaningful while a firing clip is playing.
        float2 GetMuzzle() const;

        // Ignored while he is already hurt, so one burst is one hit.
        bool TakeDamage(int amount);

        bool IsFacingRight() const { return facingRight; }

        int GetHealth() const { return health; }
        int GetMaxHealth() const;
        bool IsAlive() const { return health > 0; }
        bool IsHurt() const { return invulnerableTimer > 0.0f; }
        int GetAmmo() const { return ammo; }
        bool IsReloading() const { return reloading; }

    private:
        /*
        One thing the character can be doing. Most poses are a PAIR of sheets of
        the same frame size drawn at the same spot. The crouch sheets are whole
        bodies instead and use torso = -1 for "nothing goes on top of this".

        shootX/shootY exist because shootTorsoMarco never went through the merge
        tool, and it's like messed up or something idk
        */
        struct Pose
        {
            int   legs   = -1;
            int   torso  = -1;
            float shootX = 0.0f;
            float shootY = 0.0f;
        };

        void MoveAndCollide(float moveX, float moveY, const List<Collider>& colliders);

        // Firing, ammo and reloading, which between them decide what the torso
        // layer (or, crouched, the whole body) is playing.
        void UpdateWeapon(float deltaTime, bool crouching, Pose& want);

        // Where one layer's frame lands, as the top left corner in world pixels.
        // Draw and GetMuzzle both need this and must agree.
        float2 FrameOrigin(const AnimationManager& layer, const AnimationPlayer& play,
                           float nudgeX, float nudgeY) const;

        Pose idle, walk, run, jump, fall;
        Pose crouch, crouchWalk;
        Pose crouchShoot, crouchShootEnd; // whole body sheets, no torso layer

        // Only the first 4 frames are the shot; shootRelease is him lowering
        // the gun, played once so letting go does not snap.
        int shootTorso = -1;
        int shootRelease = -1;
        int reloadTorso = -1;

        Pose current; // what Update settled on; Draw just reads it

        float2 position{ 100.0f, 100.0f }; // world pixels, not screen pixels

        float velocityY = 0.0f;

        AnimationManager legs;
        AnimationManager torso;

        AnimationPlayer legsPlay;
        AnimationPlayer torsoPlay;

        bool facingRight = true;
        bool onGround = false; // set by MoveAndCollide, always this frame's answer

        // 'releasing' keeps the wind down animation on screen after you let go.
        bool shooting = false;
        bool releasing = false;

        // Reset to zero whenever the key is up, so the next press always fires
        // on the spot.
        float fireTimer = 0.0f;

        // 'shooting' stays true while the key is held, so the spawn needs its
        // own flag or holding fire would spawn one a frame.
        bool firedThisFrame = false;

        // Both filled in by the constructor, from the tuning constants.
        int health = 0;
        int ammo = 0;

        // While it runs he is untouchable and flashing.
        float invulnerableTimer = 0.0f;

        bool reloading = false;
        float reloadTimer = 0.0f;

        // Turns a held key into one press.
        bool jumpHeld = false;
    };
}
