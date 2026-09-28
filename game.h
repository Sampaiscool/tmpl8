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
    Game is the glue and nothing more: it owns the managers and decides the
    order things happen in each frame. Every actual job lives in a manager.

    The input callbacks are empty because the template already keeps a key
    state array of its own (keystate/IsKeyDown in template.cpp) and the Player
    reads that directly. They still have to exist, TheApp declares them pure
    virtual.
    */
    class Game : public TheApp
    {
    public:
        void Init();
        void Tick(float deltaTime);
        void Shutdown();

        void MouseUp(int) {}
        void MouseDown(int) {}
        void MouseMove(int, int) {}
        void MouseWheel(float) {}
        void KeyUp(int) {}
        void KeyDown(int) {}

    private:
        void DrawDebugColliders();

        MissionManager mission;
        RenderManager renderer;
        Player player;
    };

} // namespace Tmpl8
