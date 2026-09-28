#pragma once

#include "common.h" // SCRWIDTH / SCRHEIGHT, so this header stands on its own

namespace Tmpl8
{
    class Surface;

    /*
    THE ONLY ZOOM SETTING: how many tiles you want to see across the screen.
    Smaller is more zoomed in.

    A tile count rather than a zoom factor, because a zoom factor only means
    something once you know the tile size. Swap to a 32x32 tileset and a fixed
    factor would show you half as much map; this way the map states its tile
    size and SetupZoom works the rest out.
    */
    static const int TILES_ON_SCREEN = 32;

    /*
    Owns everything about HOW things reach the screen: the zoom, the camera and
    the one blitter that writes pixels. Drawing a tile and drawing an animation
    frame are the same job (cell N of a grid), so they share DrawFrame.

    Three coordinate spaces, do not mix them:
    - world pixels : where something is on the map, can be far off screen
    - view pixels  : world pixels with the camera applied, zoom NOT applied.
                     This is what DrawFrame takes.
    - real pixels  : what lands in the screen buffer, view * pixelScale.

    Everything outside this class works in world and view pixels only, so no
    other file needs to know the zoom exists.
    */
    class RenderManager
    {
    public:
        // Call once, after the map is loaded and the tile size is known.
        // A tileWidth of 0 or less leaves the safe 1:1 defaults alone.
        void SetupZoom(int tileWidth);

        // Places the camera so a world position lands at anchorX/anchorY of the
        // view, as a fraction (0.5, 0.5 is dead centre). The anchor stays a
        // parameter because where the player sits on screen is a gameplay call.
        void FollowTarget(float worldX, float worldY, float anchorX, float anchorY);

        int GetPixelScale() const { return pixelScale; }
        int GetViewWidth() const { return viewWidth; }
        int GetViewHeight() const { return viewHeight; }

        int ToViewX(float worldX) const { return static_cast<int>(worldX + cameraX); }
        int ToViewY(float worldY) const { return static_cast<int>(worldY + cameraY); }

        // Rejects anything fully off screen before we touch a pixel. This is
        // what keeps a big map cheap: you only pay for tiles you can see.
        bool IsInView(int viewX, int viewY, int width, int height) const;

        /*
        The one and only blitter. 'sheet' is a grid of frameWidth x frameHeight
        cells, left to right then top to bottom, and frameIndex picks one. That
        covers a tileset (a real grid) and a spritesheet (a grid one row high).

        flipX/flipY mirror the cell, for Tiled's flip flags and for the player
        walking left.
        */
        void DrawFrame(Surface* target, Surface* sheet,
                       int frameWidth, int frameHeight, int frameIndex,
                       int viewX, int viewY,
                       bool flipX = false, bool flipY = false) const;

        // Hollow rectangle outline in view pixels, for seeing collision shapes
        // that are otherwise invisible.
        void DrawBox(Surface* target, int viewX, int viewY,
                     int width, int height, unsigned int color) const;

    private:
        /*
        Filled in by SetupZoom, do not set by hand. The 1:1 defaults matter: if
        no map loads we still draw something instead of dividing by zero.

        pixelScale -> real screen pixels per game pixel
        viewWidth  -> how much world fits across, in small world pixels
        */
        int pixelScale = 1;
        int viewWidth = SCRWIDTH;
        int viewHeight = SCRHEIGHT;

        // There is no camera object that moves; we add this offset to
        // everything we draw, so the world slides past instead.
        float cameraX = 0.0f;
        float cameraY = 0.0f;
    };
}
