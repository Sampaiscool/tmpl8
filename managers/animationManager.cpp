#include "precomp.h"
#include "animationManager.h"
#include "renderManager.h"
#include "surface.h"

namespace Tmpl8
{
    void AnimationManager::Unload()
    {
        // Sub clips share an image, so only free through the owner.
        for (AnimationClip& clip : clips)
        {
            if (clip.ownsSheet) delete clip.sheet;
            clip.sheet = nullptr;
        }
        clips.clear();
    }

    // All frames sit on one horizontal strip. A missing file already stops the
    // program inside Surface, so this only guards an unusable sheet.
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

        return clips.size() - 1;
    }

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

    void AnimationManager::Play(AnimationPlayer& player, int clip) const
    {
        if (clip < 0 || clip >= clips.size()) return;
        if (clip == player.clip) return; // already running, leave the timer alone

        player.clip = clip;
        player.frame = 0;
        player.timer = 0.0f;
    }

    // Subtracting rather than zeroing keeps the leftover time, so it does not
    // drift. The while loop matters after a stall.
    void AnimationManager::Update(AnimationPlayer& player, float deltaTime) const
    {
        if (player.clip < 0 || player.clip >= clips.size()) return;

        const AnimationClip& clip = clips[player.clip];
        if (clip.frameDuration <= 0.0f) return; // would loop forever

        player.timer += deltaTime;
        while (player.timer >= clip.frameDuration)
        {
            player.timer -= clip.frameDuration;

            if (clip.loop)
            {
                player.frame = (player.frame + 1) % clip.frameCount;
                continue;
            }

            // One shot: walk to the last frame, then freeze on it.
            if (player.frame + 1 < clip.frameCount)
            {
                ++player.frame;
            }
            else
            {
                player.timer = 0.0f; // do not let it run away while we hold
                break;
            }
        }
    }

    bool AnimationManager::IsFinished(const AnimationPlayer& player) const
    {
        if (player.clip < 0 || player.clip >= clips.size()) return true;

        const AnimationClip& clip = clips[player.clip];
        if (clip.loop) return false;

        return player.frame + 1 >= clip.frameCount;
    }

    int AnimationManager::GetFrameWidth(int clip) const
    {
        if (clip < 0 || clip >= clips.size()) return 0;
        return clips[clip].frameWidth;
    }

    int AnimationManager::GetFrameHeight(int clip) const
    {
        if (clip < 0 || clip >= clips.size()) return 0;
        return clips[clip].frameHeight;
    }

    void AnimationManager::Draw(const AnimationPlayer& player, const RenderManager& renderer,
                                int viewX, int viewY, bool flipX) const
    {
        if (player.clip < 0 || player.clip >= clips.size()) return;

        const AnimationClip& clip = clips[player.clip];

        // frame counts from 0 in this clip, firstFrame shifts it onto the sheet.
        renderer.DrawFrame(clip.sheet, clip.frameWidth, clip.frameHeight,
                           static_cast<int>(clip.firstFrame + player.frame),
                           viewX, viewY, flipX, false);
    }

    bool AnimationManager::IsOpaqueAt(const AnimationPlayer& player, int x, int y, bool flipX) const
    {
        if (player.clip < 0 || player.clip >= clips.size()) return false;

        const AnimationClip& clip = clips[player.clip];
        if (!clip.sheet) return false;

        // Bounds check against frame dimensions
        if (x < 0 || x >= clip.frameWidth || y < 0 || y >= clip.frameHeight) return false;

        // Handle horizontal flipping
        int frameX = flipX ? (clip.frameWidth - 1 - x) : x;

        // Determine columns in spritesheet
        int cols = clip.sheet->width / clip.frameWidth;
        if (cols <= 0) return false;

        int absFrame = static_cast<int>(clip.firstFrame + player.frame);
        int sheetX = (absFrame % cols) * clip.frameWidth + frameX;
        int sheetY = (absFrame / cols) * clip.frameHeight + y;

        if (sheetX < 0 || sheetX >= clip.sheet->width || sheetY < 0 || sheetY >= clip.sheet->height)
            return false;

        // Visual check: surface stores pixels in 'pixels' array as unsigned int / Pixel type.
        // Pure black (0x000000) is the color key / background.
        unsigned int c = clip.sheet->pixels[sheetX + sheetY * clip.sheet->width];
        return (c & 0xFFFFFF) != 0;
    }
  }

