#include "precomp.h"
#include "enemy.h"
#include "bullet.h"
#include "collision.h"
#include "managers/renderManager.h"
#include "assets.h"

#define MUMMY ASSETS "assets/mummyEnemy/"

namespace Tmpl8
{
    // Constant gravity and fall speed cap shared across all enemy types.
    static const float ENEMY_GRAVITY = 900.0f;
    static const float ENEMY_MAX_FALL = 600.0f;

    static const float DEATH_BLINK = 0.07f; // Duration of each flash interval during death animation

    EnemyManager::EnemyManager()
    {
        RegisterVariants();
    }

    void EnemyManager::RegisterVariants()
    {
        // Load reusable animation sheets shared across enemy instances.
        const int idleLegs = legs.AddClip( MUMMY "mymmyIdle_legs.png", 1, 0.20f );
        const int idleTorso = torso.AddClip( MUMMY "mummyIdle_torso.png", 4, 0.18f );

        const int walkLegs = legs.AddClip( MUMMY "mymmuWalk_legs.png", 12, 0.08f );
        const int walkTorso = torso.AddClip( MUMMY "mummyWalk_torso.png", 12, 0.08f );

        // Single merged sheet for combined upper/lower body attack animations.
        const int shootMerged = legs.AddClip( MUMMY "mummyShoot_torso_merged.png",
                                              8, 0.06f, false );

        // Basic Mummy.
        EnemyVariant mummy;
        mummy.name = "mummy";
        mummy.idleLegs = idleLegs;
        mummy.idleTorso = idleTorso;
        mummy.walkLegs = walkLegs;
        mummy.walkTorso = walkTorso;
        mummy.attackLegs = shootMerged;
        mummy.attack = ATTACK_GUN;
        mummy.attackReleaseFrame = 0; // Immediate projectile spawn on frame 0

        // Visual offsets to align foot anchors to the ground plane.
        mummy.attackOffsetX = 0;
        mummy.attackOffsetY = 2;

        // Muzzle position within the frame relative to the top-left corner.
        mummy.muzzleX = 34;
        mummy.muzzleY = 34;
        mummy.maxHealth = 4;
        mummy.walkSpeed = 45.0f;
        mummy.sightRange = 280.0f;
        mummy.attackRange = 120.0f;
        mummy.attackCooldown = 1.4f;
        mummy.patrolRange = 0.0f; // Stationary guard behaviour
        variants.push_back(mummy);

        // Bomber Mummy.
        const int grenadeMerged = legs.AddClip( MUMMY "mummyGrenade_torso_merged.png", 17, 0.05f, false );

        EnemyVariant bomber = mummy;
        bomber.name = "bomber";
        bomber.attack = ATTACK_GRENADE;
        bomber.attackLegs = grenadeMerged;

        // Sprite offset adjustments for distinct body frame alignment.
        bomber.attackOffsetX = -17;
        bomber.attackOffsetY = 0;

        // Release grenade projectile specifically on keyframe 9.
        bomber.attackReleaseFrame = 9;
        bomber.muzzleX = 39;
        bomber.muzzleY = 12;

        bomber.attackRange = 250.0f;
        bomber.attackCooldown = 2.4f;
        variants.push_back(bomber);

        printf( "Enemies: %i variants registered\n", variants.size() );
    }

    void EnemyManager::Unload()
    {
        enemies.clear();
        variants.clear();

        legs.Unload();
        torso.Unload();
    }

    // Look up enemy variant configuration index by string key.
    int EnemyManager::FindVariant(const char* name) const
    {
        if (name == nullptr) return -1;

        for (int i = 0; i < variants.size(); ++i)
        {
            if (strcmp(variants[i].name, name) == 0) return i;
        }

        return -1;
    }

    // Instantiate an enemy instance into world coordinates.
    bool EnemyManager::Spawn(const char* variantName, float feetX, float feetY, bool facingRight)
    {
        const int index = FindVariant(variantName);
        if (index < 0) return false;

        const EnemyVariant& variant = variants[index];

        Enemy enemy;
        enemy.variant = index;
        enemy.position = float2( feetX, feetY );
        enemy.health = variant.maxHealth;
        enemy.facingRight = facingRight;
        enemy.state = ENEMY_IDLE;
        enemy.patrolOriginX = feetX;

        // Initialize animation playhead defaults to prevent invalid clip references.
        legs.Play(enemy.legs, variant.idleLegs);
        torso.Play(enemy.torso, variant.idleTorso);

        enemies.push_back(enemy);
        return true;
    }

