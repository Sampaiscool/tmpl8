#include "precomp.h"
#include "animationManager.h"
#include "renderManager.h"

namespace Tmpl8
{
    AnimationManager::~AnimationManager()
    {
        // Sub clips share the image of the clip they were cut from, so only
        // free through the clip that actually loaded it.
        for (AnimationClip& clip : clips)
        {
            if (clip.ownsSheet) delete clip.sheet;
            clip.sheet = nullptr;
        }
        clips.clear();
    }

    /*
    All frames sit on one horizontal strip, so the sheet is frameCount frames
    wide and one frame tall. Working the frame size out once here saves
    dividing on every draw.

    A missing file already stops the program inside Surface, so the only case
    left to guard is a sheet that loaded but is unusable.
    */
    int AnimationManager::AddClip(const char* sheetPath, unsigned int frameCount,
                                  float frameDuration, bool loop)
    {
        if (frameCount == 0) return -1;

        AnimationClip clip;
        clip.sheet = new Surface(sheetPath);

        if (clip.sheet->width == 0)
        {
            delete clip.sheet;
            return -1;
        }

        clip.frameWidth = clip.sheet->width / static_cast<int>(frameCount);
        clip.frameHeight = clip.sheet->height;
        clip.frameCount = frameCount;
        clip.frameDuration = frameDuration;
        clip.loop = loop;

        clips.push_back(clip);

        // The id is the position in the list; clips are never removed.
        const int id = clips.size() - 1;

        // First clip added starts playing, so there is always something to draw.
        if (currentClip < 0) Play(id);

        return id;
    }

    /*
    Copies the frame size and the image POINTER from the source and marks
    itself as not owning it. Everything else behaves like a normal clip that
    happens to be shorter.
    */
    int AnimationManager::AddSubClip(int sourceClip, unsigned int firstFrame,
                                     unsigned int frameCount, float frameDuration, bool loop)
    {
        if (sourceClip < 0 || sourceClip >= clips.size()) return -1;
        if (frameCount == 0) return -1;

        const AnimationClip& source = clips[sourceClip];
        if (firstFrame + frameCount > source.firstFrame + source.frameCount) return -1;

        AnimationClip clip;
        clip.sheet = source.sheet;
        clip.ownsSheet = false;
        clip.frameWidth = source.frameWidth;
        clip.frameHeight = source.frameHeight;
        clip.firstFrame = firstFrame;
        clip.frameCount = frameCount;
        clip.frameDuration = frameDuration;
        clip.loop = loop;

        clips.push_back(clip);
        return clips.size() - 1;
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
    Subtracting frameDuration rather than zeroing the timer keeps the leftover
    time, so the animation does not slowly drift. The while loop matters after
    a stall, when one frame can be worth several animation frames.
    */
    void AnimationManager::Update(float deltaTime)
    {
        if (currentClip < 0) return;

        const AnimationClip& clip = clips[currentClip];
        if (clip.frameDuration <= 0.0f) return; // would loop forever

        animTimer += deltaTime;
        while (animTimer >= clip.frameDuration)
        {
            animTimer -= clip.frameDuration;

            if (clip.loop)
            {
                currentFrame = (currentFrame + 1) % clip.frameCount;
                continue;
            }

            // one shot: walk to the last frame, then freeze on it
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

    bool AnimationManager::IsFinished() const
    {
        if (currentClip < 0) return true;

        const AnimationClip& clip = clips[currentClip];
        if (clip.loop) return false;

        return currentFrame + 1 >= clip.frameCount;
    }

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

        // currentFrame counts from 0 inside THIS clip; firstFrame shifts it to
        // where the clip starts on the shared sheet.
        renderer.DrawFrame(target, clip.sheet, clip.frameWidth, clip.frameHeight,
                           static_cast<int>(clip.firstFrame + currentFrame),
                           viewX, viewY, flipX, false);
    }
}
