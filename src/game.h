// Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#pragma once

#include "managers/missionManager.h"
#include "managers/renderManager.h"
#include "player.h"
#include "bullet.h"
#include "enemy.h"
#include "pow.h"
#include "titleScreen.h"
#include "victoryScreen.h"

namespace Tmpl8
{
    // Glue only: owns the managers and decides the order things happen in.
    // The input callbacks are empty because the template keeps its own key
    // state array, but TheApp declares them pure virtual so they must exist.
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
        void DrawWorld();
        void DrawDebugColliders();

        // Every enemy dead and every POW free.
        bool MissionCleared() const;

        MissionManager mission;
        RenderManager renderer;
        Player player;

        // Part of the running level, not part of the Player: he only says
        // when and where a shot left his gun.
        BulletManager bullets;
        EnemyManager enemies;
        PowManager pows;

        // Shown until Enter is pressed; the mission is already loaded behind it.
        TitleScreen title;
        bool onTitle = true;

        // Shown over the frozen level once the mission is cleared or the
        // player dies. On a win the timer runs first, so the last POW can be
        // seen running off.
        VictoryScreen victory;
        bool missionOver = false;
        float victoryDelay = 0.0f;

        // The only camera state that survives between frames.
        float cameraAnchorX = 0.0f;
    };

} // namespace Tmpl8