    // Derives bounding box directly from foot position anchor.
    AABB EnemyManager::GetBounds(const Enemy& enemy) const
    {
        const EnemyVariant& variant = variants[enemy.variant];

        AABB box;
        box.x = enemy.position.x - variant.bodyWidth * 0.5f;
        box.y = enemy.position.y - variant.bodyHeight;
        box.w = variant.bodyWidth;
        box.h = variant.bodyHeight;
        return box;
    }

    // Computes top-left corner coordinates for the leg sprite layer based on facing direction.
    float2 EnemyManager::LegsOrigin(const Enemy& enemy) const
    {
        const EnemyVariant& variant = variants[enemy.variant];

        const float centre = enemy.facingRight
                           ? variant.bodyCentreX
                           : (variant.frameWidth - 1.0f - variant.bodyCentreX);

        return float2( enemy.position.x - centre,
                       enemy.position.y - variant.frameHeight );
    }

    // Computes top-left corner coordinates for the torso layer, applying per-clip offsets.
    float2 EnemyManager::TorsoOrigin(const Enemy& enemy) const
    {
        const EnemyVariant& variant = variants[enemy.variant];

        int offsetX = variant.idleTorsoOffsetX;
        int offsetY = variant.idleTorsoOffsetY;

        if (enemy.torso.clip >= 0 && enemy.torso.clip == variant.walkTorso)
        {
            offsetX = variant.walkTorsoOffsetX;
            offsetY = variant.walkTorsoOffsetY;
        }

        const float nudgeX = enemy.facingRight ? static_cast<float>(offsetX)
                                               : -static_cast<float>(offsetX);

        const float2 legs = LegsOrigin(enemy);
        return float2( legs.x + nudgeX, legs.y + static_cast<float>(offsetY) );
    }

    // Computes origin coordinates for combined single-sheet attack animations.
    float2 EnemyManager::AttackOrigin(const Enemy& enemy) const
    {
        const EnemyVariant& variant = variants[enemy.variant];

        const float2 base = LegsOrigin(enemy);

        const float nudgeX = enemy.facingRight
                           ? static_cast<float>(variant.attackOffsetX)
                           : -static_cast<float>(variant.attackOffsetX);

        return float2( base.x + nudgeX,
                       base.y + static_cast<float>(variant.attackOffsetY) );
    }

    bool EnemyManager::IsOpaqueAt(const Enemy& enemy, int worldX, int worldY) const
    {
        const EnemyVariant& variant = variants[enemy.variant];

        const bool attacking = enemy.legs.clip >= 0
                             && enemy.legs.clip == variant.attackLegs;

        const float2 legsAt = attacking ? AttackOrigin(enemy) : LegsOrigin(enemy);
        bool flipX = !enemy.facingRight;

        int legsLocalX = static_cast<int>(worldX - legsAt.x);
        int legsLocalY = static_cast<int>(worldY - legsAt.y);

        if (legs.IsOpaqueAt(enemy.legs, legsLocalX, legsLocalY, flipX)) return true;

        if (!attacking)
        {
            const float2 torsoAt = TorsoOrigin(enemy);
            int torsoLocalX = static_cast<int>(worldX - torsoAt.x);
            int torsoLocalY = static_cast<int>(worldY - torsoAt.y);

            if (torso.IsOpaqueAt(enemy.torso, torsoLocalX, torsoLocalY, flipX)) return true;
        }

        return false;
    }

    // Flags enemy state to dying and starts death timer.
    void EnemyManager::Kill(Enemy& enemy) const
    {
        enemy.state = ENEMY_DYING;
        enemy.deathTimer = variants[enemy.variant].deathDuration;
        enemy.health = 0;
    }

    // Spawns projectile from enemy muzzle offset upon reaching specified keyframe.
    void EnemyManager::Release(Enemy& enemy, BulletManager& bullets) const
    {
        const EnemyVariant& variant = variants[enemy.variant];

        const float2 origin = AttackOrigin(enemy);

        const float muzzle = static_cast<float>(variant.muzzleX);

        // Mirror muzzle X position horizontally when facing left.
        const float offsetX = enemy.facingRight
                             ? muzzle
                             : (variant.frameWidth - 1.0f - muzzle);

        const float muzzleX = origin.x + offsetX;
        const float muzzleY = origin.y + static_cast<float>(variant.muzzleY);

        // Spawn hostiles' bullets with fromPlayer set to false.
        if (variant.attack == ATTACK_GUN)
        {
            bullets.SpawnBullet(muzzleX, muzzleY, enemy.facingRight, false);
        }
        else if (variant.attack == ATTACK_GRENADE)
        {
            bullets.SpawnGrenade(muzzleX, muzzleY, enemy.facingRight, false);
        }
    }

