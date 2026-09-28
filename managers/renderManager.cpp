#include "precomp.h"
#include "renderManager.h"

namespace Tmpl8
{
    /*
    You say how many tiles you want across (TILES_ON_SCREEN), the map says how
    many pixels a tile is, so the world we want to show is that many pixels
    wide. The real screen width divided by that is the scale.

    Current map: 1280 / (32 * 16) = 2. A 32x32 tileset gives 1, so you still
    see about the same amount of world instead of everything doubling.

    Integer division on purpose: a scale like 2.5 would make some pixels 2 real
    pixels wide and others 3, which looks wobbly on pixel art. The clamp to 1
    matters because the division can come out 0, which would divide by zero
    below and make everything vanish.
    */
    void RenderManager::SetupZoom(int tileWidth)
    {
        if (tileWidth <= 0) return; // no map, keep the 1:1 defaults

        pixelScale = SCRWIDTH / (TILES_ON_SCREEN * tileWidth);
        if (pixelScale < 1) pixelScale = 1;

        viewWidth = SCRWIDTH / pixelScale;
        viewHeight = SCRHEIGHT / pixelScale;

        printf( "Zoom: %i px tiles at %ix, showing %i tiles across\n",
                tileWidth, pixelScale, viewWidth / tileWidth );
    }

    /*
    Substitute this offset into ToViewX and the target's own x cancels out, so
    it always lands on anchorX * viewWidth wherever it is in the world.

    viewWidth and not SCRWIDTH: the camera works in small world pixels, the
    zoom to real pixels only happens at the very end inside DrawFrame.
    */
    void RenderManager::FollowTarget(float worldX, float worldY, float anchorX, float anchorY)
    {
        cameraX = (viewWidth * anchorX) - worldX;
        cameraY = (viewHeight * anchorY) - worldY;
    }

    // One full width/height of slack on the negative side, because something
    // at viewX = -10 still has most of itself poking into the view.
    bool RenderManager::IsInView(int viewX, int viewY, int width, int height) const
    {
        return viewX > -width && viewX < viewWidth &&
               viewY > -height && viewY < viewHeight;
    }

    /*
    Copies one cell of a sheet into the screen buffer, mirrored if asked and
    blown up by pixelScale.

    Finding the cell: pixels live in RAM as one long array, row after row, so a
    pixel is at Y * Width + X. Dividing the sheet by the frame size gives how
    many cells fit across, and the cell number turns back into a corner with
    the same formula in reverse. A spritesheet is just the special case of one
    row, so it needs no separate path.

    Flipping reads the source row backwards instead of keeping a second
    mirrored image on disk.

    Transparency: pure black RGB counts as see-through and is skipped, which is
    the same colour key the template's own Sprite uses. Kept pixels get full
    alpha forced on.

    Clipping is worked out per row and per block instead of per written pixel,
    so the innermost loop is a plain write with no branches in it.
    */
    void RenderManager::DrawFrame(Surface* target, Surface* sheet,
                                  int frameWidth, int frameHeight, int frameIndex,
                                  int viewX, int viewY,
                                  bool flipX, bool flipY) const
    {
        if (!target || !sheet || !sheet->pixels) return;
        if (frameWidth <= 0 || frameHeight <= 0) return;

        const int columns = sheet->width / frameWidth;
        const int rows = sheet->height / frameHeight;
        if (columns <= 0 || rows <= 0) return;
        if (frameIndex < 0 || frameIndex >= columns * rows) return;

        // Cell number -> top left corner of that cell, in source pixels. These
        // are inside the sheet by construction, so the read below needs no
        // bounds check of its own.
        const int srcX = (frameIndex % columns) * frameWidth;
        const int srcY = (frameIndex / columns) * frameHeight;

        const uint* src = sheet->pixels;
        uint* dst = target->pixels;

        for (int y = 0; y < frameHeight; ++y)
        {
            const int sy = srcY + (flipY ? (frameHeight - 1 - y) : y);

            // Which rows of this pixel's block land on screen. Only depends on
            // y, so it is worked out once per source row.
            const int blockY = (viewY + y) * pixelScale;
            const int by0 = std::max(0, -blockY);
            const int by1 = std::min(pixelScale, target->height - blockY);
            if (by0 >= by1) continue; // whole row is off screen

            const uint* srcRow = src + sy * sheet->width;

            for (int x = 0; x < frameWidth; ++x)
            {
                const int sx = srcX + (flipX ? (frameWidth - 1 - x) : x);

                uint c = srcRow[sx];
                if ((c & 0xFFFFFF) == 0) continue; // transparent
                c |= 0xFF000000;

                const int blockX = (viewX + x) * pixelScale;
                const int bx0 = std::max(0, -blockX);
                const int bx1 = std::min(pixelScale, target->width - blockX);

                for (int by = by0; by < by1; ++by)
                {
                    uint* dstRow = dst + (blockY + by) * target->width;
                    for (int bx = bx0; bx < bx1; ++bx) dstRow[blockX + bx] = c;
                }
            }
        }
    }

    /*
    Surface::Box already draws a clipped outline, so this only has to do the
    view -> real pixel conversion DrawFrame does and hand it over.

    The outline stays 1 REAL pixel thick rather than pixelScale thick, so it
    is a hairline that does not hide the art underneath it.
    */
    void RenderManager::DrawBox(Surface* target, int viewX, int viewY,
                                int width, int height, unsigned int color) const
    {
        if (!target || width <= 0 || height <= 0) return;

        target->Box(viewX * pixelScale,
                    viewY * pixelScale,
                    (viewX + width) * pixelScale - 1,
                    (viewY + height) * pixelScale - 1,
                    color | 0xFF000000);
    }
}
