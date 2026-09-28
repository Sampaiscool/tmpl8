#pragma once

#include "managers/animationManager.h"
#include "aabb.h"
#include "list.h"

namespace Tmpl8
{
    // Only used as references/pointers here, so the compiler just needs to
    // know the names exist. Keeps this header usable no matter what order a
    // .cpp includes things in.
    class Surface;
    class RenderManager;

    /*
    The player holds where he is, where he is going and which way he is facing.

    He is drawn as TWO layers, legs and torso, each with its own
    AnimationManager and its own timer. That is what makes shooting while
    running or mid jump work without a separate spritesheet for every
    combination: swapping the torso leaves the legs running underneath.
    */
    class Player
    {
    public:
        Player(); // loads the spritesheets into the animation managers

        /*
        Input, movement and animation for one frame. 'colliders' are the
        level's solid rectangles in world pixels; the Player is handed them
        rather than looking them up, so he stays a thing that moves instead of
        becoming a thing that knows what a mission is.
        */
        void Update(float deltaTime, const List<Collider>& colliders);

        // The renderer holds the camera and the zoom, so the Player only has
        // to say where he is in the world.
        void Draw(Surface* target, const RenderManager& renderer) const;

        /*
        His solid box in world pixels. Deliberately NOT the sprite: the frame
        is 34x34 but Marco does not fill it, and colliding with the whole frame
        would bump him into walls while there is still visible air between
        them.

        Public so the debug drawing in game.cpp can show it.
        */
        AABB GetBounds() const;

        float GetX() const { return position.x; }
        float GetY() const { return position.y; }
        void SetPosition(float x, float y) { position.x = x; position.y = y; }

    private:
        // Moves by this much and stops on anything solid, one axis at a time.
        void MoveAndCollide(float moveX, float moveY, const List<Collider>& colliders);

        /*
        One thing the character can be doing.

        Most poses are a PAIR of sheets of the same frame size drawn at the
        same spot, so stacking them rebuilds the whole character with no offset
        maths. The crouch sheets are whole bodies instead, and use torso = -1
        to say "nothing goes on top of this".

        shootX/shootY exist because shootTorsoMarco is the one sheet that never
        went through the merge tool, so its alignment is not baked into the
        image. Each legs sheet holds his hips at a slightly different height,
        so the correction belongs to the pose rather than being one constant.
        Align that sheet one day and every one of these drops to zero.
        */
        struct Pose
        {
            int   legs   = -1;
            int   torso  = -1;
            float shootX = 0.0f;
            float shootY = 0.0f;
        };

        Pose idle, walk, run, jump, fall;
        Pose crouch, crouchWalk;
        Pose crouchShoot, crouchShootEnd; // whole body sheets, no torso layer

        /*
        Firing swaps only the TORSO of whatever pose is playing, which is the
        entire reason for splitting him in two. Only the first 4 frames of the
        sheet are the shot itself; shootRelease is frames 4..9, him lowering
        the gun, played once so letting go does not snap.
        */
        int shootTorso = -1;
        int shootRelease = -1;

        Pose current; // what Update settled on; Draw reads it instead of
                      // working the whole decision out a second time

        float2 position{ 100.0f, 100.0f }; // world pixels, NOT screen pixels

        /*
        Vertical speed, and unlike the horizontal input it SURVIVES between
        frames. That is the whole difference between flying and falling:
        horizontal is rebuilt from the keys every frame so letting go stops you
        dead, but gravity keeps adding to this one. Negative is upwards.
        */
        float velocityY = 0.0f;

        AnimationManager legs;
        AnimationManager torso;

        bool facingRight = true;

        // True while standing on something solid. Set by MoveAndCollide when a
        // downward move gets stopped, so it is always this frame's answer.
        bool onGround = false;

        // Fire key state. 'releasing' keeps the wind down animation on screen
        // for its last few frames after you let go.
        bool shooting = false;
        bool releasing = false;

        /*
        Was jump already held last frame? Without this, holding the key would
        fire a new jump the instant he lands, forever. "Down now AND not down
        last frame" turns a held key into a single press.
        */
        bool jumpHeld = false;

        const float walkSpeed = 180.0f;
        const float runSpeed = 300.0f;
        const float crouchSpeed = 80.0f;

        const float gravity = 900.0f;
        const float jumpSpeed = 320.0f;
        const float maxFallSpeed = 600.0f;

        /*
        The size of a NORMAL body frame. Anything wider (51 for the firing
        sheets, 35 for the crouch walk) carries the extra as padding on the gun
        side, so mirroring it puts the padding on the wrong side and Draw has
        to slide it back.

        The height lines every clip up by its FEET. A sprite is drawn downwards
        from the position you give it, so without this the 64 tall jump sheet
        would sink 30 pixels into the floor the moment he left the ground.
        */
        const float bodyFrameWidth = 34.0f;
        const float spriteBaseHeight = 34.0f;

        /*
        The hitbox, measured from the top left of the SPRITE. Tune these until
        the yellow debug box in game.cpp sits nicely around Marco.
        */
        const float hitboxOffsetX = 10.0f;
        const float hitboxOffsetY = 4.0f;
        const float hitboxWidth = 14.0f;
        const float hitboxHeight = 30.0f;
    };
}
