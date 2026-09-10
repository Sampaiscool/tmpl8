// Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#pragma once

#include "list.h"
#include "player.h"

namespace Tmpl8
{
    struct TileChunk 
    {
        int x = 0;          // Tile offset X
        int y = 0;          // Tile offset Y
        int width = 0;      // Chunk breedte in tiles
        int height = 0;     // Chunk hoogte in tiles
        List<int> data;     // Onze eigen List i.p.v. std::vector<int>
    };

    struct LoadedTileset 
    {
        int firstGid = 1;
        Surface* surface = nullptr;
    };

    class Game : public TheApp
    {
    public:
        void Init();
        void Tick(float deltaTime);
        void Shutdown()
        {
            for (int i = 0; i < loadedTilesets.size(); ++i)
            {
                delete loadedTilesets[i].surface;
            }
            loadedTilesets.clear();
        }

        void BlitTile(Surface* targetSurface, int frameIndex, int dstX, int dstY);

        void MouseUp(int) {}
        void MouseDown(int) {}
        void MouseMove(int x, int y) { mousePos.x = x; mousePos.y = y; }
        void MouseWheel(float) {}
        void KeyUp(int key) { keys[key & 511] = true; }
        void KeyDown(int key) { keys[key & 511] = false; }

        void LoadTiledMap(const char* jsonPath);

        int2 mousePos;

    private:
        int mapWidth = 0;
        int mapHeight = 0;
        int tileWidth = 0;
        int tileHeight = 0;
        float cameraX = 0.0f;
        float cameraY = 0.0f;

        bool keys[512] = { false };

        List<TileChunk> mapChunks;
        List<LoadedTileset> loadedTilesets;

        Player* player = nullptr;
    };

} // namespace Tmpl8
