#pragma once

namespace Tmpl8
{
    enum class PlayerState
    {
        Idle,
        Running
    };

    class Player
    {
    public:
        Player();
        ~Player();

        void Update(float deltaTime, const bool* keys);
        void Draw(Surface* target, float cameraX, float cameraY);

        float GetX() const { return position.x; }
        float GetY() const { return position.y; }
        void SetPosition(float x, float y) { position.x = x; position.y = y; }

    private:
        void DrawFlipped(Surface* target, int screenX, int screenY);

        struct Vec2 { float x, y; };

        Vec2 position{ 100.0f, 100.0f };
        Vec2 velocity{ 0.0f, 0.0f };

        // Twee apparte sprites voor elke animatie
        Sprite* idleSprite = nullptr;
        Sprite* runSprite = nullptr;
        Sprite* activeSprite = nullptr;

        PlayerState state = PlayerState::Idle;
        bool facingRight = true;

        // Animatie parameters
        float animTimer = 0.0f;
        float frameDuration = 0.10f;
        unsigned int currentFrame = 0;

        const unsigned int idleFrames = 4;
        const unsigned int runFrames = 6;
        const float speed = 180.0f;
    };
}
