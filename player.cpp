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
            #define MARCO "assets/Marco/"
        #elif defined(__linux__)
            #define MARCO "../assets/Marco/"
        #endif

        /*
        Every normal state is a legs sheet and a torso sheet of the SAME frame
        size. They get drawn at the same position, so stacking them puts the
        character back together with no offset maths anywhere.

        idle is the odd one: the legs are a single static frame (he is stood
        still, they do not move) while the torso has 4 frames of breathing.
        Two layers with their own timers handle that without a thought.
        */
        idleLegs  = legs.AddClip (MARCO "idleLegsMarco.png",     1, 0.10f);
        idleTorso = torso.AddClip(MARCO "idleTorsoMarco.png",    4, 0.15f);

        walkLegs  = legs.AddClip (MARCO "runLegsMarco.png",      6, 0.10f);
        walkTorso = torso.AddClip(MARCO "runTorsoMarco.png",     6, 0.10f);

        runLegs   = legs.AddClip (MARCO "runFastLegsMarco.png",  6, 0.07f);
        runTorso  = torso.AddClip(MARCO "runFastTorsoMarco.png", 6, 0.07f);

        // jump plays once and holds; falling is its last 2 frames, looping
        jumpLegs  = legs.AddClip (MARCO "jumpLegsMarco.png",     6, 0.10f, false);
        jumpTorso = torso.AddClip(MARCO "jumpTorsoMarco.png",    6, 0.10f, false);
        fallLegs  = legs.AddSubClip (jumpLegs,  4, 2, 0.12f, true);
        fallTorso = torso.AddSubClip(jumpTorso, 4, 2, 0.12f, true);

        /*
        Crouching is whole body art, so no torso partner and no entry
        animation: pressing S drops him straight into the loop.
        */
        crouchLoopClip = legs.AddClip(MARCO "crouch2Marco.png",    4, 0.12f);
        crouchWalkClip = legs.AddClip(MARCO "crouchWalkMarco.png", 7, 0.10f);

        /*
        Crouch-shooting is a complete sprite, head and legs in one, so it goes
        on the LEGS layer and nothing overlays it. Only the first 4 frames,
        the ones with the muzzle flash.
        */
        int crouchShootSheetL = legs.AddClip(MARCO "crouchShootMarco.png", 10, 0.06f);
        crouchShootClip    = legs.AddSubClip(crouchShootSheetL, 0, 4, 0.06f, true);
        crouchShootRelease = legs.AddSubClip(crouchShootSheetL, 4, 6, 0.08f, false);

        /*
        The standing firing torso. Frames 0..3 are the shot itself, the only
        ones where the gun and muzzle flash reach out to x=50. Frames 4..9 are
        him lowering the gun and we do not use them, so letting go of fire
        goes straight back to the normal torso.
        */
        int shootSheet = torso.AddClip(MARCO "shootTorsoMarco.png", 10, 0.06f);
        shootTorso   = torso.AddSubClip(shootSheet, 0, 4, 0.06f, true);
        shootRelease = torso.AddSubClip(shootSheet, 4, 6, 0.08f, false);

        #undef MARCO
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
    void Player::Update(float deltaTime, const bool* keys, const List<Collider>& colliders)
    {
        /*
        Horizontal is rebuilt from the keys every frame, so letting go stops
        him dead. Vertical is NOT read from the keys at all anymore: gravity
        owns it now, and the only thing a key can do is give it one shove
        upwards. That swap is what turns free flying into a platformer.
        */
        float inputX = 0.0f;
        if (keys['a'] || keys['A']) inputX -= 1.0f;
        if (keys['d'] || keys['D']) inputX += 1.0f;

        /*
        Shift runs, S crouches.

        Crouching is only allowed with his feet down, and it cancels the
        walking input rather than fighting it: you plant yourself and duck.
        Doing it this way means the crouch clip can never get interrupted
        halfway by a stray key, and inputX being 0 makes the clip choice
        further down fall through to the crouch branch on its own.
        */
        bool runDown = keys[GLFW_KEY_LEFT_SHIFT] || keys[GLFW_KEY_RIGHT_SHIFT];
        bool crouching = onGround && (keys['s'] || keys['S']);

        /*
        Crouching does not stop him anymore, it just slows him to a shuffle,
        wich is what the crouchWalk sheet is for. Crouch beats run: you cannot
        sprint while ducked, so the shift check only applies standing up.
        */
        float moveSpeed = crouching ? crouchSpeed : (runDown ? runSpeed : walkSpeed);

        // decide wich direction to face, and keep facing it when we stop
        if (inputX < 0.0f) facingRight = false;
        else if (inputX > 0.0f) facingRight = true;

        /*
        Jumping. Space or W, and only when his feet are on something.

        The jumpHeld dance turns a key that is "down" for many frames into a
        single press: we jump only if it is down NOW and was NOT down last
        frame. Without it, holding the key would fire a new jump on the very
        frame he lands, forever.

        A jump is just setting the vertical speed to a negative number once.
        Nothing pushes him up after that; gravity below eats away at that
        speed every frame until it turns positive and he comes back down. The
        arc falls out of those two lines on its own.
        */
        bool jumpDown = keys[' '] || keys['w'] || keys['W'];
        if (jumpDown && !jumpHeld && onGround) velocityY = -jumpSpeed;
        jumpHeld = jumpDown;

        /*
        Gravity, every frame, no exceptions. Even standing still he is being
        pulled down a little, wich is exactly what keeps onGround true: he
        keeps pressing into the floor, so MoveAndCollide keeps stopping him
        and keeps reporting that he landed.

        The clamp is the anti tunneling rule from the header: never fall so
        fast in one frame that you could cross a whole floor without ever
        overlapping it.
        */
        velocityY += gravity * deltaTime;
        if (velocityY > maxFallSpeed) velocityY = maxFallSpeed;

        // direction * speed * time = distance moved this frame, but the
        // level gets a say in how much of it actually happens
        MoveAndCollide(inputX * moveSpeed * deltaTime,
                       velocityY * deltaTime,
                       colliders);

        /*
        Pick the clip. Being in the air beats everything: whatever his feet
        are doing, if there is nothing under them he is jumping or falling.

        On the ground, only HORIZONTAL movement counts as running. Checking
        vertical too would break now that gravity exists: he is always falling
        a tiny bit, so the run animation would play forever, even standing
        still.
        */
        /*
        Two independent choices: what the legs do, and what the torso does.
        They are picked seperately and never consult eachother, wich is what
        makes shooting while running or mid jump work without a single
        combined sheet.

        legsWanted is always a real clip. torsoWanted may be -1, meaning "draw
        nothing on top", wich is the case while crouching without firing,
        because the crouch sheets are whole bodies already.
        */
        int legsWanted;
        int torsoWanted;

        if (!onGround)
        {
            /*
            velocityY is negative while he is still rising and turns positive
            at the top of the arc, so that sign flip is exactly the moment a
            jump becomes a fall. The physics already knows, no timers needed.
            */
            bool rising = (velocityY < 0.0f);
            legsWanted  = rising ? jumpLegs  : fallLegs;
            torsoWanted = rising ? jumpTorso : fallTorso;

            shootOffsetX = 5.0f;   // legs are moving, same as walking
            shootOffsetY = -17.0f; // tucked up legs sit higher, so the torso does too

        }
        else if (crouching)
        {
            /*
            No entry animation, he drops straight into the crouch. Moving
            while down there swaps to the shuffle.
            */
            legsWanted = (inputX != 0.0f) ? crouchWalkClip : crouchLoopClip;

            // crouch art is whole body, so nothing ever overlays it
            torsoWanted = -1;
        }
        else if (inputX != 0.0f)
        {
            legsWanted  = runDown ? runLegs  : walkLegs;
            torsoWanted = runDown ? runTorso : walkTorso;

            shootOffsetX = 5.0f;   // on the move
            shootOffsetY = -11.0f;
        }
        else
        {
            legsWanted  = idleLegs;
            torsoWanted = idleTorso;

            shootOffsetX = 2.0f;   // stood still his stance is narrower
            shootOffsetY = -11.0f;
        }

        /*
        Firing. Holding F swaps in the shot animation, letting go swaps
        straight back; there is no wind down.

        Standing, that means replacing only the TORSO, so the legs carry on
        running or jumping underneath completely untouched. Crouching is
        different: crouchShootMarco is a complete sprite with head AND legs
        in it, so it goes on the legs layer and the torso is switched off,
        otherwise we would be drawing his upper half twice.

        Note this block still never asks what the legs are doing. Standing or
        airborne, shooting looks the same.
        */
        bool fireDown = keys['f'] || keys['F'];

        // letting go starts the wind down; pressing again cancels it
        if (fireDown) { shooting = true;  releasing = false; }
        else if (shooting) { shooting = false; releasing = true; }

        if (shooting || releasing)
        {
            int standing = shooting ? shootTorso : shootRelease;
            int ducked   = shooting ? crouchShootClip : crouchShootRelease;

            if (crouching)
            {
                legsWanted = ducked; // whole sprite, replaces both layers
                torsoWanted = -1;
            }
            else
            {
                torsoWanted = standing;
            }
        }

        legs.Play(legsWanted);
        legs.Update(deltaTime);

        if (torsoWanted >= 0)
        {
            torso.Play(torsoWanted);
            torso.Update(deltaTime);
        }

        /*
        The wind down is a one shot, so ask whichever layer is playing it
        whether it has reached the end. Crouched that is the legs layer,
        standing it is the torso; checking the wrong one would leave him
        stuck in the firing pose forever.
        */
        if (releasing && (crouching ? legs.IsFinished() : torso.IsFinished()))
        {
            releasing = false;
        }

        // remember for Draw(): -1 means the body sheet is the whole picture
        torsoShowing = torsoWanted;
    }

    /*
    This code:
    Where the player actually is, as a solid rectangle in world pixels.
    position is the top left of the SPRITE, so we shift by the hitbox offsets
    to get the smaller box that the level is allowed to stop.
    */
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
    This code:
    Moves the player and refuses to let him end up inside anything solid.

    The important idea: ONE AXIS AT A TIME.
    We move horizontally, fix any overlap we caused horizontally, and only
    then move vertically and fix that. Doing both at once and then trying to
    repair it is where collision code usually goes wrong, because once the
    boxes overlap you can no longer tell wich direction the player came from,
    so you cannot tell wich side to push him back out of. Moving one axis at
    a time means the answer is always known: if he was moving right, he must
    be pushed back to the left, full stop.

    This is also what lets you slide along a wall instead of sticking to it.
    Walking diagonally into a wall, the x move gets cancelled but the y move
    still goes through, so you slide along it, wich feels right.

    How the push out works:
    if he moved right, his right edge ended up past the wall's left edge, so
    we put his right edge exactly ON the wall's left edge. Because Overlaps()
    treats touching edges as NOT overlapping, that spot counts as free and he
    does not get detected as stuck next frame.

    We recompute GetBounds() inside the loop, because pushing him out of the
    first wall moves him, and the box we test against the second one has to
    be his NEW position, not the old one.

    Note this only ever stops him, it never moves him on its own. It is
    handed a distance and decides how much of it is allowed to happen; what
    produced that distance (keys, gravity, a jump) is none of its business.
    */
    void Player::MoveAndCollide(float moveX, float moveY, const List<Collider>& colliders)
    {
        // ---- horizontal ----
        position.x += moveX;

        if (moveX != 0.0f)
        {
            for (int i = 0; i < colliders.size(); ++i)
            {
                /*
                A one way platform is a floor, not a wall. It must never stop
                you sideways or you would snag on the lip of every platform
                you walk past, so they are skipped entirely here and only ever
                looked at on the vertical pass below.
                */
                if (colliders[i].oneWay) continue;

                const AABB& solid = colliders[i].box;
                AABB me = GetBounds();
                if (!Overlaps(me, solid)) continue;

                if (moveX > 0.0f) position.x = solid.Left() - hitboxOffsetX - hitboxWidth;
                else              position.x = solid.Right() - hitboxOffsetX;
            }
        }

        /*
        ---- vertical ----
        Same as above, but this axis also has to answer two questions for the
        jumping code: is he standing on something, and should his vertical
        speed be thrown away?

        We assume he is in the air and only prove otherwise, so onGround is
        false unless a downward move actually got stopped this frame. Walk off
        a ledge and nothing stops him, so it stays false and he cannot jump
        out of thin air.

        velocityY is zeroed on ANY vertical hit, in both directions. Landing:
        without it his speed would keep growing while he stands there, and the
        moment he stepped off a ledge he would drop like a stone. Ceiling: it
        stops him from sticking to it and hovering for the rest of the jump.
        */
        onGround = false;

        /*
        Where his feet were BEFORE this move. This single number is what makes
        one way platforms work, see the test below.
        */
        float previousBottom = GetBounds().Bottom();

        position.y += moveY;

        if (moveY != 0.0f)
        {
            for (int i = 0; i < colliders.size(); ++i)
            {
                const Collider& collider = colliders[i];
                const AABB& solid = collider.box;

                AABB me = GetBounds();
                if (!Overlaps(me, solid)) continue;

                /*
                The one way rule. Two tests, and failing either means he is
                not landing on this platform, so we ignore it completely.

                1. moveY <= 0 means he is rising. A platform never stops you
                   going up; that is the whole point, you jump straight
                   through it from underneath.

                2. previousBottom > solid.Top() means his feet were ALREADY
                   past the surface when the frame started, so he is inside
                   the platform or under it, not coming down onto it. Without
                   this he would get snapped on top the instant he jumped
                   through, instead of passing.

                Standing on one keeps working because we snap his feet exactly
                onto Top(), so next frame previousBottom == Top(), wich is not
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
                    onGround = true; // landed on top of something
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
        /*
        Line the clips up by their FEET, not their top corner.

        A sprite is drawn downwards from the position you give it, so two
        clips of different heights drawn at the same y end up with their feet
        in different places. Idle and run are 34 tall, the jump sheet is 64,
        so drawing them all at position.y would drop him 30 pixels through the
        floor the moment he jumped.

        position.y is the top of a NORMAL frame, so position.y +
        spriteBaseHeight is where his feet are. Subtracting the height of
        whatever clip is showing gives the top corner that puts those feet in
        the right spot, whatever size the art happens to be. Add a 96 tall
        death animation later and it lines up with no extra code.
        */
        float feetY = position.y + spriteBaseHeight;

        /*
        Every sheet is bottom aligned on his feet, whatever size it is. The
        artist drew each frame sitting on the bottom of its canvas, so "put
        the bottom of the frame at the feet" is all the vertical maths there
        is, and a 34 tall sheet lines up with a 64 tall one for free.

        Horizontally, most frames are 34 wide and land straight on position.x.
        The wide ones (51 for the firing sheets, 35 for the crouch walk) carry
        that extra width as padding on the GUN side. Mirror one of those and
        the padding ends up on the wrong side, pushing him sideways, so facing
        left we slide the frame back by however much wider than a body it is.
        */
        float legsGap = static_cast<float>(legs.GetFrameWidth()) - bodyFrameWidth;
        float legsX = facingRight ? position.x : position.x - legsGap;

        legs.Draw(target, renderer,
                  renderer.ToViewX(legsX),
                  renderer.ToViewY(feetY - static_cast<float>(legs.GetFrameHeight())),
                  !facingRight);

        if (torsoShowing < 0) return; // crouching, or crouch shooting: one sprite is all there is

        /*
        The torso on top. For the paired sheets this lands at exactly the same
        spot as the legs, because the merge tool's nudges are baked into the
        images themselves, so there is nothing left to correct.

        The firing torso is the exception: it is the one sheet that never went
        through that alignment, so it gets a hand tuned nudge. Mirror the X
        part of it too, otherwise the gun drifts the wrong way when he turns.
        */
        bool firing = (torsoShowing == shootTorso || torsoShowing == shootRelease);

        float torsoGap = static_cast<float>(torso.GetFrameWidth()) - bodyFrameWidth;
        float nudgeX = torsoOffsetX + (firing ? shootOffsetX : 0.0f);
        float nudgeY = firing ? shootOffsetY : 0.0f;

        float torsoX = facingRight ? (position.x + nudgeX)
                                   : (position.x - torsoGap - nudgeX);

        torso.Draw(target, renderer,
                   renderer.ToViewX(torsoX),
                   renderer.ToViewY(feetY - static_cast<float>(torso.GetFrameHeight()) + nudgeY),
                   !facingRight);
    }
}
