#include "precomp.h"
#include "player.h"
#include "managers/renderManager.h"
#include "assets.h"

// template.cpp keeps a key state array up to date for us (keystate/IsKeyDown)
// but never declares it in a header, so we do it here rather than keeping a
// second copy of the same array.
extern bool IsKeyDown( const uint key );

#define MARCO ASSETS "assets/Marco/"

namespace Tmpl8
{
    /*
    Registers every animation and remembers the ids. The AnimationManagers own
    the images, so there is no new and no delete in this file; Unload() is
    what hands them back.

    The numbers per line are how many frames are on that sheet and how long one
    frame lasts in seconds (0.10 = 10fps). A trailing false means it plays once
    and holds its last frame instead of looping.

    The order of the calls matters: ids are handed out in sequence, and the
    first clip added to a layer is the one that starts playing.
    */
    Player::Player()
    {
        // Most poses are a matched pair. Idle is the odd one: the legs are a
        // single static frame while the torso has 4 frames of breathing, which
        // two independent timers handle without a thought.
        idle = { legs.AddClip ( MARCO "idleLegsMarco.png",  1, 0.10f ),
                 torso.AddClip( MARCO "idleTorsoMarco.png", 4, 0.15f ), 2.0f, -11.0f };

        walk = { legs.AddClip ( MARCO "runLegsMarco.png",  6, 0.10f ),
                 torso.AddClip( MARCO "runTorsoMarco.png", 6, 0.10f ), 5.0f, -11.0f };

        run  = { legs.AddClip ( MARCO "runFastLegsMarco.png",  6, 0.07f ),
                 torso.AddClip( MARCO "runFastTorsoMarco.png", 6, 0.07f ), 5.0f, -11.0f };

        // Jump plays once and holds; falling is its last 2 frames on a loop.
        // Tucked up legs sit higher, so the firing torso has to follow.
        jump = { legs.AddClip ( MARCO "jumpLegsMarco.png",  6, 0.10f, false ),
                 torso.AddClip( MARCO "jumpTorsoMarco.png", 6, 0.10f, false ), 5.0f, -17.0f };

        fall = { legs.AddSubClip ( jump.legs,  4, 2, 0.12f, true ),
                 torso.AddSubClip( jump.torso, 4, 2, 0.12f, true ), 5.0f, -17.0f };

        // Crouch art is whole bodies, so no torso partner and no entry
        // animation: pressing S drops him straight into the loop.
        crouch     = { legs.AddClip( MARCO "crouch2Marco.png",    4, 0.12f ), -1 };
        crouchWalk = { legs.AddClip( MARCO "crouchWalkMarco.png", 7, 0.10f ), -1 };

        // Crouch-shooting is a complete sprite, head and legs in one, so it
        // goes on the LEGS layer and nothing overlays it.
        const int crouchShootSheet = legs.AddClip( MARCO "crouchShootMarco.png", 10, 0.06f );
        crouchShoot    = { legs.AddSubClip( crouchShootSheet, 0, 4, 0.06f, true  ), -1 };
        crouchShootEnd = { legs.AddSubClip( crouchShootSheet, 4, 6, 0.08f, false ), -1 };

        // Standing fire. Frames 0..3 are the shot itself, the only ones where
        // the muzzle flash reaches out to x=50; 4..9 are him lowering the gun.
        const int shootSheet = torso.AddClip( MARCO "shootTorsoMarco.png", 10, 0.06f );
        shootTorso   = torso.AddSubClip( shootSheet, 0, 4, 0.06f, true  );
        shootRelease = torso.AddSubClip( shootSheet, 4, 6, 0.08f, false );
    }

    void Player::Unload()
    {
        legs.Unload();
        torso.Unload();
    }

