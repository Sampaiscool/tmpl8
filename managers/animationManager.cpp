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
        /*
        Only delete an image the clip actually OWNS. Sub clips share the
        image of the clip they were cut from, so deleting through them would
        free the same block twice, wich is a crash.
        */
        for (int i = 0; i < clips.size(); ++i)
        {
            if (clips[i].ownsSheet) delete clips[i].sheet;
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
    int AnimationManager::AddClip(const char* sheetPath, unsigned int frameCount,
                                  float frameDuration, bool loop)
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
        clip.loop = loop;

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

            if (clip.loop)
            {
                // wrap back to 0 and keep cycling forever
                currentFrame = (currentFrame + 1) % clip.frameCount;
                continue;
            }

            // one shot: advance until the last frame, then freeze on it
            if (currentFrame + 1 < clip.frameCount)
            {
                ++currentFrame;
            }
            else
            {
                animTimer = 0.0f; // stop the timer running away while we hold
                break;
            }
        }
    }

    /*
    This code:
    Cuts a new clip out of a clip that is already loaded.

    It copies the frame size and the image POINTER from the source, marks
    itself as not owning that image, and remembers where in the sheet it
    starts. Everything else (the timer, the looping) works exactly like a
    normal clip, because as far as Update and Draw are concerned this is just
    another clip that happens to be shorter.

    The range check matters: asking for frames 4..7 of a six frame sheet would
    read past the end of the image and draw garbage, so we refuse instead.
    */
    int AnimationManager::AddSubClip(int sourceClip, unsigned int firstFrame,
                                     unsigned int frameCount, float frameDuration, bool loop)
    {
        if (sourceClip < 0 || sourceClip >= clips.size()) return -1;
        if (frameCount == 0) return -1;

        const AnimationClip& source = clips[sourceClip];

        // the requested range has to fit inside the source clip
        if (firstFrame + frameCount > source.firstFrame + source.frameCount) return -1;

        AnimationClip clip;
        clip.sheet = source.sheet;   // shared, NOT copied
        clip.ownsSheet = false;      // so the destructor leaves it alone
        clip.frameWidth = source.frameWidth;
        clip.frameHeight = source.frameHeight;
        clip.firstFrame = firstFrame;
        clip.frameCount = frameCount;
        clip.frameDuration = frameDuration;
        clip.loop = loop;

        clips.push_back(clip);
        return clips.size() - 1;
    }

    /*
    A looping clip runs forever so it is never "finished". A one shot clip is
    finished once currentFrame has reached the last index, because Update
    freezes it there instead of wrapping around.
    */
    bool AnimationManager::IsFinished() const
    {
        if (currentClip < 0) return true;

        const AnimationClip& clip = clips[currentClip];
        if (clip.loop) return false;

        return currentFrame + 1 >= clip.frameCount;
    }

    // Size of one frame of whatever is playing, 0 if nothing is
    int AnimationManager::GetFrameWidth() const
    {
        return (currentClip < 0) ? 0 : clips[currentClip].frameWidth;
    }

    int AnimationManager::GetFrameHeight() const
    {
        return (currentClip < 0) ? 0 : clips[currentClip].frameHeight;
    }

    void AnimationManager::Draw(Surface* target, const RenderManager& renderer,
                                int viewX, int viewY, bool flipX) const
    {
        if (currentClip < 0) return;

        const AnimationClip& clip = clips[currentClip];
        // currentFrame counts from 0 inside THIS clip, firstFrame shifts it
        // to where the clip actually starts on the shared sheet.
        renderer.DrawFrame(target, clip.sheet, clip.frameWidth, clip.frameHeight,
                           static_cast<int>(clip.firstFrame + currentFrame),
                           viewX, viewY, flipX, false);
    }
}
