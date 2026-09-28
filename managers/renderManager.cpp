#include "precomp.h"
#include "renderManager.h"

namespace Tmpl8
{
    void RenderManager::Unload()
    {
        delete view;
        view = nullptr;
    }

    /*
    You say how many tiles you want across (TILES_ON_SCREEN), the map says how
    many pixels a tile is, so the world we want to show is that many pixels
    wide. The real screen width divided by that is the scale.

    Current map: 1280 / (32 * 16) = 2, so we draw into a 640x360 view and
    double it. A 32x32 tileset gives 1, so you still see about the same amount
    of world instead of everything doubling.

    Integer division on purpose: a fractional scale would make some pixels
    wider than others, which looks wobbly on pixel art. The clamp to 1 matters
    because the division can come out 0, which would give a zero sized view.
    */
    void RenderManager::SetupZoom(int tileWidth)
    {
        if (tileWidth > 0)
        {
            pixelScale = SCRWIDTH / (TILES_ON_SCREEN * tileWidth);
            if (pixelScale < 1) pixelScale = 1;

            viewWidth = SCRWIDTH / pixelScale;
            viewHeight = SCRHEIGHT / pixelScale;

            printf( "Zoom: %i px tiles at %ix, showing %i tiles across\n",
                    tileWidth, pixelScale, viewWidth / tileWidth );
        }

        // Safe to call twice: a second mission replaces the view rather than
        // leaking the first one.
        delete view;
        view = new Surface( viewWidth, viewHeight );
    }

    /*
    Substitute this offset into ToViewX and the target's own x cancels out, so
    it always lands on anchorX * viewWidth wherever it is in the world.
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

    void RenderManager::Clear(unsigned int color) const
    {
        if (view) view->Clear(color);
    }

    /*
    Copies one cell of a sheet into the view, mirrored if asked.

    Finding the cell: pixels live in RAM as one long array, row after row, so a
    pixel is at Y * Width + X. Dividing the sheet by the frame size gives how
    many cells fit across, and the cell number turns back into a corner with
    the same formula in reverse. A spritesheet is just the special case of one
    row, so it needs no separate path.

    Flipping reads the source row backwards instead of keeping a second
    mirrored image on disk.

    Transparency: pure black RGB counts as see-through and is skipped, the same
    colour key the template's own Sprite uses. Kept pixels get full alpha.

    There is no zoom maths here at all; Present() enlarges the whole view in
    one go afterwards.
    */
    void RenderManager::DrawFrame(Surface* sheet,
                                  int frameWidth, int frameHeight, int frameIndex,
                                  int viewX, int viewY,
                                  bool flipX, bool flipY) const
    {
        if (!view || !sheet || !sheet->pixels) return;
        if (frameWidth <= 0 || frameHeight <= 0) return;

        const int columns = sheet->width / frameWidth;
        const int rows = sheet->height / frameHeight;
        if (columns <= 0 || rows <= 0) return;
        if (frameIndex < 0 || frameIndex >= columns * rows) return;

        // Cell number -> top left corner of that cell, in source pixels. These
        // are inside the sheet by construction, so the reads below need no
        // bounds check of their own.
        const int srcX = (frameIndex % columns) * frameWidth;
        const int srcY = (frameIndex / columns) * frameHeight;

        // Which part of the frame actually lands on the view. Clipped once
        // here so the loops need no per pixel bounds check.
        const int x0 = std::max( 0, -viewX );
        const int x1 = std::min( frameWidth,  view->width  - viewX );
        const int y0 = std::max( 0, -viewY );
        const int y1 = std::min( frameHeight, view->height - viewY );

        const uint* src = sheet->pixels;
        uint* dst = view->pixels;

        for (int y = y0; y < y1; ++y)
        {
            const int sy = srcY + (flipY ? (frameHeight - 1 - y) : y);
            const uint* srcRow = src + sy * sheet->width;
            uint* dstRow = dst + (viewY + y) * view->width;

            for (int x = x0; x < x1; ++x)
            {
                const int sx = srcX + (flipX ? (frameWidth - 1 - x) : x);

                const uint c = srcRow[sx];
                if ((c & 0xFFFFFF) == 0) continue; // transparent

                dstRow[viewX + x] = c | 0xFF000000;
            }
        }
    }

    // Surface::Box already clips, so there is nothing to do but hand it over.
    void RenderManager::DrawBox(int viewX, int viewY, int width, int height,
                                unsigned int color) const
    {
        if (!view || width <= 0 || height <= 0) return;

        view->Box(viewX, viewY, viewX + width - 1, viewY + height - 1,
                  color | 0xFF000000);
    }

    /*
    The zoom, all of it, in one call. CopyToScaled walks the destination and
    nearest neighbour samples the view, so every game pixel becomes a solid
    pixelScale x pixelScale block with no blending, and the art stays crisp.

    It writes every pixel of the screen, so there is no need to clear it.
    */
    void RenderManager::Present(Surface* screen) const
    {
        if (view && screen) view->CopyToScaled(screen);
    }
}
