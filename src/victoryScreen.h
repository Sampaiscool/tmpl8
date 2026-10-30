#pragma once

namespace Tmpl8
{
    class Surface;

    // The "mission complete" banner, drawn over the frozen level once every
    // enemy is dead and every POW is free. Straight to the screen, like the
    // title, because it is not part of the game world.
    class VictoryScreen
    {
    public:
        ~VictoryScreen() { Unload(); }
        void Load(const char* imagePath);
        void Unload();

        // Called once, when the mission is won; the count goes on the banner.
        void Show(int powsRescued);

        void Update(float deltaTime);

        // Over whatever is already on the screen, so call after Present.
        void Draw(Surface* screen) const;

    private:
        // One line of text, printed once into its own small surface.
        static Surface* MakeText(const char* text, unsigned int color);

        Surface* image = nullptr;
        Surface* headline = nullptr;
        Surface* rescuedLine = nullptr;
        Surface* prompt = nullptr;

        float blinkTimer = 0.0f;
    };
}
