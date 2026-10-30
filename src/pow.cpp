#include "precomp.h"
#include "pow.h"
#include "bullet.h"
#include "collision.h"
#include "managers/renderManager.h"
#include "assets.h"

#define POW_DIR ASSETS "assets/POW/"

namespace Tmpl8
{
    // Same fall as the enemies, so a POW placed a little above the floor
    // drops onto it instead of floating.
    static const float POW_GRAVITY = 900.0f;
    static const float POW_MAX_FALL = 600.0f;

    static const float POW_RUN_SPEED = 70.0f;
    static const float POW_LEAVE_TIME = 4.0f; // long enough to be well off screen

    // Both sheets have the figure facing LEFT with his feet on the bottom row,
    // and he only ever runs left, so nothing is ever mirrored.
    static const float POW_BODY_WIDTH = 20.0f;
    static const float POW_BODY_HEIGHT = 32.0f;

    PowManager::PowManager()
    {
        const int tiedSheet = anims.AddClip( POW_DIR "Tied_Free.png", 9, 0.12f );
        tiedClip = anims.AddSubClip( tiedSheet, 0, 1, 1.0f, true );
        freeingClip = anims.AddSubClip( tiedSheet, 1, 8, 0.09f, false );

        leavingClip = anims.AddClip( POW_DIR "Walk_Away.png", 12, 0.06f );
    }

    void PowManager::Unload()
    {
        pows.clear();
        anims.Unload();
    }

    void PowManager::Spawn(float feetX, float feetY)
    {
        Pow pow;
        pow.position = float2( feetX, feetY );
        anims.Play(pow.anim, tiedClip);
        pows.push_back(pow);
        ++spawned;
    }

    AABB PowManager::GetBounds(const Pow& pow) const
    {
        AABB box;
        box.x = pow.position.x - POW_BODY_WIDTH * 0.5f;
        box.y = pow.position.y - POW_BODY_HEIGHT;
        box.w = POW_BODY_WIDTH;
        box.h = POW_BODY_HEIGHT;
        return box;
    }

    void PowManager::Free(Pow& pow)
    {
        pow.state = POW_FREEING;
        anims.Play(pow.anim, freeingClip);
        ++rescued;
    }

    void PowManager::Update(float deltaTime, const List<Collider>& colliders,
                            const AABB& playerBox, BulletManager& bullets)
    {
        for (int i = 0; i < pows.size(); )
        {
            Pow& pow = pows[i];
            const AABB body = GetBounds(pow);

            if (pow.state == POW_TIED)
            {
                // Walking into him is enough, no button.
                if (Overlaps(body, playerBox)) Free(pow);
                else
                {
                    // Backwards, because Kill swaps the last bullet into the hole.
                    for (int b = bullets.GetCount() - 1; b >= 0; --b)
                    {
                        const Bullet& shot = bullets.Get(b);
                        if (!shot.fromPlayer) continue;
                        if (!Overlaps(bullets.GetBounds(shot), body)) continue;

                        bullets.Kill(b);
                        Free(pow);
                        break;
                    }
                }
            }
            else if (pow.state == POW_FREEING)
            {
                if (anims.IsFinished(pow.anim))
                {
                    pow.state = POW_LEAVING;
                    pow.leaveTimer = POW_LEAVE_TIME;
                    anims.Play(pow.anim, leavingClip);
                }
            }
            else if (pow.state == POW_LEAVING)
            {
                pow.leaveTimer -= deltaTime;
            }

            pow.velocityY = fminf(pow.velocityY + POW_GRAVITY * deltaTime, POW_MAX_FALL);

            const float moveX = (pow.state == POW_LEAVING) ? -POW_RUN_SPEED * deltaTime : 0.0f;

            AABB bodyOffset;
            bodyOffset.x = -POW_BODY_WIDTH * 0.5f;
            bodyOffset.y = -POW_BODY_HEIGHT;
            bodyOffset.w = POW_BODY_WIDTH;
            bodyOffset.h = POW_BODY_HEIGHT;

            const MoveResult result = MoveBody(pow.position, bodyOffset,
                                               moveX, pow.velocityY * deltaTime,
                                               colliders);

            if (result.onGround || result.hitCeiling) pow.velocityY = 0.0f;

            anims.Update(pow.anim, deltaTime);

            if (pow.state == POW_LEAVING && pow.leaveTimer <= 0.0f)
            {
                pows.remove_at_swap(i);
            }
            else
            {
                ++i;
            }
        }
    }

    void PowManager::Draw(const RenderManager& renderer) const
    {
        for (const Pow& pow : pows)
        {
            const int width = anims.GetFrameWidth(pow.anim.clip);
            const int height = anims.GetFrameHeight(pow.anim.clip);

            // Centred on the feet, standing on them.
            const int viewX = renderer.ToViewX(pow.position.x - width * 0.5f);
            const int viewY = renderer.ToViewY(pow.position.y - static_cast<float>(height));

            if (!renderer.IsInView(viewX, viewY, width, height)) continue;

            anims.Draw(pow.anim, renderer, viewX, viewY, false);
        }
    }
}

#undef POW_DIR
