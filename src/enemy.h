#pragma once

#include "managers/animationManager.h"
#include "aabb.h"
#include "list.h"

namespace Tmpl8
{
    class RenderManager;
    class BulletManager;

    // A new weapon is one entry here, one branch in Release(), and a row in the
    // variant table.
    enum EnemyAttack
    {
        ATTACK_NONE = 0, // it walks at you and nothing else
        ATTACK_GUN,
        ATTACK_GRENADE
    };

    // One at a time, no combinations, which keeps Think a single if/else chain.
    enum EnemyState
    {
        ENEMY_IDLE = 0,
        ENEMY_WALK,
        ENEMY_ATTACK,
        ENEMY_DYING
    };

    /*
    One KIND of enemy: which sheets it uses and what its numbers are. Shared by
    every enemy of that kind and never changed while the game runs, so a
    hundred of them cost a hundred small structs rather than a hundred copies
    of six spritesheets.

    THIS is the thing to copy and edit to add a variant; see RegisterVariants.
    A clip id of -1 means "this variant has no such animation".
    */
    struct EnemyVariant
    {
        // What the 'enemy' property in Tiled has to say. A plain pointer
        // because every one is a string literal, so it outlives the game.
        const char* name = "";

        // Idle and walk are TWO LAYERS drawn on top of eachother. The attack is
        // one MERGED sheet with the whole character on it, so it plays on the
        // legs layer with the torso switched off; a merged sheet has no waist
        // to join, which settles the alignment instead of tuning it.
        int idleLegs = -1, idleTorso = -1;
        int walkLegs = -1, walkTorso = -1;
        int attackLegs = -1;

        int attack = ATTACK_NONE;

        // Which frame of attackLegs the shot leaves on. NOT 0 for every weapon:
        // the gun sheet has its flash on the first frame, the grenade sheet
        // winds up for eight.
        int attackReleaseFrame = 0;

        // Where the shot leaves, in pixels from the top left of the ATTACK
        // frame, facing right. Mirrored automatically.
        int muzzleX = 0;
        int muzzleY = 0;

        int maxHealth = 3;
        float walkSpeed = 40.0f;

        // sightHeight stops an enemy on another floor shooting at someone far
        // above it, which a plain distance check cannot tell apart from someone
        // standing right in front.
        float sightRange = 260.0f;
        float sightHeight = 70.0f;
        float attackRange = 200.0f;
        float attackCooldown = 1.5f;

        // How far either side of where it was placed it wanders while it has
        // seen nobody. 0 means it stands and waits, which is what you want for
        // a guard on a ledge it would otherwise walk off.
        float patrolRange = 0.0f;

        float deathDuration = 0.7f; // how long the death flicker lasts

        // All the mummy frames are 64x64 with the figure on the frame's bottom
        // edge, which is why there is one size and not one per clip.
        float frameWidth = 64.0f;
        float frameHeight = 64.0f;

        /*
        Where each sheet sits relative to the legs, in pixels.

        The torso pairs exist because these two layers do not have their
        alignment baked in: each was cropped to its own art and sat on the
        bottom edge of its own 64x64 frame, so both end with their lowest pixel
        on row 63 and the head would land level with the hips. One pair per
        animation, because idle and walk share a rig and the others do not.

        The attack pair is different in kind: that sheet is merged and drawn on
        the legs' layer, so it is where the sheet sits relative to the feet
        rather than a lift. The orange debug box in game.cpp is drawn from the
        FEET, so use it as the fixed reference while tuning.
        */
        int idleTorsoOffsetX = 0;
        int idleTorsoOffsetY = -14;

        int walkTorsoOffsetX = 0;
        int walkTorsoOffsetY = -16;

        int attackOffsetX = 0;
        int attackOffsetY = 0;

        /*
        The hitbox, as a size rather than a rectangle inside the frame, because
        an enemy's position IS its feet: the box hangs off that, centred.

        42 tall covers the whole stacked figure. A 26 pixel box (all the legs
        are) would leave its head and chest unhittable.
        */
        float bodyWidth = 14.0f;
        float bodyHeight = 42.0f;
        float bodyCentreX = 8.0f; // the figure's middle inside its frame
    };

    /*
    One live enemy. Plain data, no pointers, so List<Enemy> can copy it when it
    grows; that is also why the animation is two AnimationPlayers rather than
    two AnimationManagers, which own images.

    position is the FEET: x the middle of its body, y the ground it stands on.
    The figure sits in the left of a 64 wide frame, so if position were the
    frame's corner then turning round would shift the hitbox most of a frame
    sideways.
    */
    struct Enemy
    {
        int variant = 0;

        float2 position{ 0.0f, 0.0f }; // world pixels, the feet
        float velocityY = 0.0f;

        int health = 0;
        bool facingRight = false;

        int state = ENEMY_IDLE;

        // Counts down; the gun is ready at 0, which is why a fresh enemy can
        // shoot the moment it sees you.
        float attackTimer = 0.0f;

        float deathTimer = 0.0f;

        // One shot per swing: the release frame is on screen for several frames
        // of real time, and without this every one of them would fire.
        bool releasedThisAttack = false;

        float patrolOriginX = 0.0f; // so patrolling is "near here"

        AnimationPlayer legs;
        AnimationPlayer torso;
    };

    /*
    Owns the enemies: the sheets, the variant table, and every live one. Game
    hands it the colliders and where the player is. It is handed the
    BulletManager too, because an enemy firing is just a projectile spawn, and
    that way projectiles still know nothing about enemies.
    */
    class EnemyManager
    {
    public:
        EnemyManager();
        void Unload();

        // False if the variant name is not one we know.
        bool Spawn(const char* variantName, float feetX, float feetY, bool facingRight);

        // One frame for all of them. Also resolves the player's bullets against
        // them, because this is the side that knows what an enemy is.
        void Update(float deltaTime, const List<Collider>& colliders,
                    const AABB& playerBox, BulletManager& bullets);

        void Draw(const RenderManager& renderer) const;

        const List<Enemy>& GetEnemies() const { return enemies; } // for debug drawing

        // An enemy's solid box in world pixels, hanging off its feet.
        AABB GetBounds(const Enemy& enemy) const;

    private:
        void RegisterVariants();
        int FindVariant(const char* name) const; // -1 if unknown

        void UpdateOne(Enemy& enemy, float deltaTime, const List<Collider>& colliders,
                       const AABB& playerBox, BulletManager& bullets);

        // The state machine: decides what it is doing and which way it faces.
        void Think(Enemy& enemy, const EnemyVariant& variant, float deltaTime,
                   const AABB& playerBox, BulletManager& bullets);

        void ResolveHits(BulletManager& bullets);

        void Release(Enemy& enemy, BulletManager& bullets) const;

        // Separate because being killed is the one thing that can happen to an
        // enemy from outside its own Update.
        void Kill(Enemy& enemy) const;

        // The top left corner each sheet is drawn at, from the feet. The muzzle
        // goes through AttackOrigin, the same one the weapon is drawn with, so
        // the shot and the barrel cannot drift apart.
        float2 LegsOrigin(const Enemy& enemy) const;
        float2 TorsoOrigin(const Enemy& enemy) const;
        float2 AttackOrigin(const Enemy& enemy) const;

        // Either layer solid at this world pixel counts as the enemy being there.
        bool IsOpaqueAt(const Enemy& enemy, int worldX, int worldY) const;

        List<EnemyVariant> variants;
        List<Enemy> enemies;

        // One manager per layer, shared by every enemy of every variant. The
        // per enemy state is the two AnimationPlayers inside Enemy.
        AnimationManager legs;
        AnimationManager torso;
    };
}
