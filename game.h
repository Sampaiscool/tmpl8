// Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#pragma once

#include <vector>
#include <string>

namespace Tmpl8
{

    struct TileChunk {
        int x = 0;          // Tile offset X
        int y = 0;          // Tile offset Y
        int width = 0;      // Chunk breedte in tiles
        int height = 0;     // Chunk hoogte in tiles
        std::vector<int> data;
    };

    // Container to pair loaded tileset textures with their Tiled GID offsets
    struct LoadedTileset {
        int firstGid = 1;
        Surface* surface = nullptr;
    };

    class Game : public TheApp
    {
    public:
        // game flow methods
        void Init();
        void Tick(float deltaTime);
        void Shutdown()
        {
            for (auto& ts : loadedTilesets)
            {
                delete ts.surface;
            }
            loadedTilesets.clear();
        }

        // Pass target surface pointer directly to handle multiple tilesets
        void BlitTile(Surface* targetSurface, int frameIndex, int dstX, int dstY);

        // input handling
        void MouseUp(int) { /* implement if you want to detect mouse button presses */ }
        void MouseDown(int) { /* implement if you want to detect mouse button presses */ }
        void MouseMove(int x, int y) { mousePos.x = x, mousePos.y = y; }
        void MouseWheel(float) { /* implement if you want to handle the mouse wheel */ }
        void KeyUp(int) { /* implement if you want to handle keys */ }
        void KeyDown(int) { /* implement if you want to handle keys */ }

        // Tiled Loader helper
        void LoadTiledMap(const std::string& jsonPath);

        // data members
        int2 mousePos;

    private:
        // Map Properties
        int mapWidth = 0;
        int mapHeight = 0;
        int tileWidth = 0;
        int tileHeight = 0;

        std::vector<TileChunk> mapChunks;

        // Multi-tileset container
        std::vector<LoadedTileset> loadedTilesets;
    };

} // namespace Tmpl8