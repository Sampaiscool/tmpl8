#include "precomp.h"
#include "victoryScreen.h"
#include "titleScreen.h" // BlitScaled

namespace Tmpl8
{
    static const char* HEADLINE_TEXT = "MISSION COMPLETE!";
    static const char* PROMPT_TEXT = "PRESS ESC TO QUIT";

    static const unsigned int HEADLINE_COLOR = 0xFFD040;
    static const unsigned int RESCUED_COLOR = 0xFFFFFF;
    static const unsigned int PROMPT_COLOR = 0xA0D8FF;

    static const int HEADLINE_SCALE = 6;
    static const int RESCUED_SCALE = 4;
    static const int PROMPT_SCALE = 3;

    // Where each line sits, as a fraction of the banner's height. The banner
    // has no text of its own, it is just the blue strip.
    static const float HEADLINE_AT = 0.16f;
    static const float RESCUED_AT = 0.50f;
    static const float PROMPT_AT = 0.78f;

    static const float BLINK_PERIOD = 0.9f;

    void VictoryScreen::Load(const char* imagePath)
    {
        Unload();
        image = new Surface(imagePath);
        headline = MakeText(HEADLINE_TEXT, HEADLINE_COLOR);
        prompt = MakeText(PROMPT_TEXT, PROMPT_COLOR);
    }

    void VictoryScreen::Unload()
    {
        delete image;
        image = nullptr;
        delete headline;
        headline = nullptr;
        delete rescuedLine;
        rescuedLine = nullptr;
        delete prompt;
        prompt = nullptr;
    }

    void VictoryScreen::Show(int powsRescued)
    {
        char text[32];
        snprintf(text, sizeof(text), "POWS RESCUED: %i", powsRescued);

        delete rescuedLine;
        rescuedLine = MakeText(text, RESCUED_COLOR);

        blinkTimer = 0.0f;
    }

    void VictoryScreen::Update(float deltaTime)
    {
        blinkTimer = fmodf(blinkTimer + deltaTime, BLINK_PERIOD);
    }

    // Same sizes as the title's prompt: 5x5 font, 6 pixel advance, a shadow row underneath.
    Surface* VictoryScreen::MakeText(const char* text, unsigned int color)
    {
        const int length = static_cast<int>(strlen(text));
        Surface* line = new Surface(length * 6, 6);
        line->Clear(0);
        line->Print(text, 0, 0, color);
        return line;
    }

    void VictoryScreen::Draw(Surface* screen) const
    {
        if (!screen || !image || image->width == 0) return;

        // 305x137 comes out 3x on 1280x720, leaving the level visible round it.
        int scale = std::min( screen->width / image->width, screen->height / image->height ) - 1;
        if (scale < 1) scale = 1;

        const int bannerW = image->width * scale;
        const int bannerH = image->height * scale;
        const int bannerX = (screen->width - bannerW) / 2;
        const int bannerY = (screen->height - bannerH) / 2;
        TitleScreen::BlitScaled(image, screen, bannerX, bannerY, scale, false);

        // Centred across the banner, at its fraction of the banner's height.
        auto drawLine = [&](const Surface* line, int lineScale, float at)
        {
            if (!line) return;
            const int x = (screen->width - line->width * lineScale) / 2;
            const int y = bannerY + static_cast<int>(bannerH * at);
            TitleScreen::BlitScaled(line, screen, x, y, lineScale, true);
        };

        drawLine(headline, HEADLINE_SCALE, HEADLINE_AT);
        drawLine(rescuedLine, RESCUED_SCALE, RESCUED_AT);

        // Shown for the first half of each blink.
        if (blinkTimer < BLINK_PERIOD * 0.5f) drawLine(prompt, PROMPT_SCALE, PROMPT_AT);
    }
}
