// Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#include "precomp.h"
#include "game.h"

namespace Tmpl8
{
    /*
    Where the player sits on screen, as a fraction of the visible area.
    0.5 / 0.5 would be dead center; this puts him a tenth in from the left
    and about two thirds down, so you can see the level coming at you and
    the ground he is standing on. Pure gameplay tuning, wich is why it lives
    here and not in the RenderManager.
    */
    static const float CAMERA_ANCHOR_X = 0.1f;
    static const float CAMERA_ANCHOR_Y = 0.60f;

    void Game::Init()
    {
        /*
        Order matters here. The zoom is calculated from the tile size, and we
        only know the tile size once the mission JSON has been parsed, so the
        map has to load first. If it fails the renderer keeps its safe 1:1
        defaults and we still get a window with the player in it instead of a
        crash.
        */
        if (mission.Load("../maps/superslug.json"))
        {
            renderer.SetupZoom(mission.GetTileWidth());
        }

        player = new Player();
        player->SetPosition(-1350.0f, 83.0f); // starting world position
    }

    /*
    This code:
    One frame, in the order it has to happen:
    1. wipe the screen
    2. move the player with the keys that are held down
    3. point the camera at where he ended up
    4. draw the level, then the player on top of it

    The camera has to be updated between 2 and 4: if we drew first and moved
    the camera after, everything would be one frame behind the player and he
    would jitter against the background while walking.
    */
    void Game::Tick(float deltaTime)
    {
        screen->Clear(0x1e1e1e); // dark background so you can see the map edges

        /*
        Delta-time normalization.
        The template always hands us MILLISECONDS (template.cpp: deltaTime =
        min(500.0f, 1000.0f * timer.elapsed())), so we just divide by 1000 to
        get seconds. We used to guess the unit with (deltaTime > 1.0f), wich
        was a trap: any frame faster than 1 ms gives a value like 0.8, that
        fails the test and gets used as 0.8 SECONDS instead of 0.8 ms. An 800x
        multiplier for one frame, so Marco teleports. Vsync is off in this
        template so high framerates are very reachable.

        The clamp below limits how much SIMULATED time one frame may advance,
        it is not an fps cap. If the game stalls (dragging the window, a
        breakpoint) a 400 ms frame would move the player 180 * 0.4 = 72 pixels
        in one step, straight through any wall we add later. That is called
        tunneling. Clamping to 0.05 keeps the step small; the cost is that the
        game runs in slow motion during a stall instead of catching up.
        */
        float dt = deltaTime / 1000.0f;
        if (dt > 0.05f) dt = 0.05f;

        if (player)
        {
            player->Update(dt, keys);
            renderer.FollowTarget(player->GetX(), player->GetY(),
                                  CAMERA_ANCHOR_X, CAMERA_ANCHOR_Y);
        }

        mission.Draw(screen, renderer);

        if (player) player->Draw(screen, renderer);
    }

    /*
    This code:
    The player is the only thing here we allocated by hand, so he is the only
    thing we have to free by hand. The managers are plain members that clean
    up after themselves in their own destructors; mission.Unload() is called
    anyway so the tileset images are gone at a moment we chose, instead of
    whenever Game itself gets destroyed.
    */
    void Game::Shutdown()
    {
        delete player;
        player = nullptr;

        mission.Unload();
    }

} // namespace Tmpl8
