#pragma once

#include "managers/animationManager.h"
#include "aabb.h"
#include "list.h"

namespace Tmpl8
{
    class RenderManager;
    class BulletManager;

    // A prisoner goes through these in order and never back.
    enum PowState
    {
        POW_TIED = 0, // sits still until the player reaches him
        POW_FREEING,  // the ropes come off, a one shot
        POW_LEAVING   // runs off to the left and is removed
    };

    /*
    One prisoner. Plain data like Enemy, so List<Pow> can copy it.

    position is the FEET, the same as an enemy, so a POW is placed in Tiled
    exactly like one: a rectangle with the "enemy" property set to "pow".
    */
    struct Pow
    {
        float2 position{ 0.0f, 0.0f };
        float velocityY = 0.0f;

        int state = POW_TIED;
        float leaveTimer = 0.0f; // counts down while running off

        AnimationPlayer anim;
    };

    /*
    Owns the POW sheets and every prisoner in the level. Freed by walking into
    him or by shooting him; a player bullet that frees one is used up, the
    way it would be by an enemy.
    */
    class PowManager
    {
    public:
        PowManager();
        void Unload();

        void Spawn(float feetX, float feetY);

        void Update(float deltaTime, const List<Collider>& colliders,
                    const AABB& playerBox, BulletManager& bullets);

        void Draw(const RenderManager& renderer) const;

        const List<Pow>& GetPows() const { return pows; } // for debug drawing

        AABB GetBounds(const Pow& pow) const;

        int GetRescued() const { return rescued; }

        // Counted at spawn, because a freed POW is removed once he has run off.
        bool AllRescued() const { return rescued >= spawned; }

    private:
        void Free(Pow& pow);

        List<Pow> pows;
        AnimationManager anims;

        int tiedClip = -1;
        int freeingClip = -1;
        int leavingClip = -1;

        int rescued = 0;
        int spawned = 0;
    };
}
