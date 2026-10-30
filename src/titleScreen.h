#pragma once

namespace Tmpl8
{
    class Surface;

	// The logo and some text, drawn straigh to the screen bc it is not part of the game world.
    class TitleScreen
    {
    public:
        ~TitleScreen() { Unload(); }
        void Load(const char* imagePath);
        void Unload();

        // True on the frame the player asks to start.
        bool Update(float deltaTime);

        void Draw(Surface* screen) const;

        // Nearest neighbour, whole pixels only, like the game view. Public so
        // the victory screen can use it too.
        static void BlitScaled(const Surface* src, Surface* dst,
                               int dstX, int dstY, int scale, bool keyBlack);

    private:

        Surface* image = nullptr;
        Surface* prompt = nullptr;

        float blinkTimer = 0.0f;
    };
}
