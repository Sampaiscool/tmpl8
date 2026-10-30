// Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#include "precomp.h"
#include "game.h"
#include "assets.h"

namespace Tmpl8
{
    // Player sits this far across the view, leaning LEAD the way he faces.
    static const float CAMERA_ANCHOR_X = 0.3f;
    static const float CAMERA_LEAD = 0.1f;
    static const float CAMERA_TURN_SPEED = 4.0f; // how fast the lean eases over

    // Vertical framing is fixed, the camera does not follow him up and down.
    // CAMERA_WORLD_Y is the middle of the level, parked at ANCHOR_Y of the view.
    static const float CAMERA_WORLD_Y = 78.0f;
    static const float CAMERA_ANCHOR_Y = 0.5f;

    // Debug collision boxes. Set false to hide them.
    static const bool SHOW_COLLIDERS = true;
    static const unsigned int COLOR_SOLID = 0x00FF00;
    static const unsigned int COLOR_PLATFORM = 0x00CCFF;
    static const unsigned int COLOR_PLAYER = 0xFFFF00;
    static const unsigned int COLOR_BULLET = 0xFF00FF;
    static const unsigned int COLOR_ENEMY = 0xFF6600;
    static const unsigned int COLOR_POW = 0xFFFFFF;

    // Seconds between the mission being cleared and the banner coming up.
    static const float VICTORY_DELAY = 1.5f;

    void Game::Init()
    {
        mission.Load(ASSETS "maps/superslug.json");

        // Zoom needs the tile size, so this has to come after the map loads.
        renderer.SetupZoom(mission.GetTileWidth());
        renderer.SetVerticalAnchor(CAMERA_WORLD_Y, CAMERA_ANCHOR_Y);

        player.SetPosition(-1350.0f, 83.0f);

        // Start already leaning
        cameraAnchorX = CAMERA_ANCHOR_X
                      + (player.IsFacingRight() ? -CAMERA_LEAD : CAMERA_LEAD);

        // A POW is placed like any enemy in Tiled, enemy = "pow", but it aint an Enemy, so it is picked off here.
        for (const EnemySpawn& spawn : mission.GetEnemySpawns())
        {
            if (strcmp(spawn.variant, "pow") == 0)
            {
                pows.Spawn(spawn.feetX, spawn.feetY);
                continue;
            }

            if (enemies.Spawn(spawn.variant, spawn.feetX, spawn.feetY, spawn.facingRight)) continue;

            printf( "Unknown enemy variant '%s' at %.0f,%.0f - check the 'enemy' property in Tiled\n",
                    spawn.variant, spawn.feetX, spawn.feetY );
        }

        title.Load(ASSETS "assets/titlescreen.png");
        victory.Load(ASSETS "assets/victoryscreen.png");
    }

    void Game::Tick(float deltaTime)
    {
        // deltaTime is in milliseconds.
        // Clamped so a stall cannot move the player far enough to tunnel through a wall.
        const float dt = fminf( deltaTime / 1000.0f, 0.05f );

        // Nothing in the level moves until the title is dismissed.
        if (onTitle)
        {
            if (title.Update(dt)) onTitle = false;
            title.Draw(screen);
            return;
        }

        // Won: the level stops where it was and the banner goes over it.
        if (won)
        {
            victory.Update(dt);
            DrawWorld();
            victory.Draw(screen);
            return;
        }

        player.Update(dt, mission.GetColliders());

        // Before the spawn below, so a new bullet starts on the muzzle.
        bullets.Update(dt, mission.GetColliders());

        if (player.JustFired())
        {
            const float2 muzzle = player.GetMuzzle();
            bullets.SpawnBullet(muzzle.x, muzzle.y, player.IsFacingRight(), true);
        }

        const AABB body = player.GetBounds();

        // Also resolves the player's bullets against the enemies.
        enemies.Update(dt, mission.GetColliders(), body, bullets);

        // After the enemies, so a bullet that hits an enemy in front of a prisoner is spent on the enemy.
        pows.Update(dt, mission.GetColliders(), body, bullets);

        // Enemy bullets against the player. Backwards, because Kill swaps the last bullet into the hole.
        for (int i = bullets.GetCount() - 1; i >= 0; --i)
        {
            const Bullet& shot = bullets.Get(i);
            if (shot.fromPlayer) continue;

            // if it doesn't touch the player; continue
            if (!Overlaps(bullets.GetBounds(shot), body)) continue;

            player.TakeDamage(1); // ignored if he is already flashing
            bullets.Kill(i);
        }

        // Standing in a blast hurts. TakeDamage ignores repeats itself.
        for (int i = 0; i < bullets.GetExplosionCount(); ++i)
        {
            if (Overlaps(bullets.GetBlastBox(i), body)) player.TakeDamage(1);
        }

        // Ease the lean towards the way he is facing. The expf is what makes it framerate independent.
        const float wantAnchorX = CAMERA_ANCHOR_X
                                + (player.IsFacingRight() ? -CAMERA_LEAD : CAMERA_LEAD);

        cameraAnchorX += (wantAnchorX - cameraAnchorX)
                       * (1.0f - expf(-CAMERA_TURN_SPEED * dt));

        // After the move, before the draw, or everything lags a frame behind.
        renderer.FollowTarget(body.x + body.w * 0.5f, cameraAnchorX);

        if (MissionCleared())
        {
            victoryDelay += dt;
            if (victoryDelay >= VICTORY_DELAY)
            {
                won = true;
                victory.Show(pows.GetRescued());
            }
        }

        DrawWorld();
    }

    // A dying enemy is still in the list until its flicker ends, so this waits
    // for the last one to actually disappear.
    bool Game::MissionCleared() const
    {
        return enemies.GetEnemies().empty() && pows.AllRescued();
    }

    void Game::DrawWorld()
    {
        renderer.Clear(0x1e1e1e);

        // Level, then prisoners and enemies under the player, then bullets over everything.
        mission.Draw(renderer);
        pows.Draw(renderer);
        enemies.Draw(renderer);
        player.Draw(renderer);
        bullets.Draw(renderer);

        if (SHOW_COLLIDERS) DrawDebugColliders();

        renderer.Present(screen); // enlarges the view onto the real screen
    }

    void Game::DrawDebugColliders()
    {
        for (const Collider& collider : mission.GetColliders())
        {
            renderer.DrawWorldBox(collider.box,
                                  collider.oneWay ? COLOR_PLATFORM : COLOR_SOLID);
        }

        renderer.DrawWorldBox(player.GetBounds(), COLOR_PLAYER);

        for (const Bullet& bullet : bullets.GetBullets())
        {
            renderer.DrawWorldBox(bullets.GetBounds(bullet), COLOR_BULLET);
        }

        for (const Enemy& enemy : enemies.GetEnemies())
        {
            renderer.DrawWorldBox(enemies.GetBounds(enemy), COLOR_ENEMY);
        }

        for (const Pow& pow : pows.GetPows())
        {
            renderer.DrawWorldBox(pows.GetBounds(pow), COLOR_POW);
        }
    }

    // template.cpp never deletes the Game :), so no destructor here ever runs.
    void Game::Shutdown()
    {
        player.Unload();
        bullets.Unload();
        enemies.Unload();
        pows.Unload();
        title.Unload();
        victory.Unload();
        mission.Unload();
        renderer.Unload();
    }

} // namespace Tmpl8
