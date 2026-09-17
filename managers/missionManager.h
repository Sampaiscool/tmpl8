#pragma once

#include "list.h"

namespace Tmpl8
{
    class Surface;
    class RenderManager;

    /*
    This code:
    One rectangle of tiles out of the Tiled JSON. A bounded map gives us a
    single chunk covering everything, an infinite map gives us a lot of small
    ones scattered over world space, so this struct covers both.
    */
    struct TileChunk
    {
        int x = 0;          // tile offset X in the world
        int y = 0;          // tile offset Y in the world
        int width = 0;      // chunk width in tiles
        int height = 0;     // chunk height in tiles

        /*
        The RAW tile values straight from Tiled, flip flags still attached in
        the top 4 bits. We used to mask those off while loading and throw them
        away, wich meant a tile the level designer mirrored in Tiled got drawn
        unmirrored. Now we keep the whole value and split it at draw time.

        It has to be unsigned: the horizontal flip flag is 0x80000000, wich
        sets the sign bit. In a signed int that value is negative and the
        comparisons against firstGid would go haywire.
        */
        List<unsigned int> data;
    };

    /*
    This code:
    One tileset image plus the global tile id its first tile owns. Tiled
    numbers every tile in the whole map in one long sequence, so the only way
    to know wich image a tile belongs to is to find the tileset with the
    highest firstGid that is still <= that tile id. ResolveTile does that.

    'surface' is owned by the MissionManager and freed in Unload().
    */
    struct LoadedTileset
    {
        int firstGid = 1;
        Surface* surface = nullptr;
    };

    /*
    This code:
    The MissionManager owns the level: it parses the Tiled JSON, keeps the
    tile data and the tileset images alive, and draws them.

    Why it draws itself instead of handing its data to game.cpp:
    it owns the tile data, so walking over that data belongs here. It does not
    own any pixels or know about zoom, so for the actual drawing it asks the
    RenderManager. That keeps the dependency one way round (mission -> render)
    and keeps Game::Tick down to a handfull of lines.

    Ownership:
    the tileset Surfaces are raw pointers allocated with new, so the
    destructor frees them. Loading a second mission frees the first one's
    images before allocating the new ones.
    */
    class MissionManager
    {
    public:
        ~MissionManager() { Unload(); }

        /*
        Parses the Tiled JSON and loads every tileset image it names. Returns
        false and leaves the manager empty if the file is missing or broken,
        so the caller can notice instead of running on with no map.
        */
        bool Load(const char* jsonPath);

        // Frees the tileset images and drops the tile data
        void Unload();

        // Draws every visible tile of the mission through the renderer
        void Draw(Surface* target, const RenderManager& renderer) const;

        int GetTileWidth() const { return tileWidth; }
        int GetTileHeight() const { return tileHeight; }

        // A mission is only drawable if we have tiles AND images to draw them with
        bool IsLoaded() const { return !chunks.empty() && !tilesets.empty(); }

    private:
        /*
        Turns a Tiled global tile id into the sheet it lives on plus the cell
        number inside that sheet. Returns nullptr for a tile id that belongs
        to no loaded tileset.
        */
        Surface* ResolveTile(int tileId, int& frameIndex) const;

        List<TileChunk> chunks;
        List<LoadedTileset> tilesets;

        int tileWidth = 0;
        int tileHeight = 0;
    };
}
