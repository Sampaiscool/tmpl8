#include "precomp.h"
#include "bullet.h"
#include "managers/renderManager.h"
#include "assets.h"

namespace Tmpl8
{
    // Initializes predefined projectile profiles (player bullets, enemy bullets, and grenades).
    // Configures speeds, hitboxes, lifetimes, gravity arc physics, and animation playheads.
    BulletManager::BulletManager()
    {
        // Standard high-speed player bullet.
        ProjectileInfo& bullet = kinds[PROJECTILE_BULLET];
        bullet.clip = sprites.AddClip( ASSETS "assets/bullet.png", 4, 0.04f );
        bullet.frameWidth = 16.0f;
        bullet.hitboxOffsetX = 3.0f;
        bullet.hitboxOffsetY = 6.0f;
        bullet.hitboxWidth = 8.0f;
        bullet.hitboxHeight = 5.0f;
        bullet.speed = 520.0f;
        bullet.lift = 0.0f;
        bullet.gravity = 0.0f;
        bullet.lifetime = 0.5f;

        // Arced grenade projectile.
        ProjectileInfo& grenade = kinds[PROJECTILE_GRENADE];
        grenade.clip = sprites.AddClip( ASSETS "assets/mummyGrenade.png", 8, 0.06f );
        grenade.frameWidth = 34.0f;
        grenade.hitboxOffsetX = 1.0f;
        grenade.hitboxOffsetY = 22.0f;
        grenade.hitboxWidth = 11.0f;
        grenade.hitboxHeight = 11.0f;
        grenade.speed = 190.0f;
        grenade.lift = 260.0f;   // Upward force applied on spawn
        grenade.gravity = 700.0f;
        grenade.lifetime = 3.0f;
        grenade.explodes = true; // Triggers area-of-effect explosion upon removal
        grenade.landsOnPlatforms = true; // Collides with one-way platform tops while descending

        // Slower variant of standard bullet used by enemy entities.
        ProjectileInfo& enemyBullet = kinds[PROJECTILE_ENEMY_BULLET];
        enemyBullet = bullet;        // Reuse visual sprite sheet and hitbox properties
        enemyBullet.speed = 180.0f;  // Reduced velocity to give player reaction time
        enemyBullet.lifetime = 2.0f; // Extended lifetime because it's slower

        // Explosion visual effect sheet (single-shot animation).
        explosionClip = sprites.AddClip( ASSETS "assets/explosion.png", 10, 0.045f, false );

        // Initialize shared sprite playheads for each projectile kind.
        for (int kind = 0; kind < PROJECTILE_KINDS; ++kind)
        {
            sprites.Play( kindPlayer[kind], kinds[kind].clip );
        }
    }

    void BulletManager::Unload()
    {
        bullets.clear();
        explosions.clear();
        sprites.Unload();
    }

    // Instantiates explosion animation centered at collision/impact coordinates.
    void BulletManager::SpawnExplosion(float atX, float atY)
    {
        if (explosionClip < 0) return; // If there is no explosion sprite it would break

        Explosion blast;
        blast.position.x = atX - explosionAnchorX;
        blast.position.y = atY - explosionAnchorY;

        // Start animation from frame 0 on dedicated playhead.
        sprites.Play(blast.play, explosionClip);

        explosions.push_back(blast);
    }

    void BulletManager::SpawnBullet(float muzzleX, float muzzleY, bool facingRight, bool fromPlayer)
    {
        Spawn(fromPlayer ? PROJECTILE_BULLET : PROJECTILE_ENEMY_BULLET,
              muzzleX, muzzleY, facingRight, fromPlayer);
    }

    void BulletManager::SpawnGrenade(float muzzleX, float muzzleY, bool facingRight, bool fromPlayer)
    {
        Spawn(PROJECTILE_GRENADE, muzzleX, muzzleY, facingRight, fromPlayer);
    }

    // Instantiates active projectile instance aligned with weapon muzzle coordinates.
    void BulletManager::Spawn(int kind, float muzzleX, float muzzleY, bool facingRight, bool fromPlayer)
    {
        if (kind < 0 || kind >= PROJECTILE_KINDS) return;

        const ProjectileInfo& info = kinds[kind];

        Bullet bullet;
        bullet.kind = kind;
        bullet.fromPlayer = fromPlayer;
        bullet.velocityX = facingRight ? info.speed : -info.speed;
        bullet.velocityY = -info.lift;
        bullet.lifetime = info.lifetime;

        const float offsetX = facingRight ? info.hitboxOffsetX : MirroredOffsetX(info);

        // Align projectile position so leading edge starts at barrel exit point.
        bullet.position.x = facingRight ? (muzzleX - offsetX)
                                        : (muzzleX - offsetX - info.hitboxWidth);
        bullet.position.y = muzzleY - info.hitboxOffsetY - info.hitboxHeight * 0.5f;

        bullets.push_back(bullet);
    }

    // Destroys projectile instance by index, triggering explosion effects.
    void BulletManager::Kill(int index)
    {
        if (index < 0 || index >= bullets.size()) return;

        const Bullet& bullet = bullets[index];

        // Trigger explosion effect at bottom-center of projectile hitbox.
        if (kinds[bullet.kind].explodes)
        {
            const AABB box = GetBounds(bullet);
            SpawnExplosion(box.x + box.w * 0.5f, box.Bottom());
        }

        bullets.remove_at_swap(index);
    }

