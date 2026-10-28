#pragma once

#include "managers/animationManager.h"
#include "aabb.h"
#include "list.h"

namespace Tmpl8
{
    class RenderManager;

    // Supported projectile varieties managed by BulletManager.
    // PROJECTILE_KINDS serves as the element count for the lookup array.
    enum ProjectileKind
    {
        PROJECTILE_BULLET = 0,  // Fast, flat trajectory player projectile
        PROJECTILE_GRENADE,     // High-arcing explosive projectile affected by gravity
        PROJECTILE_ENEMY_BULLET,// Slower enemy projectile designed for player dodge mechanics
        PROJECTILE_KINDS
    };

    /*
    Data structure representing a single active projectile instance in the world.
    Designed as POD (Plain Old Data) for efficient memory contiguous storage in List container.
    Horizontal direction is implied by the sign of velocityX.
    */
    struct Bullet
    {
        float2 position{ 0.0f, 0.0f }; // World-space coordinates of the sprite's top-left corner
        float velocityX = 0.0f;        // Horizontal movement rate (pixels per second; sign indicates direction)
        float velocityY = 0.0f;        // Vertical movement rate (used by arcing projectiles such as grenades)
        float lifetime = 0.0f;         // Remaining lifespan duration before automatic expiration
        int kind = PROJECTILE_BULLET;  // Projectile type key corresponding to ProjectileKind enum

        // Faction flag preventing self-harm on initial spawn frame.
        bool fromPlayer = true;

        Bullet() = default;
    };

    /*
    Data structure representing an active explosion visual effect instance.
    Carries an independent AnimationPlayer state because explosions originate asynchronously.
    */
    struct Explosion
    {
        float2 position{ 0.0f, 0.0f }; // World-space coordinates of the explosion sprite's top-left corner
        AnimationPlayer play;          // Dedicated animation playhead instance
    };

    /*
    Central manager responsible for spawning, moving, colliding, and rendering all world projectiles.
    Decoupled from unit target resolution to simplify collision responsibilities.
    */
    class BulletManager
    {
    public:
        BulletManager();
        void Unload();

        // Spawns bullets (from either player or enemy)
        void SpawnBullet(float muzzleX, float muzzleY, bool facingRight, bool fromPlayer);

        // Spawns granade
        void SpawnGrenade(float muzzleX, float muzzleY, bool facingRight, bool fromPlayer);

        // Advances projectile positions, update the collision logic and lifetime timers.
        void Update(float deltaTime, const List<Collider>& colliders);

        // Renders all active projectiles and explosions.
        void Draw(const RenderManager& renderer) const;

        // Returns active bullet count.
        int GetCount() const { return bullets.size(); }

        // Accesses active bullet by array index.
        const Bullet& Get(int index) const { return bullets[index]; }

        // Destroys projectile index (also explodes the grande).
        void Kill(int index);

        // Returns active explosion count.
        int GetExplosionCount() const { return explosions.size(); }

        // Computes damage area-of-effect bounding box for target explosion index.
        AABB GetBlastBox(int index) const;

        // Provides read-only access to internal projectile array.
        const List<Bullet>& GetBullets() const { return bullets; }

        // Calculates tight bounding box adjusted for inner sprite graphics art offsets.
        AABB GetBounds(const Bullet& bullet) const;

        // Is the bullet's sprite solid at this world pixel?
        bool IsOpaqueAt(const Bullet& bullet, int worldX, int worldY) const;

    private:
        // Static configuration profile defining physical and visual traits for a projectile type.
        struct ProjectileInfo
        {
            int clip = -1;              // Sprite clip identifier index inside animation manager
            float frameWidth = 0.0f;    // Width of single frame in pixels

            // Bounding box offsets relative to sprite frame bounds.
            float hitboxOffsetX = 0.0f;
            float hitboxOffsetY = 0.0f;
            float hitboxWidth = 0.0f;
            float hitboxHeight = 0.0f;

            float speed = 0.0f;    // Horizontal speed (pixels per second)
            float lift = 0.0f;     // Upward velocity applied on launch
            float gravity = 0.0f;  // Downward acceleration constant
            float lifetime = 1.0f; // Maximum active duration limit (seconds)

            bool explodes = false; // Flag indicating if explosion effect spawns upon removal

            bool landsOnPlatforms = false; // Flag indicating interaction with one-way platform tops
        };

        // Instantiates explosion animation entity at target world position.
        void SpawnExplosion(float atX, float atY);

        // Internal helper to initialize and register new bullet instance.
        void Spawn(int kind, float muzzleX, float muzzleY, bool facingRight, bool fromPlayer);

        // Sub-step movement iteration logic preventing collision tunneling.
        bool Travel(Bullet& bullet, float moveX, float moveY,
                    const List<Collider>& colliders) const;

        // Level collision testing routine against level geometry.
        bool HitsLevel(const Bullet& bullet, const AABB& box,
                       const List<Collider>& colliders) const;

        // Calculates horizontal hitbox offset when projectile sprite is horizontally mirrored.
        float MirroredOffsetX(const ProjectileInfo& info) const
        {
            return info.frameWidth - info.hitboxOffsetX - info.hitboxWidth;
        }

        List<Bullet> bullets;        // Active projectile instance list
        List<Explosion> explosions;  // Active explosion effect list

        AnimationManager sprites;    // Shared animation manager handling projectile sprite sheets

        ProjectileInfo kinds[PROJECTILE_KINDS]; // Predefined configuration array for projectile kinds

        // Shared playheads per projectile type (used for synchronization across identical bullets).
        AnimationPlayer kindPlayer[PROJECTILE_KINDS];

        int explosionClip = -1;      // Sprite clip index for explosion visual sequence

        // Anchor offset points aligning explosion sprite center to detonation location.
        static constexpr float explosionAnchorX = 21.0f;
        static constexpr float explosionAnchorY = 63.0f;

        // Half-width radius determining damage area of explosion blasts.
        static constexpr float explosionRadius = 28.0f;
    };
}
