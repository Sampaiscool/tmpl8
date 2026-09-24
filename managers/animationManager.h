#pragma once

#include "list.h"

namespace Tmpl8
{
    class Surface;
    class RenderManager;

    /*
    This code:
    The AnimationManager owns a set of spritesheets and keeps track of wich
    one is playing and how far along it is.

    Why it exists:
    the Player used to carry all of this itself: two Sprite pointers, a third
    pointer aliasing one of them, a frame counter, a timer, a frame duration,
    two frame count constants and a PlayerState enum to tie them together.
    That is a lot of bookkeeping for "play the run animation", and every new
    animation (jump, shoot, die) meant another pointer, another constant and
    another enum value. Here you just AddClip once and Play the number you get
    back, so the Player only remembers its clip ids.

    Why Surface and not the template's Sprite:
    a Sprite is a Surface plus a frame count plus a per frame table of scanline
    start offsets that only Sprite::Draw uses. We draw through
    RenderManager::DrawFrame instead, wich needs nothing but the pixels and
    the frame size, so going straight to Surface skips an allocation and an
    indirection we would never read.

    Ownership:
    every clip owns its Surface, allocated with new in AddClip and freed in
    the destructor. A clip is only ever stored inside 'clips', so nothing else
    may delete it.
    */
    class AnimationManager
    {
    public:
        ~AnimationManager();

        /*
        Loads a spritesheet and registers it as a clip. The frames have to sit
        next to eachother on one horizontal strip, wich is how both of Marco's
        sheets are exported.

        frameDuration is how long ONE frame stays on screen, in seconds, so
        0.10 gives you a 10 fps animation.

        'loop' decides what happens at the end. A walk cycle loops forever;
        a one shot like a jump should hold its last frame instead, otherwise
        it snaps back to the crouch pose halfway through the arc and looks
        like he jumps twice.

        Returns the clip id you pass to Play() later, or -1 if the image could
        not be loaded.
        */
        int AddClip(const char* sheetPath, unsigned int frameCount,
                    float frameDuration, bool loop = true);

        /*
        Makes a new clip out of PART of a clip you already loaded, without
        touching the disk again. Used for the fall animation, wich is just the
        last couple of frames of the jump sheet.

        The new clip SHARES the source clip's image instead of copying it, so
        the png is only in memory once. That means it must not free it, hence
        the ownsSheet flag on the struct below: whoever loaded the image is
        the one who deletes it, and a sub clip never does.

        firstFrame is counted in the SOURCE sheet, so (4, 2) on a six frame
        sheet gives you frames 4 and 5. Returns -1 if the range does not fit.
        */
        int AddSubClip(int sourceClip, unsigned int firstFrame,
                       unsigned int frameCount, float frameDuration, bool loop = true);

        /*
        Size of ONE frame of the clip that is playing right now. The Player
        needs the height because not every clip is the same size: the jump
        sheet is 64 tall where idle and run are 34, and a sprite is drawn from
        its top left corner, so without knowing the height you cannot line up
        the feet. Returns 0 when nothing is playing.
        */
        int GetFrameWidth() const;
        int GetFrameHeight() const;

        /*
        True when a ONE SHOT clip has reached its last frame and is sitting
        on it. A looping clip is never finished, so this is always false for
        those. The torso layer uses it to know when the "lower the gun"
        animation has played out and it can stop drawing itself.
        */
        bool IsFinished() const;

        /*
        Switches to a clip. Playing the clip that is already running does
        nothing at all, so you can safely call this every frame. Switching to
        a different one restarts it at frame 0 with a fresh timer, otherwise
        the new animation would continue halfway through the old one's cycle
        and look like a glitch.
        */
        void Play(int clip);

        // Steps the frame timer forward. Call once per frame with delta time.
        void Update(float deltaTime);

        /*
        Draws the frame that is currently showing at the given VIEW position
        (world pixels with the camera already applied). 'flipX' mirrors it,
        for a character walking the other way.
        */
        void Draw(Surface* target, const RenderManager& renderer,
                  int viewX, int viewY, bool flipX) const;

    private:
        /*
        One animation. frameWidth/frameHeight are worked out in AddClip by
        dividing the sheet up, so we never have to divide again while drawing.
        */
        struct AnimationClip
        {
            Surface* sheet = nullptr;   // freed in the destructor, but only if ownsSheet
            bool ownsSheet = true;      // false for a sub clip borrowing someone elses image
            unsigned int firstFrame = 0;// where this clip starts inside the sheet
            int frameWidth = 0;         // width of ONE frame, not the sheet
            int frameHeight = 0;
            unsigned int frameCount = 1;
            float frameDuration = 0.1f; // seconds per frame
            bool loop = true;           // false = stop on the last frame
        };

        List<AnimationClip> clips;

        int currentClip = -1;         // -1 means nothing is playing yet
        unsigned int currentFrame = 0;// wich frame of the sheet we show now
        float animTimer = 0.0f;       // counts up until it passes frameDuration
    };
}
