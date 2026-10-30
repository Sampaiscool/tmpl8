#include "precomp.h"
#include "titleScreen.h"

extern bool IsKeyDown( const uint key );

namespace Tmpl8
{
    static const char* PROMPT_TEXT = "PRESS ENTER TO START";
    static const int PROMPT_SCALE = 4;
    static const unsigned int PROMPT_COLOR = 0xFFD040;

    static const float PROMPT_AT = 0.84f;

    static const float BLINK_PERIOD = 0.9f;

    void TitleScreen::Load(const char* imagePath)
    {
        Unload();
        image = new Surface(imagePath);

        // Surface::Print uses a 5x5 font with a 6 pixel advance and draws a
        // black shadow row under it, hence 6 tall.
        const int length = static_cast<int>(strlen(PROMPT_TEXT));
        prompt = new Surface(length * 6, 6);
        prompt->Clear(0);
        prompt->Print(PROMPT_TEXT, 0, 0, PROMPT_COLOR);
    }

    void TitleScreen::Unload()
    {
        delete image;
        image = nullptr;
        delete prompt;
        prompt = nullptr;
    }

    bool TitleScreen::Update(float deltaTime)
    {
        blinkTimer = fmodf(blinkTimer + deltaTime, BLINK_PERIOD);

        return IsKeyDown( GLFW_KEY_ENTER ) || IsKeyDown( GLFW_KEY_KP_ENTER );
    }

    void TitleScreen::Draw(Surface* screen) const
    {
        if (!screen) return;

        if (!image || image->width == 0)
        {
            screen->Clear(0);
            return;
        }

        // The image's own corner color, so the borders blend into it.
        screen->Clear(image->pixels[0]);

        // As big as fits, in whole pixels: 302x225 comes out 3x on 1280x720.
        int scale = std::min( screen->width / image->width, screen->height / image->height );
        if (scale < 1) scale = 1;

        const int imageX = (screen->width - image->width * scale) / 2;
        const int imageY = (screen->height - image->height * scale) / 2;
        BlitScaled(image, screen, imageX, imageY, scale, false);

        // Shown for the first half of each blink.
        if (prompt && blinkTimer < BLINK_PERIOD * 0.5f)
        {
            const int promptX = (screen->width - prompt->width * PROMPT_SCALE) / 2;
            const int promptY = imageY + static_cast<int>(image->height * scale * PROMPT_AT);
            BlitScaled(prompt, screen, promptX, promptY, PROMPT_SCALE, true);
        }
    }

    // keyBlack skips pure black, the same color key DrawFrame uses; the text needs it, the image must not have it.
    // Basicly the same as DrawFrame.
    void TitleScreen::BlitScaled(const Surface* src, Surface* dst,
                                 int dstX, int dstY, int scale, bool keyBlack)
    {
        for (int y = 0; y < src->height * scale; ++y)
        {
            const int ty = dstY + y;
            if (ty < 0 || ty >= dst->height) continue;

            const uint* srcRow = src->pixels + (y / scale) * src->width;
            uint* dstRow = dst->pixels + ty * dst->width;

            for (int x = 0; x < src->width * scale; ++x)
            {
                const int tx = dstX + x;
                if (tx < 0 || tx >= dst->width) continue;

                const uint c = srcRow[x / scale];
                if (keyBlack && (c & 0xFFFFFF) == 0) continue; // transparent

                dstRow[tx] = c | 0xFF000000;
            }
        }
    }
}
