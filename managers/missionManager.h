#pragma once

#include "list.h"
#include "aabb.h"

namespace Tmpl8
{
    class Surface;
    class RenderManager;

    // One rectangle of tiles out of the JSON. A bounded map is a single chunk
    // covering everything, an infinite map is many small ones scattered over
    // world space, so this covers both.
    struct TileChunk
    {
        int x = 0;      // tile offset in the world, not pixels
        int y = 0;
        int width = 0;  // in tiles
        int height = 0;

        /*
        Raw tile values with Tiled's flip flags still in the top bits; Draw
        splits them. Masking them off at load time would silently unmirror
        every tile the level designer flipped.

        Unsigned because the horizontal flip flag is 0x80000000, which in a
        signed int is negative and would break the firstGid comparisons.
        */
        List<unsigned int> data;
    };

    // One tileset image plus the global id of its first tile. Tiled numbers
    // every tile in the map in one sequence, so ResolveTile has to work out
    // which sheet an id fell in.
    struct LoadedTileset
    {
        int firstGid = 1;
        Surface* surface = nullptr; // owned, freed in Unload
    };

    /*
    Owns the level: parses the Tiled JSON, keeps the tile data and the tileset
    images alive, and draws them.

    It draws itself rather than handing its data out, because it owns that
    data. It does not own pixels or know about zoom, so the actual drawing
    goes through the RenderManager. That keeps the dependency one way round.
    */
    class MissionManager
    {
    public:
        ~MissionManager() { Unload(); }

        // False (and empty) if the file is missing or broken, so the caller
        // can notice instead of running on with no map.
        bool Load(const char* jsonPath);

        void Unload();

        void Draw(const RenderManager& renderer) const;

        // The level's solid rectangles in world pixels, handed to the Player.
        // By const reference: no copy, and he cannot edit the level.
        const List<Collider>& GetColliders() const { return colliders; }

        int GetTileWidth() const { return tileWidth; }
        int GetTileHeight() const { return tileHeight; }

        // Only drawable with tiles AND images to draw them with.
        bool IsLoaded() const { return !chunks.empty() && !tilesets.empty(); }

    private:
        // Global tile id -> the sheet it lives on plus the cell number inside
        // it. nullptr for an id belonging to no loaded tileset.
        Surface* ResolveTile(int tileId, int& frameIndex) const;

        List<TileChunk> chunks;
        List<LoadedTileset> tilesets;

        // Every object layer rectangle ticked "collider" in Tiled. Plain data,
        // no pointers, so there is nothing to free.
        List<Collider> colliders;

        int tileWidth = 0;
        int tileHeight = 0;
    };
}
