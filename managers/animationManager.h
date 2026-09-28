#pragma once

#include "list.h"

namespace Tmpl8
{
    class Surface;
    class RenderManager;

    /*
    Owns a set of spritesheets and keeps track of which one is playing and how
    far along it is. You AddClip once and Play the id you get back, so an actor
    only has to remember its clip ids.

    One instance is one layer. The Player runs two of them, legs and torso, so
    what his legs do and what his arms do never touch eachother.

    Why Surface and not the template's Sprite: Sprite has no frame timer, no
    looping, no flipping and its scaled draw is unclipped, so none of the work
    it does for us is work we need. We draw through RenderManager::DrawFrame,
    which only wants the pixels and the frame size.

    Every clip owns its Surface unless it borrowed one from another clip.
    */
    class AnimationManager
    {
    public:
        // Same pattern as MissionManager: the destructor is the safety net,
        // but template.cpp never deletes the app, so whoever owns us has to
        // call Unload() at a moment of their choosing or the sheets leak.
        ~AnimationManager() { Unload(); }
        void Unload();

        /*
        Loads a spritesheet whose frames sit next to eachother on one
        horizontal strip. frameDuration is seconds per frame, so 0.10 is 10fps.

        loop false makes it a one shot: it holds the last frame instead of
        starting over, which is what a jump needs.

        Returns the clip id for Play(), or -1 if the sheet could not be used.
        */
        int AddClip(const char* sheetPath, unsigned int frameCount,
                    float frameDuration, bool loop = true);

        /*
        Makes a clip out of part of a clip already loaded, without reading the
        disk again: the fall animation is just the last frames of the jump
        sheet. The new clip SHARES the source image and must not free it.

        firstFrame counts in the source sheet. Returns -1 if the range does not
        fit.
        */
        int AddSubClip(int sourceClip, unsigned int firstFrame,
                       unsigned int frameCount, float frameDuration, bool loop = true);

        // Size of one frame of whatever is playing, 0 if nothing is. Clips are
        // not all the same size (jump is 64 tall, idle 34), and the Player
        // needs the height to line the feet up.
        int GetFrameWidth() const;
        int GetFrameHeight() const;

        // True once a ONE SHOT clip is sitting on its last frame. A looping
        // clip is never finished.
        bool IsFinished() const;

        // Safe to call every frame: playing the clip that is already running
        // does nothing, switching to another restarts it at frame 0.
        void Play(int clip);

        void Update(float deltaTime);

        // viewX/viewY are view pixels, camera applied but not the zoom.
        void Draw(Surface* target, const RenderManager& renderer,
                  int viewX, int viewY, bool flipX) const;

    private:
        struct AnimationClip
        {
            Surface* sheet = nullptr;
            bool ownsSheet = true;       // false for a sub clip borrowing an image
            unsigned int firstFrame = 0; // where this clip starts in the sheet
            int frameWidth = 0;          // one frame, not the whole sheet
            int frameHeight = 0;
            unsigned int frameCount = 1;
            float frameDuration = 0.1f;
            bool loop = true;
        };

        List<AnimationClip> clips;

        int currentClip = -1;
        unsigned int currentFrame = 0;
        float animTimer = 0.0f;
    };
}