    /*
    One frame: read the keys, move, then pick the animation.

    Multiplying speeds by deltaTime is what makes him travel the same distance
    per second whatever the framerate; without it he would sprint on a fast pc
    and crawl on a slow one.
    */
    void Player::Update(float deltaTime, const List<Collider>& colliders)
    {
        // Rebuilt from the keys every frame, so letting go stops him dead.
        // Vertical is not read from the keys at all: gravity owns it, and a
        // key can only give it one shove upwards.
        float inputX = 0.0f;
        if (IsKeyDown( GLFW_KEY_A )) inputX -= 1.0f;
        if (IsKeyDown( GLFW_KEY_D )) inputX += 1.0f;

        const bool runDown = IsKeyDown( GLFW_KEY_LEFT_SHIFT ) || IsKeyDown( GLFW_KEY_RIGHT_SHIFT );
        const bool crouching = onGround && IsKeyDown( GLFW_KEY_S );

        // Crouch beats run: you cannot sprint while ducked, you shuffle.
        const float moveSpeed = crouching ? crouchSpeed : (runDown ? runSpeed : walkSpeed);

        // Keep facing the last direction pressed when we stop.
        if (inputX < 0.0f) facingRight = false;
        else if (inputX > 0.0f) facingRight = true;

        /*
        A jump is just setting the vertical speed to a negative number once.
        Nothing pushes him up after that: gravity eats away at it every frame
        until it turns positive and he comes back down, so the arc falls out
        of these two lines on its own.
        */
        const bool jumpDown = IsKeyDown( GLFW_KEY_SPACE ) || IsKeyDown( GLFW_KEY_W );
        if (jumpDown && !jumpHeld && onGround) velocityY = -jumpSpeed;
        jumpHeld = jumpDown;

        /*
        Gravity every frame, no exceptions. Even standing still he is pulled
        down a little, which is exactly what keeps onGround true: he keeps
        pressing into the floor, so MoveAndCollide keeps reporting a landing.

        The clamp is the anti tunneling rule: never fall so fast in one frame
        that you could cross a whole floor without ever overlapping it.
        */
        velocityY = fminf( velocityY + gravity * deltaTime, maxFallSpeed );

        // direction * speed * time = distance, but the level gets a say in
        // how much of it actually happens
        MoveAndCollide(inputX * moveSpeed * deltaTime,
                       velocityY * deltaTime,
                       colliders);

        /*
        Pick the pose. Being in the air beats everything: whatever his feet are
        doing, if there is nothing under them he is jumping or falling.
        velocityY turns positive at the top of the arc, so that sign flip is
        exactly the moment a jump becomes a fall, no timers needed.

        On the ground only HORIZONTAL movement counts as running. Checking
        vertical too would break now that gravity exists, because he is always
        falling a tiny bit and the run animation would never stop.
        */
        Pose want;
        if (!onGround)          want = (velocityY < 0.0f) ? jump : fall;
        else if (crouching)     want = (inputX != 0.0f) ? crouchWalk : crouch;
        else if (inputX != 0.0f)want = runDown ? run : walk;
        else                    want = idle;

        /*
        Firing. Letting go starts the wind down, pressing again cancels it.

        Standing, this replaces only the TORSO so the legs carry on running or
        jumping underneath untouched. Crouching is different: the crouch shoot
        sheet is a complete sprite, head and legs in one, so it replaces the
        whole pose instead of drawing his upper half twice.
        */
        const bool fireDown = IsKeyDown( GLFW_KEY_F );
        if (fireDown) { shooting = true; releasing = false; }
        else if (shooting) { shooting = false; releasing = true; }

        if (shooting || releasing)
        {
            if (crouching) want = shooting ? crouchShoot : crouchShootEnd;
            else           want.torso = shooting ? shootTorso : shootRelease;
        }

        legs.Play(want.legs);
        legs.Update(deltaTime);

        if (want.torso >= 0)
        {
            torso.Play(want.torso);
            torso.Update(deltaTime);
        }

        // The wind down is a one shot, so ask whichever layer is playing it.
        // Crouched that is the legs, standing it is the torso; asking the
        // wrong one would leave him stuck in the firing pose forever.
        if (releasing && (crouching ? legs.IsFinished() : torso.IsFinished()))
        {
            releasing = false;
        }

        current = want; // Draw is const and runs after us, so it just reads this
    }

    // position is the top left of the SPRITE, so shift by the offsets to get
    // the smaller box the level is allowed to stop.
    AABB Player::GetBounds() const
    {
        AABB box;
        box.x = position.x + hitboxOffsetX;
        box.y = position.y + hitboxOffsetY;
        box.w = hitboxWidth;
        box.h = hitboxHeight;
        return box;
    }