    // Primary update iteration: updates state machines, movement, and hit checks.
    void EnemyManager::Update(float deltaTime, const List<Collider>& colliders,
                              const AABB& playerBox, BulletManager& bullets)
    {
        for (int i = 0; i < enemies.size(); )
        {
            UpdateOne(enemies[i], deltaTime, colliders, playerBox, bullets);

            // Swap-remove expired dying enemies without skipping indices.
            if (enemies[i].state == ENEMY_DYING && enemies[i].deathTimer <= 0.0f)
            {
                enemies.remove_at_swap(i);
            }
            else
            {
                ++i;
            }
        }

        ResolveHits(bullets);
    }

    // Evaluates enemy decision tree (targeting player, movement ranges, attack triggers).
    void EnemyManager::Think(Enemy& enemy, const EnemyVariant& variant, float deltaTime,
                             const AABB& playerBox, BulletManager& bullets)
    {
        if (enemy.attackTimer > 0.0f) enemy.attackTimer -= deltaTime;

        // Calculate relative vector to player bounding box center.
        const AABB body = GetBounds(enemy);

        const float toPlayerX = (playerBox.x + playerBox.w * 0.5f) - (body.x + body.w * 0.5f);
        const float toPlayerY = (playerBox.y + playerBox.h * 0.5f) - (body.y + body.h * 0.5f);

        // Rectangular line-of-sight bounds check.
        const bool sees = fabsf(toPlayerX) <= variant.sightRange
                       && fabsf(toPlayerY) <= variant.sightHeight;

        const bool inRange = fabsf(toPlayerX) <= variant.attackRange
                          && fabsf(toPlayerY) <= variant.sightHeight;

        const bool canAttack = variant.attack != ATTACK_NONE;

        if (enemy.state == ENEMY_ATTACK)
        {
            // Execute attack action upon reaching release frame during single-shot animation.
            if (!enemy.releasedThisAttack
                && static_cast<int>(enemy.legs.frame) >= variant.attackReleaseFrame)
            {
                Release(enemy, bullets);
                enemy.releasedThisAttack = true;
            }

            if (legs.IsFinished(enemy.legs))
            {
                enemy.state = ENEMY_IDLE;
                enemy.attackTimer = variant.attackCooldown;
            }
        }
        else if (sees && inRange && canAttack && enemy.attackTimer <= 0.0f)
        {
            // Turn towards player target prior to initiating attack.
            enemy.facingRight = (toPlayerX > 0.0f);

            enemy.state = ENEMY_ATTACK;
            enemy.releasedThisAttack = false;

            // Reset animation playhead state for accurate release frame timing.
            enemy.torso.clip = -1;

            enemy.legs.clip = -1;
            legs.Play(enemy.legs, variant.attackLegs);
        }
        else if (sees && (!inRange || !canAttack))
        {
            // Approach target until within effective weapon range.
            enemy.facingRight = (toPlayerX > 0.0f);
            enemy.state = ENEMY_WALK;
        }
        else if (variant.patrolRange > 0.0f)
        {
            // Execute automated patrol back and forth around spawn origin.
            enemy.state = ENEMY_WALK;

            const float fromHome = enemy.position.x - enemy.patrolOriginX;

            if (fromHome > variant.patrolRange) enemy.facingRight = false;
            else if (fromHome < -variant.patrolRange) enemy.facingRight = true;
        }
        else
        {
            enemy.state = ENEMY_IDLE;
        }
    }

