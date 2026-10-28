#pragma once

#include "list.h"
#include "aabb.h"

namespace Tmpl8
{
    class Surface;
    class RenderManager;

    // One rectangle of tiles. A bounded map is one chunk, an infinite map many.
    struct TileChunk
    {
        int x = 0;      // tile offset in the world, not pixels
        int y = 0;
        int width = 0;  // in tiles
        int height = 0;

        // Flip flags still in the top bits, Draw splits them. Unsigned because
        // 0x80000000 is negative in a signed int and breaks the firstGid tests.
        List<unsigned int> data;
    };

    // Not an Enemy: the mission reports what the map asked for, the
    // EnemyManager knows what a "mummy" is. The name is copied because
    // tileson's string dies with the parsed map, into a fixed buffer so the
    // struct stays plain data List can copy.
    struct EnemySpawn
    {
        char variant[24] = "";

        // World pixels: x the middle of the body, y the floor under it.
        float feetX = 0.0f;
        float feetY = 0.0f;

        bool facingRight = true;
    };

    // One tileset image plus the global id of its first tile.
    struct LoadedTileset
    {
        int firstGid = 1;
        Surface* surface = nullptr; // owned, freed in Unload
    };

    // Owns the level: parses the Tiled JSON, keeps the tile data and tileset
    // images alive, and draws them through the RenderManager.
    class MissionManager
    {
    public:
        ~MissionManager() { Unload(); }

        // False (and empty) if the file is missing or broken.
        bool Load(const char* jsonPath);

        void Unload();

        void Draw(const RenderManager& renderer) const;

        const List<Collider>& GetColliders() const { return colliders; }

        // Kept, so a mission restart can place them all again without
        // re-reading the file.
        const List<EnemySpawn>& GetEnemySpawns() const { return enemySpawns; }

        int GetTileWidth() const { return tileWidth; }

        // Only drawable with tiles AND images to draw them with.
        bool IsLoaded() const { return !chunks.empty() && !tilesets.empty(); }

    private:
        // Global tile id -> the sheet it lives on plus the cell number inside
        // it. nullptr for an id belonging to no loaded tileset.
        Surface* ResolveTile(int tileId, int& frameIndex) const;

        List<TileChunk> chunks;
        List<LoadedTileset> tilesets;

        // Both plain data, so there is nothing to free.
        List<Collider> colliders;
        List<EnemySpawn> enemySpawns;

        int tileWidth = 0;
        int tileHeight = 0;
    };
}
