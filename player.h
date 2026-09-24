#pragma once

#include "managers/animationManager.h"
#include "aabb.h"
#include "list.h"

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

        /*
        input + movement + animation.
        'colliders' are the solid rectangles of the level, in world pixels.
        The Player is handed them rather than owning or looking them up,
        so he stays a thing that moves and does not become a thing that
        knows what a mission is.
        */
        void Update(float deltaTime, const bool* keys, const List<Collider>& colliders);

        /*
        Draws the player at his world position. The renderer holds the camera
        and the zoom, so the Player never has to know either of them exists,
        it just says where it is in the world.
        */
        void Draw(Surface* target, const RenderManager& renderer) const;

        /*
        The players solid box in world pixels. It is deliberately NOT the same
        as the sprite: the drawn frame is 34x34 but Marco does not fill all of
        that, there is empty space around him. Colliding with the full frame
        would make him bump into walls while there is still visible air
        between him and them.

        Public because the debug drawing in game.cpp wants to show it.
        */
        AABB GetBounds() const;

        // Small inline getters/setters, game.cpp uses these to follow the player with the camera
        float GetX() const { return position.x; }
        float GetY() const { return position.y; }
        void SetPosition(float x, float y) { position.x = x; position.y = y; }

    private:
        /*
        Moves the player by this much and stops him on anything solid.
        Done one axis at a time, see the long comment in player.cpp.
        */
        void MoveAndCollide(float moveX, float moveY, const List<Collider>& colliders);

        // Tiny helper struct so we can pass x/y around as one thing
        struct Vec2 { float x, y; };

        Vec2 position{ 100.0f, 100.0f }; // world position in pixels, NOT screen position

        /*
        Vertical speed in pixels per second, and unlike the horizontal input
        it SURVIVES between frames. That is the whole difference between
        flying and falling: horizontal movement is rebuilt from the keys every
        frame, so letting go of a key stops you instantly, but vertical speed
        is something gravity keeps adding to, so it builds up the longer you
        are in the air. Negative is upwards, because y grows downwards.
        */
        float velocityY = 0.0f;

        /*
        Owns the spritesheets and runs the frame timer. A plain member, not a
        pointer: it has a destructor that cleans up its own sheets, so there
        is nothing here for us to new or delete.
        */
        AnimationManager legs;

        /*
        The SECOND layer, drawn on top of the legs one. This is what makes
        shooting while running or jumping possible without a seperate sheet
        for every combination: the two layers have their own clip and their
        own timer, so what the legs are doing and what the arms are doing
        never touch eachother.

        AnimationManager needed no changes at all for this. One instance
        already holds exactly one layer's worth of state, so two instances
        give two independent layers for free.
        */
        AnimationManager torso;

        /*
        Clip ids. Almost every state is a PAIR: a legs sheet and a torso
        sheet of the exact same frame size, drawn at the exact same spot. The
        artist lined them up in the canvas, so there is no offset maths at all
        for these; stacking them just rebuilds the whole character.

        Shooting swaps only the TORSO id. The legs carry on with whatever they
        were doing, wich is the entire reason for splitting him up.
        */
        int idleLegs = -1,      idleTorso = -1;
        int walkLegs = -1,      walkTorso = -1;
        int runLegs = -1,       runTorso = -1;   // shift
        int jumpLegs = -1,      jumpTorso = -1;
        int fallLegs = -1,      fallTorso = -1;  // tail of the jump sheets

        /*
        The crouch art is whole bodies, not pairs, so while crouching the torso
        layer is switched off completely: crouch2 loops while he sits there,
        crouchWalk is shuffling along, and crouchShoot is a complete sprite of
        its own that replaces both layers.
        */
        int crouchLoopClip = -1;
        int crouchWalkClip = -1;

        /*
        Firing. shootTorso is a torso layer that overlays the normal body.
        crouchShootClip is NOT a torso: it is a complete crouched sprite,
        head and legs included, so it replaces both layers on its own.

        Only the first 4 frames are used, the ones that actually carry the
        muzzle flash. The rest of the sheet is him lowering the gun again and
        we deliberately do not play it, so letting go of fire snaps straight
        back to the normal animation.
        */
        int shootTorso = -1;
        int crouchShootClip = -1;

        /*
        The wind down, frames 4..9 of the same two sheets: him lowering the
        gun after you let go. Plays once and holds its last frame, then the
        normal animation takes over again. The standing one is a torso, the
        crouched one is a whole sprite, same split as the firing clips.
        */
        int shootRelease = -1;
        int crouchShootRelease = -1;

        // Is the fire key down this frame
        bool shooting = false;

        /*
        Fire has been let go but the gun is still coming down. Keeps the
        firing animation on screen for those last few frames instead of
        snapping straight back to walking.
        */
        bool releasing = false;

        /*
        Which torso clip Update() settled on, or -1 for none. Draw() is const
        and runs after Update(), so it just reads the answer instead of
        working the whole decision out a second time.
        */
        int torsoShowing = -1;

        bool facingRight = true;    // decides if we draw normal or mirrored

        /*
        True while he is standing on something solid. Set by MoveAndCollide
        when a downwards move gets stopped, so it is always the answer for
        THIS frame. You can only start a jump while this is true.
        */
        bool onGround = false;

        /*
        Was the jump key already down last frame? Without this, holding the
        key would make him jump again the instant he lands, over and over.
        Checking "down now AND not down last frame" turns the key into a
        single press instead of a continuous signal.
        */
        bool jumpHeld = false;

        const float walkSpeed = 180.0f;  // normal, pixels per second
        const float runSpeed = 300.0f;   // holding shift
        const float crouchSpeed = 80.0f; // shuffling along while ducked

        /*
        Nudge for the firing torso only, if the gun does not sit in his hands.
        The paired sheets need nothing: they are the same size as their legs
        and already line up. The shoot sheets are 51 wide against the body's
        34, the extra 17 being the gun out in front, and Draw() mirrors that
        gap automaticly when he turns around.
        */
        const float torsoOffsetX = 0.0f;

        /*
        Where the firing torso has to sit, PER LEGS ANIMATION.

        shootTorsoMarco is the one sheet that never went through the merge
        tool, so unlike every other pair it has no alignment baked into the
        image. And it does not want one single correction either: each legs
        sheet holds his hips at a slightly different height, so the torso has
        to follow. Standing still wants x=2, walking wants x=5, and jumping
        wants the torso lifted 17 instead of 11.

        These are set right next to the legs clip in Update(), so there is
        exactly one place that knows which leg pose is playing. Align
        shootTorsoMarco in the merge tool one day and I can bake it into the
        image like all the others, and every one of these drops to zero.
        */
        float shootOffsetX = 2.0f;
        float shootOffsetY = -11.0f;

        /*
        The width a normal body frame is drawn at. Anything wider (the 51 wide
        firing sheets, the 35 wide crouch walk) carries that extra as padding
        on the gun side, so when he faces left and the frame gets mirrored the
        padding lands on the wrong side and has to be slid back. See Draw().
        */
        const float bodyFrameWidth = 34.0f;

        /*
        The height of a NORMAL frame (idle and run are both 34 tall). Every
        clip is lined up by its feet against this, see Player::Draw. Without
        it the 64 tall jump sheet would be drawn from the same top corner as
        the 34 tall ones and he would sink 30 pixels into the floor the
        instant he left the ground.
        */
        const float spriteBaseHeight = 34.0f;

        /*
        The three numbers that decide how jumping feels. Tune these.

        gravity     - how fast falling speeds up, in pixels per second per
                      second. Higher = heavier, floatier games use less.
        jumpSpeed   - the upward speed he gets the moment he jumps. The height
                      this reaches is jumpSpeed^2 / (2 * gravity), so
                      320*320 / (2*900) = about 57 pixels, wich is three and a
                      half tiles on a 16 pixel grid.
        maxFallSpeed- terminal velocity, the fastest he may ever fall. This is
                      not for realism, it stops tunneling: with deltaTime
                      clamped to 0.05s in game.cpp, the biggest step he can
                      take in one frame is 600 * 0.05 = 30 pixels. The thinnest
                      floor in the map is 37 pixels tall, so he can never skip
                      straight through it between two frames.
        */
        const float gravity = 900.0f;
        const float jumpSpeed = 320.0f;
        const float maxFallSpeed = 600.0f;

        /*
        The hitbox, measured from the top left corner of the SPRITE. Tune
        these four numbers until the yellow debug box in game.cpp sits nicely
        around Marco; the frame is 34x34, so this keeps a 10 pixel margin on
        the left and right and 4 at the top.
        */
        const float hitboxOffsetX = 10.0f;
        const float hitboxOffsetY = 4.0f;
        const float hitboxWidth = 14.0f;
        const float hitboxHeight = 30.0f;
    };
}
