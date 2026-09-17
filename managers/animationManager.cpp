#include "precomp.h"
#include "animationManager.h"
#include "renderManager.h"
#include <iostream>

namespace Tmpl8
{
    /*
    This code:
    Gives back the heap memory of every spritesheet we loaded. These are
    legacy raw pointers so there is no unique_ptr doing it for us; without
    this the pixel buffers would stay allocated for the rest of the program.

    clips.clear() afterwards is not strictly needed since the List dies right
    after us, but it makes a dangling pointer impossible.
    */
    AnimationManager::~AnimationManager()
    {
        for (int i = 0; i < clips.size(); ++i)
        {
            delete clips[i].sheet;
            clips[i].sheet = nullptr;
        }
        clips.clear();
    }

    /*
    This code:
    Loads one spritesheet and works out the size of a single frame.

    All frames sit next to eachother on one long horizontal image, so the
    sheet is frameCount frames wide and 1 frame tall. That means one frame is
    (sheet width / frameCount) wide and the full sheet height tall. We store
    that once here instead of dividing again on every draw.

    The guards: a frameCount of 0 would divide by zero, and a sheet that
    failed to load has width 0, wich would give us a frame of 0 pixels that
    draws nothing while looking like it worked. Both give back -1 so the
    caller knows the clip does not exist.
    */
    int AnimationManager::AddClip(const char* sheetPath, unsigned int frameCount, float frameDuration)
    {
        if (frameCount == 0) return -1;

        AnimationClip clip;
        clip.sheet = new Surface(sheetPath);

        if (clip.sheet->width == 0)
        {
            std::cout << "ERROR: failed to load spritesheet at " << sheetPath << std::endl;
            delete clip.sheet;
            return -1;
        }

        clip.frameWidth = clip.sheet->width / static_cast<int>(frameCount);
        clip.frameHeight = clip.sheet->height;
        clip.frameCount = frameCount;
        clip.frameDuration = frameDuration;

        clips.push_back(clip);

        // The id is just the position in the list, and clips are never removed
        int id = clips.size() - 1;

        // First clip added becomes the one playing, so there is always something to draw
        if (currentClip < 0) Play(id);

        return id;
    }

    void AnimationManager::Play(int clip)
    {
        if (clip < 0 || clip >= clips.size()) return;
        if (clip == currentClip) return; // already running, leave the timer alone

        currentClip = clip;
        currentFrame = 0;
        animTimer = 0.0f;
    }

    /*
    This code:
    Animation timing. We add the frame time to a small accumulator; as soon as
    it passes frameDuration we advance one frame and SUBTRACT frameDuration
    instead of setting the timer to 0. That way we keep the leftover time and
    the animation does not slowly drift out of sync.

    The while loop instead of an if: after a stall one frame can be worth
    several animation frames, and an if would only ever advance by one and
    leave the rest of the time sitting in the accumulator.

    The modulo (%) makes the frame counter wrap back to 0, so the animation
    loops forever.
    */
    void AnimationManager::Update(float deltaTime)
    {
        if (currentClip < 0) return;

        const AnimationClip& clip = clips[currentClip];
        if (clip.frameDuration <= 0.0f) return; // a duration of 0 would loop forever

        animTimer += deltaTime;
        while (animTimer >= clip.frameDuration)
        {
            animTimer -= clip.frameDuration;
            currentFrame = (currentFrame + 1) % clip.frameCount;
        }
    }

    void AnimationManager::Draw(Surface* target, const RenderManager& renderer,
                                int viewX, int viewY, bool flipX) const
    {
        if (currentClip < 0) return;

        const AnimationClip& clip = clips[currentClip];
        renderer.DrawFrame(target, clip.sheet, clip.frameWidth, clip.frameHeight,
                           static_cast<int>(currentFrame), viewX, viewY, flipX, false);
    }
}