    // Handles gravity, physics resolution, wall turnarounds, and animation tick updates for a single enemy.
    void EnemyManager::UpdateOne(Enemy& enemy, float deltaTime, const List<Collider>& colliders,
                                 const AABB& playerBox, BulletManager& bullets)
    {
        const EnemyVariant& variant = variants[enemy.variant];

        // Apply constant downward gravity acceleration.
        enemy.velocityY = fminf(enemy.velocityY + ENEMY_GRAVITY * deltaTime, ENEMY_MAX_FALL);

        if (enemy.state == ENEMY_DYING) enemy.deathTimer -= deltaTime;
        else Think(enemy, variant, deltaTime, playerBox, bullets);

        // Compute horizontal step distance based on state.
        const float moveX = (enemy.state == ENEMY_WALK)
                          ? (enemy.facingRight ? variant.walkSpeed : -variant.walkSpeed) * deltaTime
                          : 0.0f;

        // Bounding box centered horizontally on position, resting above foot Y.
        AABB bodyOffset;
        bodyOffset.x = -variant.bodyWidth * 0.5f;
        bodyOffset.y = -variant.bodyHeight;
        bodyOffset.w = variant.bodyWidth;
        bodyOffset.h = variant.bodyHeight;

        const MoveResult result = MoveBody(enemy.position, bodyOffset,
                                           moveX, enemy.velocityY * deltaTime,
                                           colliders);

        if (result.onGround || result.hitCeiling) enemy.velocityY = 0.0f;

        // Reverse movement direction upon colliding with obstacle walls.
        if (result.blockedX) enemy.facingRight = !enemy.facingRight;

        // Assign corresponding animation clips for walking and idle states.
        if (enemy.state == ENEMY_WALK)
        {
            legs.Play(enemy.legs, variant.walkLegs);
            torso.Play(enemy.torso, variant.walkTorso);
        }
        else if (enemy.state == ENEMY_IDLE)
        {
            legs.Play(enemy.legs, variant.idleLegs);
            torso.Play(enemy.torso, variant.idleTorso);
        }

        if (enemy.state != ENEMY_DYING)
        {
            legs.Update(enemy.legs, deltaTime);
            torso.Update(enemy.torso, deltaTime);
        }
    }

    // Processes overlaps between player bullets and enemy hitboxes.
    void EnemyManager::ResolveHits(BulletManager& bullets)
    {
        for (int b = bullets.GetCount() - 1; b >= 0; --b)
        {
            const Bullet& bullet = bullets.Get(b);
            if (!bullet.fromPlayer) continue; // Ignore enemy projectiles

            const AABB hitbox = bullets.GetBounds(bullet);

            for (int e = 0; e < enemies.size(); ++e)
            {
                Enemy& enemy = enemies[e];

                if (enemy.state == ENEMY_DYING) continue; // Ignore dead bodies

                AABB enemyBox = GetBounds(enemy);
                if (!Overlaps(hitbox, enemyBox)) continue;

                // Compute bounding box intersection region
                int startX = static_cast<int>(fmaxf(hitbox.x, enemyBox.x));
                int endX   = static_cast<int>(fminf(hitbox.Right(), enemyBox.Right()));
                int startY = static_cast<int>(fmaxf(hitbox.y, enemyBox.y));
                int endY   = static_cast<int>(fminf(hitbox.Bottom(), enemyBox.Bottom()));

                bool hit = false;
                for (int y = startY; y <= endY && !hit; ++y)
                {
                    for (int x = startX; x <= endX; ++x)
                    {
                        if (bullets.IsOpaqueAt(bullet, x, y) && IsOpaqueAt(enemy, x, y))
                        {
                            hit = true;
                            break;
                        }
                    }
                }

                if (!hit) continue;

                --enemy.health;
                if (enemy.health <= 0) Kill(enemy);

                bullets.Kill(b);
                break; // Prevent a single bullet from penetrating multiple enemies
            }
        }
    }

    // Renders active enemies, handling camera view culling, death blinking, and layer stacking.
    void EnemyManager::Draw(const RenderManager& renderer) const
    {
        for (const Enemy& enemy : enemies)
        {
            const EnemyVariant& variant = variants[enemy.variant];

            // Render flashing effect on alternate frames while dying.
            if (enemy.state == ENEMY_DYING)
            {
                const int blink = static_cast<int>(enemy.deathTimer / DEATH_BLINK);
                if ((blink & 1) == 0) continue;
            }

            const bool attacking = enemy.legs.clip >= 0
                                && enemy.legs.clip == variant.attackLegs;

            const float2 legsAt = attacking ? AttackOrigin(enemy) : LegsOrigin(enemy);
            const float2 torsoAt = TorsoOrigin(enemy);

            const int legsX = renderer.ToViewX(legsAt.x);
            const int legsY = renderer.ToViewY(legsAt.y);
            const int torsoY = renderer.ToViewY(torsoAt.y);

            // Compute combined bounding box for view culling across both layers.
            const int top = (legsY < torsoY) ? legsY : torsoY;
            const int spread = (legsY > top) ? (legsY - top) : 0;

            if (!renderer.IsInView(legsX, top,
                                   static_cast<int>(variant.frameWidth),
                                   static_cast<int>(variant.frameHeight) + spread)) continue;

            legs.Draw(enemy.legs, renderer, legsX, legsY, !enemy.facingRight);

            torso.Draw(enemy.torso, renderer,
                       renderer.ToViewX(torsoAt.x), torsoY, !enemy.facingRight);
        }
    }
}

#undef MUMMY
