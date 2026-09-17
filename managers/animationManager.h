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

        Returns the clip id you pass to Play() later, or -1 if the image could
        not be loaded.
        */
        int AddClip(const char* sheetPath, unsigned int frameCount, float frameDuration);

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
            Surface* sheet = nullptr;   // owned, freed in the destructor
            int frameWidth = 0;         // width of ONE frame, not the sheet
            int frameHeight = 0;
            unsigned int frameCount = 1;
            float frameDuration = 0.1f; // seconds per frame
        };

        List<AnimationClip> clips;

        int currentClip = -1;         // -1 means nothing is playing yet
        unsigned int currentFrame = 0;// wich frame of the sheet we show now
        float animTimer = 0.0f;       // counts up until it passes frameDuration
    };
}
