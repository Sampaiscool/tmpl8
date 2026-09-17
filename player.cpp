#include "precomp.h"
#include "player.h"
#include "managers/renderManager.h"

namespace Tmpl8
{
    /*
    This code:
    Registers every animation the player needs and remembers the id of each
    one. The AnimationManager loads the images and owns them, so there is no
    new and no matching delete anywhere in this file: when the Player dies,
    its AnimationManager member dies with it and frees the sheets in its own
    destructor. That is why the Player does not need a destructor at all
    anymore.

    The two numbers per line are how many frames are packed into that sheet
    and how long one frame lasts in seconds (0.10 = 10 fps).
    */
    Player::Player()
    {
        #if defined(_WIN32)
            idleClip = animations.AddClip("assets/Marco/Marco_merged.png", 4, 0.10f);
            runClip = animations.AddClip("assets/Marco/runMarco_merged.png", 6, 0.10f);
        #elif defined(__linux__)
            idleClip = animations.AddClip("../assets/Marco/Marco_merged.png", 4, 0.10f);
            runClip = animations.AddClip("../assets/Marco/runMarco_merged.png", 6, 0.10f);
        #endif
    }

    /*
    This code:
    Runs once every frame and does three things in order:
    1. reads the keyboard and builds a movement direction
    2. moves the position using that direction
    3. tells the animation manager wich clip belongs to what we are doing

    Why multiply by deltaTime?
    deltaTime is the amount of seconds the last frame took. By multiplying the
    speed with it, the player moves the same distance per second no matter how
    fast or slow the pc is running. Without it the player would sprint on a
    fast machine and crawl on a slow one.

    Why there is no state enum anymore:
    Play() already ignores a clip that is the same one it is playing, and
    restarts the timer for one that is not. That was the only thing the old
    PlayerState enum and the "did the state change" check were for, so
    "moving or not" can just pick a clip id directly.
    */
    void Player::Update(float deltaTime, const bool* keys)
    {
        velocity = { 0.0f, 0.0f }; // rebuild the direction from scratch every frame

        // LIFE IS MOVEMENT
        if (keys['a'] || keys['A']) velocity.x -= 1.0f;
        if (keys['d'] || keys['D']) velocity.x += 1.0f;
        if (keys['w'] || keys['W']) velocity.y -= 1.0f;
        if (keys['s'] || keys['S']) velocity.y += 1.0f;

        // decide wich direction to face, and keep facing it when we stop
        if (velocity.x < 0.0f) facingRight = false;
        else if (velocity.x > 0.0f) facingRight = true;

        // direction * speed * time = distance moved this frame
        position.x += velocity.x * speed * deltaTime;
        position.y += velocity.y * speed * deltaTime;

        // Any movement at all means running, standing still means idle
        bool moving = (velocity.x != 0.0f || velocity.y != 0.0f);
        animations.Play(moving ? runClip : idleClip);
        animations.Update(deltaTime);
    }

    /*
    This code:
    Hands the players world position to the renderer and lets it do the rest.

    World space vs view space:
    position is where the player is on the map, wich can be far outside the
    window. ToViewX/ToViewY add the camera offset that game.cpp set this
    frame, wich slides everything so the player lands where we want him on
    screen. The zoom on top of that happens inside DrawFrame.

    Flipping:
    the spritesheets only contain him facing right, so walking left is drawn
    by reading every row of pixels backwards. facingRight -> normal,
    facing left -> mirrored, hence the '!'.
    */
    void Player::Draw(Surface* target, const RenderManager& renderer) const
    {
        animations.Draw(target, renderer,
                        renderer.ToViewX(position.x),
                        renderer.ToViewY(position.y),
                        !facingRight);
    }
}
