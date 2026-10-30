#include "precomp.h"
#include "renderManager.h"

namespace Tmpl8
{
    void RenderManager::Unload()
    {
        delete view;
        view = nullptr;
    }

    // Current map: 1280 / (32 * 16) = 2, a 640x360 view doubled. Integer
    // division on purpose, a fractional scale makes some pixels wider than
    // others. The clamp matters because the division can come out 0.
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

        // Safe to call twice: a second mission replaces the view.
        delete view;
        view = new Surface( viewWidth, viewHeight );
    }

    // The target's own x cancels out, so it lands on anchorX * viewWidth
    // wherever it is in the world.
    void RenderManager::FollowTarget(float worldX, float anchorX)
    {
        cameraX = (viewWidth * anchorX) - worldX;
    }

    // Same sum on the other axis, run once instead of every frame.
    void RenderManager::SetVerticalAnchor(float worldY, float anchorY)
    {
        cameraY = (viewHeight * anchorY) - worldY;
    }

    // Slack on the negative side: something at viewX = -10 still pokes in.
    bool RenderManager::IsInView(int viewX, int viewY, int width, int height) const
    {
        return viewX > -width && viewX < viewWidth &&
               viewY > -height && viewY < viewHeight;
    }

    void RenderManager::Clear(unsigned int color) const
    {
        if (view) view->Clear(color);
    }

    // Copies one cell of a sheet into the view, mirrored if asked. Pixels live
    // in RAM row after row, so a pixel is at Y * Width + X. Flipping reads the
    // source row backwards. Pure black RGB is the color key.
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

        // Inside the sheet by construction, so the reads need no bounds check.
        const int srcX = (frameIndex % columns) * frameWidth;
        const int srcY = (frameIndex / columns) * frameHeight;

        // Clipped once here, so the loops need no per pixel bounds check.
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

    // Nearest neighbour, so every game pixel becomes a solid block and the art
    // stays crisp. Writes every screen pixel, so no need to clear first.
    void RenderManager::Present(Surface* screen) const
    {
        if (view && screen) view->CopyToScaled(screen);
    }
}
