#include "precomp.h"
#include "collision.h"

namespace Tmpl8
{
    // Recomputed every test, because pushing out of one wall moves the body.
    static AABB BoxAt(const float2& position, const AABB& body)
    {
        AABB box;
        box.x = position.x + body.x;
        box.y = position.y + body.y;
        box.w = body.w;
        box.h = body.h;
        return box;
    }

    // Broad phase: the union of where the body is and where it is trying to
    // get to. Resolving only ever pulls it back towards the start, so nothing
    // outside this box can be touched this frame and one cheap test throws
    // most of the level away before the per axis work below.
    static AABB TravelBox(const float2& position, const AABB& body,
                          float moveX, float moveY)
    {
        const AABB start = BoxAt(position, body);

        AABB box;
        box.x = fminf(start.x, start.x + moveX);
        box.y = fminf(start.y, start.y + moveY);
        box.w = start.w + fabsf(moveX);
        box.h = start.h + fabsf(moveY);
        return box;
    }

    MoveResult MoveBody(float2& position, const AABB& body,
                        float moveX, float moveY,
                        const List<Collider>& colliders)
    {
        MoveResult result;

        const AABB travel = TravelBox(position, body, moveX, moveY);

        position.x += moveX;

        if (moveX != 0.0f)
        {
            for (const Collider& collider : colliders)
            {
                // Platforms are floors, not walls, or you would snag on the
                // lip of every one you walk past.
                if (collider.oneWay) continue;

                const AABB& solid = collider.box;

                // Broad, then narrow.
                if (!Overlaps(travel, solid)) continue;
                if (!Overlaps(BoxAt(position, body), solid)) continue;

                if (moveX > 0.0f) position.x = solid.Left() - body.x - body.w;
                else              position.x = solid.Right() - body.x;

                result.blockedX = true;
            }
        }

        // Where the feet were before the move, which is what makes one way
        // platforms work.
        const float previousBottom = BoxAt(position, body).Bottom();

        position.y += moveY;

        if (moveY != 0.0f)
        {
            for (const Collider& collider : colliders)
            {
                const AABB& solid = collider.box;

                if (!Overlaps(travel, solid)) continue;
                if (!Overlaps(BoxAt(position, body), solid)) continue;

                // Rising never lands. Feet already past the surface mean it is
                // inside or under the platform, not coming down onto it.
                // Standing works because the feet snap exactly onto Top(), so
                // next frame previousBottom == Top(), which is not greater.
                if (collider.oneWay)
                {
                    if (moveY <= 0.0f) continue;
                    if (previousBottom > solid.Top()) continue;
                }

                if (moveY > 0.0f)
                {
                    position.y = solid.Top() - body.y - body.h;
                    result.onGround = true;
                }
                else
                {
                    position.y = solid.Bottom() - body.y;
                    result.hitCeiling = true;
                }
            }
        }

        return result;
    }
}
