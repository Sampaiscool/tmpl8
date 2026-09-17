// Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#pragma once

#include "managers/missionManager.h"
#include "managers/renderManager.h"
#include "player.h"

namespace Tmpl8
{
    /*
    This code:
    Game is the glue, nothing more. It owns the managers, forwards input into
    them and decides the order things happen in each frame. Every actual job
    lives in one manager:

    MissionManager   - parses the Tiled JSON, owns the tile data and the
                       tileset images, draws the level.
    RenderManager    - owns the zoom and the camera and does all the pixel
                       writing, for tiles and sprites alike.
    AnimationManager - owned by the Player, owns the spritesheets and runs
                       the frame timers.

    Everything the managers took over used to sit in this class: the tile
    structs, the tileset list, the zoom fields, the camera, the blitter and
    the whole map parser. The zoom setting itself (TILES_ON_SCREEN) moved to
    renderManager.h, because that is what reads it.
    */
    class Game : public TheApp
    {
    public:
        void Init();
        void Tick(float deltaTime);
        void Shutdown();

        void MouseUp(int) {}
        void MouseDown(int) {}
        void MouseMove(int x, int y) { mousePos.x = x; mousePos.y = y; }
        void MouseWheel(float) {}
        // Key released -> mark it as not held. Key pressed -> mark it as held.
        // The '& 511' keeps the index inside the array no matter what code GLFW sends.
        void KeyUp(int key) { keys[key & 511] = false; }
        void KeyDown(int key) { keys[key & 511] = true; }

        int2 mousePos;

    private:
        MissionManager mission;
        RenderManager renderer;

        /*
        A pointer and not a plain member on purpose: the constructor loads
        images off disk, and we want that to happen inside Init() together
        with the map, not whenever the template happens to construct Game.
        Created in Init(), freed in Shutdown().
        */
        Player* player = nullptr;

        bool keys[512] = { false };
    };

} // namespace Tmpl8
