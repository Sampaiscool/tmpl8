// Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#include "precomp.h"
#include "game.h"
#include "assets.h"

namespace Tmpl8
{
    /*
    Where the player sits on screen, as a fraction of the visible area. 0.5 /
    0.5 would be dead centre; this puts him a tenth in from the left and two
    thirds down, so you can see the level coming at you and the ground under
    him. Gameplay tuning, which is why it lives here and not in the renderer.
    */
    static const float CAMERA_ANCHOR_X = 0.1f;
    static const float CAMERA_ANCHOR_Y = 0.60f;

    /*
    Collision is invisible by nature, you only ever see what it stops. With
    this on you can check the shapes line up with the art and that the hitbox
    sits nicely around Marco. Set it to false when you are done looking.
    */
    static const bool SHOW_COLLIDERS = true;
    static const unsigned int COLOR_SOLID = 0x00FF00;    // level geometry, green
    static const unsigned int COLOR_PLATFORM = 0x00CCFF; // one way platforms, cyan
    static const unsigned int COLOR_PLAYER = 0xFFFF00;   // player hitbox, yellow

    /*
    Order matters: the zoom is worked out from the tile size, and we only know
    that once the JSON has been parsed. If the load fails the renderer keeps
    its safe 1:1 defaults, so we still get a window with the player in it
    instead of a crash.
    */
    void Game::Init()
    {
        if (mission.Load(ASSETS "maps/superslug.json"))
        {
            renderer.SetupZoom(mission.GetTileWidth());
        }

        player.SetPosition(-1350.0f, 83.0f); // starting world position
    }

    /*
    One frame, in the order it has to happen: wipe the screen, move the player,
    point the camera at where he ended up, then draw the level and him on top.

    The camera has to be updated between moving and drawing. Draw first and
    move the camera after and everything is a frame behind him, so he jitters
    against the background while walking.
    */
    void Game::Tick(float deltaTime)
    {
        screen->Clear(0x1e1e1e); // dark background so you can see the map edges

        /*
        The template always hands us MILLISECONDS (template.cpp: deltaTime =
        min(500.0f, 1000.0f * timer.elapsed())), so this is just a unit change.

        The clamp limits how much SIMULATED time one frame may advance; it is
        not an fps cap. If the game stalls (dragging the window, a breakpoint)
        a 400ms frame would move the player 72 pixels in one step, straight
        through any wall. That is tunneling. The cost of clamping is that the
        game runs in slow motion during a stall instead of catching up.
        */
        const float dt = fminf( deltaTime / 1000.0f, 0.05f );

        player.Update(dt, mission.GetColliders());
        renderer.FollowTarget(player.GetX(), player.GetY(),
                              CAMERA_ANCHOR_X, CAMERA_ANCHOR_Y);

        mission.Draw(screen, renderer);
        player.Draw(screen, renderer);

        if (SHOW_COLLIDERS) DrawDebugColliders();
    }

    /*
    Drawn last so it sits on top of everything. Both the level shapes and the
    player box are stored in WORLD pixels, so they go through the same
    ToViewX/ToViewY as the rest of the drawing; that is what keeps them glued
    to the level while the camera moves.
    */
    void Game::DrawDebugColliders()
    {
        for (const Collider& collider : mission.GetColliders())
        {
            const AABB& box = collider.box;

            // one way platforms in their own colour, so you can see at a
            // glance which shapes you can jump up through
            const unsigned int color = collider.oneWay ? COLOR_PLATFORM : COLOR_SOLID;

            renderer.DrawBox(screen,
                             renderer.ToViewX(box.x), renderer.ToViewY(box.y),
                             static_cast<int>(box.w), static_cast<int>(box.h),
                             color);
        }

        const AABB box = player.GetBounds();
        renderer.DrawBox(screen,
                         renderer.ToViewX(box.x), renderer.ToViewY(box.y),
                         static_cast<int>(box.w), static_cast<int>(box.h),
                         COLOR_PLAYER);
    }

    // The managers are plain members and clean up after themselves, but
    // Unload() is called anyway so the tileset images are freed at a moment we
    // chose rather than whenever Game itself is destroyed.
    void Game::Shutdown()
    {
        mission.Unload();
    }

} // namespace Tmpl8
