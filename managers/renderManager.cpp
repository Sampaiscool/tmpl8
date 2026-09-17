#include "precomp.h"
#include "renderManager.h"
#include <iostream>

namespace Tmpl8
{
    /*
    This code:
    Decides how far to zoom in, using the tile size that came out of the
    Tiled JSON. This is the only place the zoom is calculated.

    The idea:
    You say how many tiles you want to see (TILES_ON_SCREEN in
    renderManager.h), the map says how many pixels a tile is, so the width we
    want to show is TILES_ON_SCREEN * tileWidth game pixels. Divide the real
    screen width by that and you get how many real pixels each game pixel
    should become.

    With the current map: 1280 / (32 * 16) = 1280 / 512 = 2.
    Load a map with 32x32 tiles and it becomes 1280 / 1024 = 1, so you still
    see about the same amount of world instead of everything doubling.

    Why integer division:
    A fractional scale like 2.5 would make some pixels 2 real pixels wide and
    others 3, wich looks uneven and wobbly on pixel art. Whole numbers keep
    every game pixel exactly the same size.

    The clamp to 1 matters: with very big tiles or very many of them the
    division can come out 0, and a scale of 0 would make everything vanish
    and divide by zero below.
    */
    void RenderManager::SetupZoom(int tileWidth)
    {
        if (tileWidth <= 0) return; // no map, keep the 1:1 defaults

        pixelScale = SCRWIDTH / (TILES_ON_SCREEN * tileWidth);
        if (pixelScale < 1) pixelScale = 1;

        // How much world fits on screen once everything is that many times bigger
        viewWidth = SCRWIDTH / pixelScale;
        viewHeight = SCRHEIGHT / pixelScale;

        std::cout << "Zoom: tiles are " << tileWidth << " px"
                  << ", scale " << pixelScale << "x"
                  << ", showing " << (viewWidth / tileWidth) << " tiles across"
                  << std::endl;
    }

    /*
    This code:
    Moves the camera so the target sits at the wanted spot on screen.

    Substitute this offset into ToViewX and the target's own x cancels out,
    so it always lands on anchorX * viewWidth no matter where it is in the
    world. viewWidth/viewHeight and not SCRWIDTH/SCRHEIGHT, because the
    camera works in small world pixels: the zoom to real pixels only happens
    at the very end, inside DrawFrame.
    */
    void RenderManager::FollowTarget(float worldX, float worldY, float anchorX, float anchorY)
    {
        cameraX = (viewWidth * anchorX) - worldX;
        cameraY = (viewHeight * anchorY) - worldY;
    }

    /*
    This code:
    Rejects anything that is completely off screen. We allow one full
    width/height of slack on the negative side, because something at
    viewX = -10 still has most of its right half poking into the view and has
    to be drawn.
    */
    bool RenderManager::IsInView(int viewX, int viewY, int width, int height) const
    {
        return viewX > -width && viewX < viewWidth &&
               viewY > -height && viewY < viewHeight;
    }

    /*
    This code:
    Copies one cell out of a sheet into the screen buffer, mirrored if asked
    and blown up by pixelScale. This replaces both the old Game::BlitTile and
    the old Player::DrawSprite, wich were two copies of this same loop.

    Finding the cell:
    2D pixels live in RAM as one long 1D array, row after row, so the address
    of a pixel is always Y * Width + X. Dividing the sheet size by the frame
    size gives us how many cells fit across and down, and from the cell number
    we get its top left corner with the same formula in reverse:
    srcX = (frameIndex % columns) * frameWidth
    srcY = (frameIndex / columns) * frameHeight

    A spritesheet with all frames next to eachother is just the special case
    where columns equals the frame count and there is only one row, so it
    needs no seperate code path.

    Flipping:
    Instead of reading source pixel x we read (frameWidth - 1 - x), wich walks
    the row backwards and mirrors it. Same idea for y. That is how a tile the
    designer mirrored in Tiled and how the player walking left both work,
    without a second flipped image on disk.

    The zoom:
    Every source pixel is written as a pixelScale x pixelScale square of
    identical pixels, so a 16x16 tile covers 32x32 real pixels at scale 2. No
    blending or filtering, we just repeat the pixel, so the art stays crisp
    and blocky instead of blurry.

    Boundary protection & clipping:
    Every write is checked against the target size. Writing outside the buffer
    would corrupt memory or crash, so out of range pixels are simply skipped.
    The checks sit inside the block loops, because a block can hang half off
    the edge of the screen.

    Color & transparency:
    We mask off the alpha channel (c & 0xFFFFFF) to look at the pure RGB. Pure
    black counts as transparent here and gets skipped. Every pixel we do keep
    gets its top 8 bits forced to max (c | 0xFF000000) so it is drawn fully
    opaque.
    */
    void RenderManager::DrawFrame(Surface* target, Surface* sheet,
                                  int frameWidth, int frameHeight, int frameIndex,
                                  int viewX, int viewY,
                                  bool flipX, bool flipY) const
    {
        if (!target || !sheet || !sheet->pixels) return;
        if (frameWidth <= 0 || frameHeight <= 0) return;

        // How many cells fit on the sheet, so we can turn frameIndex into x/y
        int columns = sheet->width / frameWidth;
        int rows = sheet->height / frameHeight;
        if (columns <= 0 || rows <= 0) return;
        if (frameIndex < 0 || frameIndex >= columns * rows) return;

        // Cell number -> top left corner of that cell, in source pixels
        int srcX = (frameIndex % columns) * frameWidth;
        int srcY = (frameIndex / columns) * frameHeight;

        uint* src = sheet->pixels;
        uint* dst = target->pixels;

        for (int y = 0; y < frameHeight; ++y)
        {
            // Walk the source rows backwards when flipped vertically
            int sy = srcY + (flipY ? (frameHeight - 1 - y) : y);
            if (sy < 0 || sy >= sheet->height) continue; // source Y bound check

            for (int x = 0; x < frameWidth; ++x)
            {
                // Walk the source row backwards when flipped horizontally
                int sx = srcX + (flipX ? (frameWidth - 1 - x) : x);
                if (sx < 0 || sx >= sheet->width) continue; // source X bound check

                // 2D -> 1D offset formula: Y * Width + X
                uint c = src[sy * sheet->width + sx];

                // Skip black/transparent background pixels
                if ((c & 0xFFFFFF) == 0) continue;
                c |= 0xFF000000; // force alpha to 255 so the pixel is fully opaque

                // This one source pixel becomes a pixelScale x pixelScale block
                int blockX = (viewX + x) * pixelScale;
                int blockY = (viewY + y) * pixelScale;

                for (int by = 0; by < pixelScale; ++by)
                {
                    int py = blockY + by;
                    if (py < 0 || py >= target->height) continue; // vertical clip

                    for (int bx = 0; bx < pixelScale; ++bx)
                    {
                        int px = blockX + bx;
                        if (px < 0 || px >= target->width) continue; // horizontal clip

                        dst[py * target->width + px] = c;
                    }
                }
            }
        }
    }
}
