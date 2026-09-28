#include "precomp.h"
#include "missionManager.h"
#include "renderManager.h"
#include "assets.h"

// X11 defines a macro called 'None' and tileson has a member with that name,
// so without this the preprocessor rewrites tileson and the compile fails with
// errors that make no sense. Linux only, but harmless everywhere.
#ifdef None
#undef None
#endif

#include "tileson.hpp"

namespace Tmpl8
{
    /*
    How Tiled packs a tile: the low 28 bits are the id, the top bits are flip
    flags. (Bit 29, the diagonal flip, would need a transpose and is not
    supported here.)
    */
    static const unsigned int TILE_ID_MASK = 0x0FFFFFFFu;
    static const unsigned int TILE_FLIP_H  = 0x80000000u;
    static const unsigned int TILE_FLIP_V  = 0x40000000u;

    void MissionManager::Unload()
    {
        for (LoadedTileset& tileset : tilesets)
        {
            delete tileset.surface;
            tileset.surface = nullptr;
        }
        tilesets.clear();
        chunks.clear();
        colliders.clear();

        tileWidth = 0;
        tileHeight = 0;
    }

    /*
    Parses the JSON and copies out the few things we need.

    tileson hands back a unique_ptr to a whole parsed tree; letting it go out
    of scope at the end frees all of that for us, so nothing in here leaks.

    A failed parse returns before Unload(), so a broken file leaves the
    previous mission intact instead of half destroying it.
    */
    bool MissionManager::Load(const char* jsonPath)
    {
        tson::Tileson tileson;
        std::unique_ptr<tson::Map> parsedMap = tileson.parse(std::filesystem::path(jsonPath));

        if (!parsedMap || parsedMap->getStatus() != tson::ParseStatus::OK)
        {
            printf( "Tileson failed to parse map JSON: %s\n", jsonPath );
            return false;
        }

        Unload(); // out with the old mission before putting the new one in

        tileWidth = parsedMap->getTileSize().x;
        tileHeight = parsedMap->getTileSize().y;

        for (tson::Tileset& parsedTileset : parsedMap->getTilesets())
        {
            LoadedTileset loaded;
            loaded.firstGid = parsedTileset.getFirstgid();

            // Tiled writes image paths relative to its own project root, and
            // sometimes absolute. Keep only the filename and put our own
            // folder in front of it.
            const std::filesystem::path imagePath(parsedTileset.getImagePath().string());
            const std::string localPath = std::string(ASSETS "maps/assets/") + imagePath.filename().string();

            loaded.surface = new Surface(localPath.c_str());
            tilesets.push_back(loaded);
        }

        /*
        One pass over the layers. The two kinds hold completely different
        things, so they fill different lists: a tile layer is a grid of ids, an
        object layer is free floating rectangles already in world pixels.
        */
        for (tson::Layer& layer : parsedMap->getLayers())
        {
            if (layer.getType() == tson::LayerType::TileLayer)
            {
                // Every chunk carries its own tile coordinates in the world.
                for (tson::Chunk& parsedChunk : layer.getChunks())
                {
                    TileChunk chunk;
                    chunk.x = parsedChunk.getPosition().x;
                    chunk.y = parsedChunk.getPosition().y;
                    chunk.width = parsedChunk.getSize().x;
                    chunk.height = parsedChunk.getSize().y;

                    // Copy tileson's storage into our own List. The cast keeps
                    // the bit pattern exactly, flip flags included; it only
                    // changes how we are allowed to read it.
                    for (const auto& value : parsedChunk.getData())
                    {
                        chunk.data.push_back(static_cast<unsigned int>(value));
                    }

                    chunks.push_back(chunk);
                }
            }
            else if (layer.getType() == tson::LayerType::ObjectGroup)
            {
                for (tson::Object& object : layer.getObjects())
                {
                    /*
                    Only objects the designer ticked "collider" on are solid.
                    An object layer is also where spawn points and triggers
                    will end up later, so everything else is left alone.

                    getProp returns null when the property is simply absent, so
                    that has to be checked before asking for the value.
                    */
                    if (object.getProp("collider") == nullptr) continue;
                    if (!object.get<bool>("collider")) continue;

                    Collider solid;
                    solid.box.x = static_cast<float>(object.getPosition().x);
                    solid.box.y = static_cast<float>(object.getPosition().y);
                    solid.box.w = static_cast<float>(object.getSize().x);
                    solid.box.h = static_cast<float>(object.getSize().y);

                    // Optional. Without it a collider stays solid from every
                    // side, which is why the default is false.
                    if (object.getProp("platform") != nullptr)
                    {
                        solid.oneWay = object.get<bool>("platform");
                    }

                    if (solid.box.w <= 0.0f || solid.box.h <= 0.0f) continue; // can never be hit

                    colliders.push_back(solid);
                }
            }
        }

        printf( "Mission loaded: %s (%i chunks, %i tilesets, %i colliders)\n",
                jsonPath, chunks.size(), tilesets.size(), colliders.size() );

        return IsLoaded();
    }

    /*
    Tiled numbers the tiles of every tileset in one continuous sequence, so the
    sheet we want is the one with the HIGHEST firstGid that is still <= the id.
    Scanning them all rather than stopping at the first hit means the order
    they happen to sit in does not matter.

    Subtracting firstGid turns the global id into a cell number inside that one
    sheet, which is what DrawFrame wants.
    */
    Surface* MissionManager::ResolveTile(int tileId, int& frameIndex) const
    {
        const LoadedTileset* best = nullptr;

        for (const LoadedTileset& tileset : tilesets)
        {
            if (tileset.firstGid > tileId) continue;
            if (!best || tileset.firstGid > best->firstGid) best = &tileset;
        }

        if (!best || !best->surface) return nullptr;

        frameIndex = tileId - best->firstGid;
        return best->surface;
    }

    /*
    Walks every tile of every chunk and draws the ones you can see. The early
    continues do the heavy lifting: an empty cell costs one comparison and an
    off screen tile costs two coordinate calculations, so we only pay the per
    pixel price for tiles that are actually visible.
    */
    void MissionManager::Draw(Surface* target, const RenderManager& renderer) const
    {
        if (!IsLoaded()) return;

        for (const TileChunk& chunk : chunks)
        {
            for (int cy = 0; cy < chunk.height; ++cy)
            {
                for (int cx = 0; cx < chunk.width; ++cx)
                {
                    const int cellIndex = cx + cy * chunk.width; // Y * Width + X
                    if (cellIndex >= chunk.data.size()) continue;

                    const unsigned int rawTile = chunk.data[cellIndex];

                    const int tileId = static_cast<int>(rawTile & TILE_ID_MASK);
                    if (tileId == 0) continue; // 0 is an empty cell in Tiled

                    int frameIndex = 0;
                    Surface* sheet = ResolveTile(tileId, frameIndex);
                    if (!sheet) continue;

                    // Tile grid position -> world pixels -> view pixels
                    const int viewX = renderer.ToViewX(static_cast<float>((chunk.x + cx) * tileWidth));
                    const int viewY = renderer.ToViewY(static_cast<float>((chunk.y + cy) * tileHeight));
                    if (!renderer.IsInView(viewX, viewY, tileWidth, tileHeight)) continue;

                    renderer.DrawFrame(target, sheet, tileWidth, tileHeight, frameIndex,
                                       viewX, viewY,
                                       (rawTile & TILE_FLIP_H) != 0,
                                       (rawTile & TILE_FLIP_V) != 0);
                }
            }
        }
    }
}
