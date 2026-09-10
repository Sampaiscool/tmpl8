#include "precomp.h"
#include "player.h"

namespace Tmpl8
{
    Player::Player()
    {
        Surface* idleSurface = new Surface("../assets/Marco/Marco_merged.png");
        idleSprite = new Sprite(idleSurface, idleFrames);

        Surface* runSurface = new Surface("../assets/Marco/runMarco_merged.png");
        runSprite = new Sprite(runSurface, runFrames);

        activeSprite = idleSprite;
    }

    Player::~Player()
    {
        delete idleSprite;
        delete runSprite;
    }

    void Player::Update(float deltaTime, const bool* keys)
    {
        velocity = { 0.0f, 0.0f };

        if (keys['a'] || keys['A'] || keys[GLFW_KEY_LEFT])  velocity.x += 1.0f;
        if (keys['d'] || keys['D'] || keys[GLFW_KEY_RIGHT]) velocity.x -= 1.0f;

        // Richting bepalen (gespiegeld of niet)
        if (velocity.x < 0.0f) facingRight = false;
        else if (velocity.x > 0.0f) facingRight = true;

        if (velocity.x != 0.0f && velocity.y != 0.0f)
        {
            velocity.x *= 0.7071f;
            velocity.y *= 0.7071f;
        }

        position.x += velocity.x * speed * deltaTime;
        position.y += velocity.y * speed * deltaTime;

        PlayerState newState = (velocity.x != 0.0f || velocity.y != 0.0f) ? PlayerState::Running : PlayerState::Idle;

        if (newState != state)
        {
            state = newState;
            currentFrame = 0;
            animTimer = 0.0f;
        }

        unsigned int maxFrames = 0;
        if (state == PlayerState::Running)
        {
            activeSprite = runSprite;
            maxFrames = runFrames;
        }
        else
        {
            activeSprite = idleSprite;
            maxFrames = idleFrames;
        }

        animTimer += deltaTime;
        if (animTimer >= frameDuration)
        {
            animTimer -= frameDuration;
            currentFrame = (currentFrame + 1) % maxFrames;
        }

        if (activeSprite)
        {
            activeSprite->SetFrame(currentFrame);
        }
    }

    void Player::Draw(Surface* target, float cameraX, float cameraY)
    {
        if (!activeSprite) return;

        int screenX = static_cast<int>(position.x + cameraX);
        int screenY = static_cast<int>(position.y + cameraY);

        if (facingRight)
        {
            // Standaard Tmpl8 rendering voor rechts lopen
            activeSprite->Draw(target, screenX, screenY);
        }
        else
        {
            // Custom pixel-flip rendering voor links lopen
            DrawFlipped(target, screenX, screenY);
        }
    }

    void Player::DrawFlipped(Surface* target, int screenX, int screenY)
    {
        if (!activeSprite || !target) return;

        int width = activeSprite->GetWidth();
        int height = activeSprite->GetHeight();
        
        // Wijs direct naar de start van het HUIDIGE frame in de spritesheet
        uint* src = activeSprite->GetBuffer() + currentFrame * width;
        uint* dst = target->pixels;

        int targetWidth = target->width;
        int targetHeight = target->height;
        int numFrames = activeSprite->GetFrameCount(); // Mocht GetFrameCount() niet bestaan, gebruik dan idleFrames / runFrames afhankelijk van de state

        for (int y = 0; y < height; ++y)
        {
            int dy = screenY + y;

            // Binnen de verticale schermgrenzen?
            if (dy >= 0 && dy < targetHeight)
            {
                for (int x = 0; x < width; ++x)
                {
                    int dx = screenX + x;

                    // Binnen de horizontale schermgrenzen?
                    if (dx >= 0 && dx < targetWidth)
                    {
                        // KEY TRICK: (width - 1 - x) spiegelt de x-as van de sprite
                        uint pixel = src[width - 1 - x];

                        // Sla zwarte/transparante pixels over en teken naar het scherm
                        if ((pixel & 0xFFFFFF) != 0)
                        {
                            dst[dy * targetWidth + dx] = pixel | 0xFF000000;
                        }
                    }
                }
            }

            // Spring in de bron-buffer naar de volgende scanline (rij) van de spritesheet
            src += width * numFrames;
        }
    }
}
