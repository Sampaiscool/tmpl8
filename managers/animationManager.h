#pragma once

#include "list.h"

namespace Tmpl8
{
    class Surface;
    class RenderManager;

    // Which clip is running, which frame, and how long it has been up. Plain
    // data with no pointers, so a List can copy it and one set of sheets can
    // drive any number of actors, each on its own frame.
    struct AnimationPlayer
    {
        int clip = -1;
        unsigned int frame = 0;
        float timer = 0.0f;
    };

    // Owns a set of spritesheets. AddClip once, then Play the id you get back.
    // One instance is one layer: the Player runs two, legs and torso.
    // Every call takes the caller's playhead and is const.
    class AnimationManager
    {
    public:
        // template.cpp never deletes the app, so the owner calls Unload();
        // the destructor is only a safety net.
        ~AnimationManager() { Unload(); }
        void Unload();

        // frameDuration is seconds per frame (0.10 = 10fps). loop false makes
        // a one shot that holds its last frame. Returns -1 if unusable.
        int AddClip(const char* sheetPath, unsigned int frameCount,
                    float frameDuration, bool loop = true);

        // Part of a clip already loaded, without reading the disk again.
        // shares the source image.
        int AddSubClip(int sourceClip, unsigned int firstFrame,
                       unsigned int frameCount, float frameDuration, bool loop = true);

        // Safe every frame: replaying the running clip does nothing.
        void Play(AnimationPlayer& player, int clip) const;

        void Update(AnimationPlayer& player, float deltaTime) const;

        // True once a one shot sits on its last frame. A loop never finishes.
        bool IsFinished(const AnimationPlayer& player) const;

        // viewX/viewY are view pixels: camera applied, no zoom.
        void Draw(const AnimationPlayer& player, const RenderManager& renderer,
                  int viewX, int viewY, bool flipX) const;

        // Clips are not all the same size (jump is 64 tall, idle 34).
        int GetFrameWidth(int clip) const;
        int GetFrameHeight(int clip) const;

        // Is this pixel of the current frame solid? x/y are from the sheet's top
        // left corner as drawn. The colour key is pure black, same as DrawFrame.
        bool IsOpaqueAt(const AnimationPlayer& player, int x, int y, bool flipX) const;

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
    };
}