    // Computes square damage box around explosion center for area-of-effect calculations.
    AABB BulletManager::GetBlastBox(int index) const
    {
        AABB box;
        if (index < 0 || index >= explosions.size()) return box;

        const Explosion& blast = explosions[index];

        box.x = blast.position.x + explosionAnchorX - explosionRadius;
        box.y = blast.position.y + explosionAnchorY - explosionRadius;
        box.w = explosionRadius * 2.0f;
        box.h = explosionRadius * 2.0f;
        return box;
    }

    // Advances timers, physics, sub-step movement, collisions, and active explosion clips.
    void BulletManager::Update(float deltaTime, const List<Collider>& colliders)
    {
        // Advance animation timers for shared projectile sprite sheets.
        for (int kind = 0; kind < PROJECTILE_KINDS; ++kind)
        {
            sprites.Update( kindPlayer[kind], deltaTime );
        }

        // Update explosion playheads and remove finished single-shot animations.
        for (int i = 0; i < explosions.size(); )
        {
            sprites.Update( explosions[i].play, deltaTime );

            if (sprites.IsFinished( explosions[i].play )) explosions.remove_at_swap(i);
            else ++i;
        }

        // Update active projectiles.
        for (int i = 0; i < bullets.size(); )
        {
            Bullet& bullet = bullets[i];

            bullet.lifetime -= deltaTime;

            bool dead = bullet.lifetime <= 0.0f;

            if (!dead)
            {
                // Apply downward gravity acceleration.
                bullet.velocityY += kinds[bullet.kind].gravity * deltaTime;

                dead = Travel(bullet,
                              bullet.velocityX * deltaTime,
                              bullet.velocityY * deltaTime,
                              colliders);
            }

            if (dead) Kill(i); // Triggers explosion if applicable and removes bullet
            else ++i;
        }
    }

    // Sub-steps movement vector into smaller increments to prevent high-speed projectiles from tunneling through thin colliders/walls between frames.
    bool BulletManager::Travel(Bullet& bullet, float moveX, float moveY,
                               const List<Collider>& colliders) const
    {
        const ProjectileInfo& info = kinds[bullet.kind];

        const float step = fminf(info.hitboxWidth, info.hitboxHeight);
        const float distance = fmaxf(fabsf(moveX), fabsf(moveY));

        // Calculate step count to guarantee increments remain smaller than hitbox dimensions.
        const int steps = (step > 0.0f) ? static_cast<int>(distance / step) + 1 : 1;

        const float stepX = moveX / steps;
        const float stepY = moveY / steps;

        for (int i = 0; i < steps; ++i)
        {
            bullet.position.x += stepX;
            bullet.position.y += stepY;

            if (HitsLevel(bullet, GetBounds(bullet), colliders)) return true;
        }

        return false;
    }

    // Evaluates projectile bounding box overlap against level colliders.
    bool BulletManager::HitsLevel(const Bullet& bullet, const AABB& box,
                                  const List<Collider>& colliders) const
    {
        const ProjectileInfo& info = kinds[bullet.kind];

        for (const Collider& collider : colliders)
        {
            // One-way platforms: ignore unless projectile explicitly lands on top while falling.
            if (collider.oneWay)
            {
                if (!info.landsOnPlatforms) continue;
                if (bullet.velocityY <= 0.0f) continue;
            }

            if (Overlaps(box, collider.box)) return true;
        }

        return false;
    }

    // Computes world-space bounding box based on velocity direction and sprite offsets.
    AABB BulletManager::GetBounds(const Bullet& bullet) const
    {
        const ProjectileInfo& info = kinds[bullet.kind];
        const bool movingRight = bullet.velocityX > 0.0f;

        AABB box;
        box.x = bullet.position.x + (movingRight ? info.hitboxOffsetX : MirroredOffsetX(info));
        box.y = bullet.position.y + info.hitboxOffsetY;
        box.w = info.hitboxWidth;
        box.h = info.hitboxHeight;
        return box;
    }

    // Renders active projectiles and explosions with viewport frustum culling.
    void BulletManager::Draw(const RenderManager& renderer) const
    {
        for (const Bullet& bullet : bullets)
        {
            const int clip = kinds[bullet.kind].clip;

            const int width = sprites.GetFrameWidth(clip);
            const int height = sprites.GetFrameHeight(clip);

            const int viewX = renderer.ToViewX(bullet.position.x);
            const int viewY = renderer.ToViewY(bullet.position.y);

            if (!renderer.IsInView(viewX, viewY, width, height)) continue;

            sprites.Draw(kindPlayer[bullet.kind], renderer,
                         viewX, viewY, bullet.velocityX < 0.0f);
        }

        // Render explosion animations on top of projectiles.
        const int blastWidth = sprites.GetFrameWidth(explosionClip);
        const int blastHeight = sprites.GetFrameHeight(explosionClip);

        for (const Explosion& blast : explosions)
        {
            const int viewX = renderer.ToViewX(blast.position.x);
            const int viewY = renderer.ToViewY(blast.position.y);

            if (!renderer.IsInView(viewX, viewY, blastWidth, blastHeight)) continue;

            sprites.Draw(blast.play, renderer, viewX, viewY, false);
        }
    }

    bool BulletManager::IsOpaqueAt(const Bullet& bullet, int worldX, int worldY) const
    {
        if (bullet.kind < 0 || bullet.kind >= PROJECTILE_KINDS) return false;

        int localX = static_cast<int>(worldX - bullet.position.x);
        int localY = static_cast<int>(worldY - bullet.position.y);

        bool flipX = bullet.velocityX < 0.0f;
        return sprites.IsOpaqueAt(kindPlayer[bullet.kind], localX, localY, flipX);
    }
}
