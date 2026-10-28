#pragma once

#include "aabb.h"
#include "list.h"

namespace Tmpl8
{
    // What the level did to a body that tried to move through it.
    struct MoveResult
    {
        bool blockedX = false;
        bool onGround = false;
        bool hitCeiling = false;
    };

    // Moves a body and stops it on anything solid. Shared by the player and
    // the enemies, so they stand on the same floors.
    //
    // 'position' is moved in place, 'body' is the hitbox relative to it.
    // One axis at a time: once two boxes overlap you cannot tell which way the
    // body came from, so you cannot tell which side to push it out of.
    //
    // Two phases per collider: broad, a single test against the box the body
    // sweeps this frame, which rejects the rest of the level outright; then
    // narrow, the exact per axis overlap and push out.
    MoveResult MoveBody(float2& position, const AABB& body,
                        float moveX, float moveY,
                        const List<Collider>& colliders);
}