    /*
    Moves the player and refuses to let him end up inside anything solid.

    The important idea is ONE AXIS AT A TIME. Move horizontally, fix any
    overlap that caused, then move vertically and fix that. Doing both at once
    is where collision code usually goes wrong: once the boxes overlap you can
    no longer tell which direction he came from, so you cannot tell which side
    to push him out of. One axis at a time the answer is always known - if he
    was moving right, he gets pushed left, full stop.

    It is also what lets you slide along a wall: walking diagonally into one,
    the x move is cancelled but the y move still happens.

    Pushing him out puts his edge exactly ON the wall's edge. Overlaps() counts
    touching as not overlapping, so that spot is free and he does not read as
    stuck next frame. GetBounds() is recomputed inside the loop because pushing
    him out of the first wall moves him, and the second test has to use the new
    position.

    This only ever stops him, it never moves him on its own: it is handed a
    distance and decides how much of it is allowed to happen. What produced
    that distance (keys, gravity, a jump) is none of its business.
    */
    void Player::MoveAndCollide(float moveX, float moveY, const List<Collider>& colliders)
    {
        // ---- horizontal ----
        position.x += moveX;

        if (moveX != 0.0f)
        {
            for (const Collider& collider : colliders)
            {
                // A one way platform is a floor, not a wall. Stopping you
                // sideways would snag you on the lip of every platform you
                // walk past, so they are only looked at on the vertical pass.
                if (collider.oneWay) continue;

                const AABB& solid = collider.box;
                if (!Overlaps(GetBounds(), solid)) continue;

                if (moveX > 0.0f) position.x = solid.Left() - hitboxOffsetX - hitboxWidth;
                else              position.x = solid.Right() - hitboxOffsetX;
            }
        }

        /*
        ---- vertical ----
        Same again, but this axis also answers two questions for the jumping
        code: is he standing on something, and should his vertical speed be
        thrown away.

        We assume he is airborne and only prove otherwise, so walking off a
        ledge leaves onGround false and he cannot jump out of thin air.

        velocityY is zeroed on ANY vertical hit. Landing: otherwise his speed
        would keep growing while he stands there and he would drop like a stone
        the moment he stepped off. Ceiling: it stops him sticking to it.
        */
        onGround = false;

        // Where his feet were BEFORE this move. This one number is what makes
        // one way platforms work, see below.
        const float previousBottom = GetBounds().Bottom();

        position.y += moveY;

        if (moveY != 0.0f)
        {
            for (const Collider& collider : colliders)
            {
                const AABB& solid = collider.box;
                if (!Overlaps(GetBounds(), solid)) continue;

                /*
                The one way rule, two tests, and failing either means he is not
                landing on this platform so we ignore it completely.

                1. moveY <= 0 means he is rising, and a platform never stops
                   you going up: that is the whole point.
                2. previousBottom > solid.Top() means his feet were ALREADY
                   past the surface when the frame started, so he is inside it
                   or under it, not coming down onto it. Without this he would
                   get snapped on top the instant he jumped through.

                Standing on one keeps working because we snap his feet exactly
                onto Top(), so next frame previousBottom == Top(), which is not
                GREATER than it, and he lands again.
                */
                if (collider.oneWay)
                {
                    if (moveY <= 0.0f) continue;
                    if (previousBottom > solid.Top()) continue;
                }

                if (moveY > 0.0f)
                {
                    position.y = solid.Top() - hitboxOffsetY - hitboxHeight;
                    onGround = true;
                }
                else
                {
                    position.y = solid.Bottom() - hitboxOffsetY; // bonked a ceiling
                }

                velocityY = 0.0f;
            }
        }
    }

    /*
    Hands the player's world position to the renderer and lets it do the rest.
    position is where he is on the map, which can be far outside the window;
    ToViewX/ToViewY add the camera offset and DrawFrame applies the zoom.

    The sheets only contain him facing right, so walking left is drawn by
    reading every row of pixels backwards, hence the '!'.
    */
    void Player::Draw(Surface* target, const RenderManager& renderer) const
    {
        /*
        Line the clips up by their FEET, not their top corner.

        A sprite is drawn downwards from the position you give it, so two clips
        of different heights drawn at the same y end up with their feet in
        different places. position.y + spriteBaseHeight is where his feet are,
        and subtracting the height of whatever is playing gives the top corner
        that puts them there. Add a 96 tall death animation later and it lines
        up with no extra code.

        Horizontally, most frames are 34 wide and land on position.x. The wide
        ones carry that extra as padding on the GUN side, so mirroring one puts
        the padding on the wrong side and it has to be slid back.
        */
        const float feetY = position.y + spriteBaseHeight;

        const float legsGap = static_cast<float>(legs.GetFrameWidth()) - bodyFrameWidth;
        const float legsX = facingRight ? position.x : position.x - legsGap;

        legs.Draw(target, renderer,
                  renderer.ToViewX(legsX),
                  renderer.ToViewY(feetY - static_cast<float>(legs.GetFrameHeight())),
                  !facingRight);

        if (current.torso < 0) return; // crouching: one sprite is all there is

        /*
        The torso on top. For the paired sheets this lands at exactly the same
        spot as the legs, because the merge tool baked the alignment into the
        images. The firing torso is the exception and gets the pose's hand
        tuned nudge, mirrored along with everything else so the gun does not
        drift the wrong way when he turns.
        */
        const bool firing = (current.torso == shootTorso || current.torso == shootRelease);

        const float torsoGap = static_cast<float>(torso.GetFrameWidth()) - bodyFrameWidth;
        const float nudgeX = firing ? current.shootX : 0.0f;
        const float nudgeY = firing ? current.shootY : 0.0f;

        const float torsoX = facingRight ? (position.x + nudgeX)
                                         : (position.x - torsoGap - nudgeX);

        torso.Draw(target, renderer,
                   renderer.ToViewX(torsoX),
                   renderer.ToViewY(feetY - static_cast<float>(torso.GetFrameHeight()) + nudgeY),
                   !facingRight);
    }
}

#undef MARCO
