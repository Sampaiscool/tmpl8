#pragma once

#include "common.h" // SCRWIDTH / SCRHEIGHT, so this header stands on its own

namespace Tmpl8
{
    /*
    This code:
    Forward declaration. We only use Surface as a POINTER in this header, and
    for a pointer the compiler does not need to know what is inside the class,
    only that the name exists. player.h does the same thing and for the same
    reason: the header stays usable no matter what order the .cpp files
    include things in.
    */
    class Surface;

    /*
    THE ONLY ZOOM SETTING. Change this one number, nothing else.

    It says how many tiles you want to see across the screen. Everything else
    (the zoom factor, the size of the visible area) is worked out from this
    plus the tile size that comes out of the Tiled JSON, in SetupZoom().

    Why this number instead of a zoom factor?
    Because a zoom factor only makes sense if you already know how big your
    tiles are. Swap to a tileset with 32x32 tiles instead of 16x16 and a fixed
    zoom would suddenly show you half as much map. This way the map tells us
    its tile size and the zoom adjusts itself, so you always see roughly this
    many tiles no matter what tileset the JSON uses.

    Smaller number = more zoomed in.
    */
    static const int TILES_ON_SCREEN = 32;

    /*
    This code:
    The RenderManager owns everything about HOW things end up on screen: the
    zoom, the camera, and the one blitter that actually writes pixels.

    Why this class exists:
    Game::BlitTile and Player::DrawSprite used to be two seperate copies of
    almost the same loop. Both walked a rectangle out of a source image, both
    skipped black pixels, both blew every pixel up into a block, both clipped
    against the screen. The only real difference was how they found the start
    of the rectangle. So they are now one function, DrawFrame, and a tile and
    an animation frame are both just "cell number N of a grid".

    Three coordinate spaces, do not mix them up:
    - world pixels : where something is on the map. Can be far off screen.
    - view pixels  : world pixels after the camera offset. Small pixels, the
                     zoom has NOT been applied yet. This is what you hand to
                     DrawFrame.
    - real pixels  : what actually lands in the screen buffer. DrawFrame
                     multiplies by pixelScale to get here.

    Everything outside this class works in world pixels and view pixels only,
    so no other file has to know the zoom exists.
    */
    class RenderManager
    {
    public:
        /*
        Works out pixelScale/viewWidth/viewHeight from the tile size that came
        out of the Tiled JSON. Call it once, after the map is loaded and the
        tile size is known. A tileWidth of 0 or less leaves the safe 1:1
        defaults alone.
        */
        void SetupZoom(int tileWidth);

        /*
        Puts the camera so that the given world position lands at anchorX /
        anchorY of the view, as a fraction: 0.5 / 0.5 is dead center, 0.1 puts
        it a tenth in from the left. The anchor stays a parameter because
        WHERE the player should sit on screen is a gameplay decision, not a
        rendering one, so game.cpp gets to pick it.
        */
        void FollowTarget(float worldX, float worldY, float anchorX, float anchorY);

        int GetPixelScale() const { return pixelScale; }
        int GetViewWidth() const { return viewWidth; }
        int GetViewHeight() const { return viewHeight; }

        // World pixels -> view pixels. Still small pixels, DrawFrame zooms.
        int ToViewX(float worldX) const { return static_cast<int>(worldX + cameraX); }
        int ToViewY(float worldY) const { return static_cast<int>(worldY + cameraY); }

        /*
        Is a width x height thing at this view position worth drawing at all?
        Checked against the VIEW size, not the real screen size, because view
        pixels are what we are given. Anything that fails this is skipped
        before we touch a single pixel, wich is the cheap way to keep a big
        map fast: we only pay for the tiles you can actually see.
        */
        bool IsInView(int viewX, int viewY, int width, int height) const;

        /*
        The one and only blitter.

        'sheet' is treated as a grid of frameWidth x frameHeight cells, laid
        out left to right, top to bottom. frameIndex picks a cell. That covers
        both cases we have: a tileset is a real grid, and a spritesheet is a
        grid that happens to be one row high.

        viewX/viewY are in view pixels; the multiply by pixelScale happens
        inside. flipX/flipY mirror the cell, for Tiled's flip flags and for
        the player walking left.
        */
        void DrawFrame(Surface* target, Surface* sheet,
                       int frameWidth, int frameHeight, int frameIndex,
                       int viewX, int viewY,
                       bool flipX = false, bool flipY = false) const;

    private:
        /*
        Filled in by SetupZoom(), do not set these by hand.

        pixelScale  = how many real screen pixels one game pixel becomes.
        viewWidth   = how much world we can see across, in small world pixels.
        viewHeight  = same vertically.

        The sane defaults matter: if a map never loads, we still draw at 1:1
        over the full screen instead of dividing by zero.
        */
        int pixelScale = 1;
        int viewWidth = SCRWIDTH;
        int viewHeight = SCRHEIGHT;

        /*
        The camera offset, in world pixels. There is no camera object that
        moves around: we just add this to everything we draw, so the world
        slides past instead. Set by FollowTarget().
        */
        float cameraX = 0.0f;
        float cameraY = 0.0f;
    };
}
