#pragma once

#include "managers/animationManager.h"

namespace Tmpl8
{
    /*
    This code:
    Forward declarations. We only ever use Surface and RenderManager as
    references/pointers in this header, and for those the compiler does not
    need to know what is inside the class, only that the name exists. So we
    can promise "these classes exist somewhere" instead of including their
    headers.

    Why bother?
    Before this, player.h only compiled because it happened to always be
    included AFTER precomp.h (player.cpp line 1, and game.cpp includes
    precomp.h before game.h). That is a hidden dependency on include order:
    the moment somebody writes a new .cpp that starts with #include "player.h"
    it would break with "Surface does not name a type", through no fault of
    their own. Now the header stands on its own.

    player.cpp still needs the real definitions to call methods on them, and
    it gets those from precomp.h on its first line.
    */
    class Surface;
    class RenderManager;

    /*
    This code:
    The Player class holds where the character is, where he is going and wich
    way he is facing. That is all he owns now.

    What moved out:
    the two Sprite pointers, the third pointer aliasing one of them, the frame
    counter, the animation timer, the frame duration, the two frame count
    constants and the PlayerState enum all live in the AnimationManager. The
    enum could go completely, because its only job was deciding wich
    spritesheet to use, and that is now just "play clip idleClip or clip
    runClip". The drawing loop moved to the RenderManager, wich draws the
    tiles with the exact same function.

    Public vs private:
    the outside world (game.cpp) only needs to update, draw and place the
    player. Everything else is private so no other code can put the player in
    an invalid state by accident.
    */
    class Player
    {
    public:
        Player(); // loads the spritesheets into the animation manager

        void Update(float deltaTime, const bool* keys); // input + movement + animation

        /*
        Draws the player at his world position. The renderer holds the camera
        and the zoom, so the Player never has to know either of them exists,
        it just says where it is in the world.
        */
        void Draw(Surface* target, const RenderManager& renderer) const;

        // Small inline getters/setters, game.cpp uses these to follow the player with the camera
        float GetX() const { return position.x; }
        float GetY() const { return position.y; }
        void SetPosition(float x, float y) { position.x = x; position.y = y; }

    private:
        // Tiny helper struct so we can pass x/y around as one thing
        struct Vec2 { float x, y; };

        Vec2 position{ 100.0f, 100.0f }; // world position in pixels, NOT screen position
        Vec2 velocity{ 0.0f, 0.0f };     // direction of this frame, length 1 when moving

        /*
        Owns the spritesheets and runs the frame timer. A plain member, not a
        pointer: it has a destructor that cleans up its own sheets, so there
        is nothing here for us to new or delete.
        */
        AnimationManager animations;

        // Clip ids handed out by animations.AddClip() in the constructor
        int idleClip = -1;
        int runClip = -1;

        bool facingRight = true;    // decides if we draw normal or mirrored
        const float speed = 180.0f; // pixels per second, multiplied with deltaTime
    };
}
