#pragma once

#include "common.h" // SCRWIDTH / SCRHEIGHT
#include "aabb.h"

namespace Tmpl8
{
    class Surface;

    // The only zoom setting: how many tiles to show across the screen.
    static const int TILES_ON_SCREEN = 32;

    // Owns how things reach the screen. Everything draws 1:1 into a small view
    // surface, and Present() enlarges that once at the end, so nothing that
    // draws has to know the zoom exists.
    //
    // Two coordinate spaces, do not mix them:
    //   world pixels - where something is on the map, can be far off screen
    //   view pixels  - world pixels with the camera applied; what the draw
    //                  calls below take
    class RenderManager
    {
    public:
        ~RenderManager() { Unload(); } // safety net; Game calls Unload by hand
        void Unload();

        // Call once, after the map is loaded. A tileWidth of 0 or less keeps
        // the 1:1 defaults, so a failed map load still draws.
        void SetupZoom(int tileWidth);

        // Puts worldX at anchorX of the view (0.5 is dead centre). Horizontal
        // only, the anchor is a parameter because it is a gameplay call.
        void FollowTarget(float worldX, float anchorX);

        // Parks worldY at anchorY of the view and leaves it there. Call once,
        // after SetupZoom. The camera does not follow Y: the level fits the
        // view, so following would just swing the world about when he jumps.
        void SetVerticalAnchor(float worldY, float anchorY);

        int ToViewX(float worldX) const { return static_cast<int>(worldX + cameraX); }
        int ToViewY(float worldY) const { return static_cast<int>(worldY + cameraY); }

        // Rejects anything fully off screen before we touch a pixel.
        bool IsInView(int viewX, int viewY, int width, int height) const;

        void Clear(unsigned int color) const;

        // The one and only blitter. 'sheet' is a grid of frameWidth x
        // frameHeight cells and frameIndex picks one, which covers a tileset
        // and a spritesheet (a grid one row high).
        void DrawFrame(Surface* sheet,
                       int frameWidth, int frameHeight, int frameIndex,
                       int viewX, int viewY,
                       bool flipX = false, bool flipY = false) const;

        // Hollow outline, for seeing collision shapes.
        void DrawBox(int viewX, int viewY, int width, int height,
                     unsigned int color) const;

        // The same for a box in world pixels.
        void DrawWorldBox(const AABB& box, unsigned int color) const
        {
            DrawBox(ToViewX(box.x), ToViewY(box.y),
                    static_cast<int>(box.w), static_cast<int>(box.h), color);
        }

        // Enlarges the view onto the real screen. Once per frame, last thing.
        void Present(Surface* screen) const;

    private:
        Surface* view = nullptr; // everything is drawn here, 1:1

        // Filled in by SetupZoom. The 1:1 defaults matter: if no map loads we
        // still draw something instead of dividing by zero.
        int pixelScale = 1;
        int viewWidth = SCRWIDTH;
        int viewHeight = SCRHEIGHT;

        // No camera object moves; these are added to everything drawn, so the
        // world slides past instead.
        float cameraX = 0.0f;
        float cameraY = 0.0f;
    };
}
