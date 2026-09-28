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
    Owns everything about HOW things reach the screen.

    The zoom works by drawing small and enlarging once at the end: everything
    is drawn 1:1 into a view surface of SCRWIDTH/pixelScale x
    SCRHEIGHT/pixelScale, and Present() blows that up onto the real screen
    with Surface::CopyToScaled. So nothing that draws has to know the zoom
    exists, and the blitter never multiplies a coordinate.

    Two coordinate spaces, do not mix them:
    - world pixels : where something is on the map, can be far off screen
    - view pixels  : world pixels with the camera applied. Everything drawn
                     goes here, and this is what the draw calls below take.
    */
    class RenderManager
    {
    public:
        // template.cpp never deletes the app, so Game has to call Unload();
        // the destructor is only a safety net. Same pattern as the others.
        ~RenderManager() { Unload(); }
        void Unload();

        // Works the zoom out from the map's tile size and creates the view
        // surface. Call once, after the map is loaded. A tileWidth of 0 or
        // less keeps the safe 1:1 defaults, so a failed map load still draws.
        void SetupZoom(int tileWidth);

        // Places the camera so a world position lands at anchorX/anchorY of
        // the view, as a fraction (0.5, 0.5 is dead centre). The anchor stays
        // a parameter because where the player sits on screen is a gameplay
        // call, not a rendering one.
        void FollowTarget(float worldX, float worldY, float anchorX, float anchorY);

        int GetPixelScale() const { return pixelScale; }
        int GetViewWidth() const { return viewWidth; }
        int GetViewHeight() const { return viewHeight; }

        int ToViewX(float worldX) const { return static_cast<int>(worldX + cameraX); }
        int ToViewY(float worldY) const { return static_cast<int>(worldY + cameraY); }

        // Rejects anything fully off screen before we touch a pixel. This is
        // what keeps a big map cheap: you only pay for tiles you can see.
        bool IsInView(int viewX, int viewY, int width, int height) const;

        void Clear(unsigned int color) const;

        /*
        The one and only blitter. 'sheet' is a grid of frameWidth x frameHeight
        cells, left to right then top to bottom, and frameIndex picks one. That
        covers a tileset (a real grid) and a spritesheet (a grid one row high).

        flipX/flipY mirror the cell, for Tiled's flip flags and for the player
        walking left.
        */
        void DrawFrame(Surface* sheet,
                       int frameWidth, int frameHeight, int frameIndex,
                       int viewX, int viewY,
                       bool flipX = false, bool flipY = false) const;

        // Hollow rectangle outline, for seeing collision shapes that are
        // otherwise invisible. One view pixel thick, so pixelScale thick once
        // it reaches the screen.
        void DrawBox(int viewX, int viewY, int width, int height,
                     unsigned int color) const;

        // Enlarges the view onto the real screen. Once per frame, last thing.
        void Present(Surface* screen) const;

    private:
        /*
        Everything is drawn into this, at one view pixel per game pixel. Owned
        here, created by SetupZoom, freed by Unload.
        */
        Surface* view = nullptr;

        /*
        Filled in by SetupZoom, do not set by hand. The 1:1 defaults matter: if
        no map loads we still draw something instead of dividing by zero.

        pixelScale -> real screen pixels per game pixel
        viewWidth  -> how much world fits across, in game pixels
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
