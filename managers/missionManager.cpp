#include "precomp.h"
#include "missionManager.h"
#include "renderManager.h"
#include "assets.h"

// X11 defines a macro called 'None' and tileson has a member with that name,
// so without this the preprocessor rewrites tileson and the compile fails.
#ifdef None
#undef None
#endif

#include "tileson.hpp"

namespace Tmpl8
{
    // Tiled packs the tile id in the low 28 bits and flip flags in the top.
    // (Bit 29, the diagonal flip, would need a transpose and is not supported.)
    static const unsigned int TILE_ID_MASK = 0x0FFFFFFFu;
    static const unsigned int TILE_FLIP_H  = 0x80000000u;
    static const unsigned int TILE_FLIP_V  = 0x40000000u;

    // An object with an "enemy" string property naming the variant. A
    // rectangle stands the enemy on the middle of its bottom edge, a point
    // spawns it where you clicked.
    static bool ReadEnemySpawn(tson::Object& object, EnemySpawn& spawn)
    {
        if (object.getProp("enemy") == nullptr) return false;

        const std::string variant = object.get<std::string>("enemy");

        // Always terminates, unlike strncpy.
        snprintf(spawn.variant, sizeof(spawn.variant), "%s", variant.c_str());
        if (spawn.variant[0] == '\0') return false;

        const float x = static_cast<float>(object.getPosition().x);
        const float y = static_cast<float>(object.getPosition().y);
        const float w = static_cast<float>(object.getSize().x);
        const float h = static_cast<float>(object.getSize().y);

        if (w > 0.0f && h > 0.0f)
        {
            spawn.feetX = x + w * 0.5f;
            spawn.feetY = y + h;
        }
        else
        {
            spawn.feetX = x;
            spawn.feetY = y;
        }

        // Optional, so the common case needs no property at all.
        if (object.getProp("facingRight") != nullptr)
        {
            spawn.facingRight = object.get<bool>("facingRight");
        }

        return true;
    }

    // Only objects ticked "collider" are solid. getProp returns null when the
    // property is absent, so check before reading.
    static bool ReadCollider(tson::Object& object, Collider& solid)
    {
        if (object.getProp("collider") == nullptr) return false;
        if (!object.get<bool>("collider")) return false;

        solid.box.x = static_cast<float>(object.getPosition().x);
        solid.box.y = static_cast<float>(object.getPosition().y);
        solid.box.w = static_cast<float>(object.getSize().x);
        solid.box.h = static_cast<float>(object.getSize().y);

        // Without the flag a collider is solid from every side.
        if (object.getProp("platform") != nullptr)
        {
            solid.oneWay = object.get<bool>("platform");
        }

        return solid.box.w > 0.0f && solid.box.h > 0.0f; // else it can never be hit
    }

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
        enemySpawns.clear();

        tileWidth = 0;
        tileHeight = 0;
    }

    // A failed parse returns before Unload(), so a broken file leaves the
    // previous mission intact.
    bool MissionManager::Load(const char* jsonPath)
    {
        tson::Tileson tileson;
        std::unique_ptr<tson::Map> parsedMap = tileson.parse(std::filesystem::path(jsonPath));

        if (!parsedMap || parsedMap->getStatus() != tson::ParseStatus::OK)
        {
            printf( "Tileson failed to parse map JSON: %s\n", jsonPath );
            return false;
        }

        Unload();

        tileWidth = parsedMap->getTileSize().x;
        tileHeight = parsedMap->getTileSize().y;

        for (tson::Tileset& parsedTileset : parsedMap->getTilesets())
        {
            LoadedTileset loaded;
            loaded.firstGid = parsedTileset.getFirstgid();

            // Tiled's paths are relative to its own project root, sometimes
            // absolute. Keep the filename and put our folder in front.
            const std::filesystem::path imagePath(parsedTileset.getImagePath().string());
            const std::string localPath = std::string(ASSETS "maps/assets/") + imagePath.filename().string();

            loaded.surface = new Surface(localPath.c_str());
            tilesets.push_back(loaded);
        }

        // Tile layers are a grid of ids, object layers free floating
        // rectangles already in world pixels.
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

                    // The cast keeps the bit pattern, flip flags included.
                    for (const auto& value : parsedChunk.getData())
                    {
                        chunk.data.push_back(static_cast<unsigned int>(value));
                    }

                    chunks.push_back(chunk);
                }
            }
            else if (layer.getType() == tson::LayerType::ObjectGroup)
            {
                // Spawns first, so one object is never both.
                for (tson::Object& object : layer.getObjects())
                {
                    EnemySpawn spawn;
                    if (ReadEnemySpawn(object, spawn))
                    {
                        enemySpawns.push_back(spawn);
                        continue;
                    }

                    Collider solid;
                    if (ReadCollider(object, solid)) colliders.push_back(solid);
                }
            }
        }

        printf( "Mission loaded: %s (%i chunks, %i tilesets, %i colliders, %i enemy spawns)\n",
                jsonPath, chunks.size(), tilesets.size(), colliders.size(),
                enemySpawns.size() );

        return IsLoaded();
    }

    // Tiled numbers every tileset in one sequence, so the sheet we want has the
    // highest firstGid still <= the id. Subtracting it gives the cell number.
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

    // The early continues are what keep a big map cheap: we only pay the per
    // pixel price for visible tiles.
    void MissionManager::Draw(const RenderManager& renderer) const
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

                    renderer.DrawFrame(sheet, tileWidth, tileHeight, frameIndex,
                                       viewX, viewY,
                                       (rawTile & TILE_FLIP_H) != 0,
                                       (rawTile & TILE_FLIP_V) != 0);
                }
            }
        }
    }
}
