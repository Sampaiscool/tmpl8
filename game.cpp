// Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#include "precomp.h"
#include "game.h"
#include <iostream>

// --- FIX FOR X11 MACRO CONFLICT ---
#ifdef None
#undef None
#endif

#include "tileson.hpp"

namespace Tmpl8
{

    void Game::LoadTiledMap(const std::string& jsonPath)
    {
        tson::Tileson t;
        std::unique_ptr<tson::Map> parsedMap = t.parse(std::filesystem::path(jsonPath));

        if (!parsedMap || parsedMap->getStatus() != tson::ParseStatus::OK)
        {
            std::cout << "Tileson failed to parse map JSON: " << jsonPath << std::endl;
            return;
        }

        tileWidth = parsedMap->getTileSize().x;
        tileHeight = parsedMap->getTileSize().y;
        mapChunks.clear();

        // 1. Clean up old tilesets if re-loading
        for (auto& ts : loadedTilesets)
        {
            delete ts.surface;
        }
        loadedTilesets.clear();

        // 2. Load every tileset dynamically from the JSON
        for (auto& ts : parsedMap->getTilesets())
        {
            LoadedTileset lts;
            lts.firstGid = ts.getFirstgid();

            // 1. Haal de raw path string op uit Tileson
            std::string rawPath = ts.getImagePath().string();

            // 2. Strip evt. quotes of paden, pak alleen de losse filename
            std::filesystem::path p(rawPath);
            std::string filename = p.filename().string();

            // 3. Bouw het schone pad
            std::string imagePath = "maps/assets/" + filename;

            std::cout << "Loading tileset: [" << imagePath << "]" << std::endl;

            lts.surface = new Surface(imagePath.c_str());

            if (!lts.surface || lts.surface->width == 0)
            {
                std::cout << "ERROR: Failed to load surface at " << imagePath << std::endl;
            }

            loadedTilesets.push_back(lts);
        }

        // 3. Load chunk and layer data
        for (auto& layer : parsedMap->getLayers())
        {
            if (layer.getType() == tson::LayerType::TileLayer)
            {
                if (layer.getChunks().empty())
                {
                    TileChunk chunk;
                    chunk.x = 0;
                    chunk.y = 0;
                    chunk.width = layer.getSize().x;
                    chunk.height = layer.getSize().y;

                    for (auto& tileId : layer.getData())
                    {
                        chunk.data.push_back(tileId & 0x0FFFFFFF);
                    }
                    mapChunks.push_back(chunk);
                }
                else
                {
                    for (auto& chunkData : layer.getChunks())
                    {
                        TileChunk chunk;
                        chunk.x = chunkData.getPosition().x;
                        chunk.y = chunkData.getPosition().y;
                        chunk.width = chunkData.getSize().x;
                        chunk.height = chunkData.getSize().y;

                        for (auto& tileId : chunkData.getData())
                        {
                            chunk.data.push_back(static_cast<int>(tileId & 0x0FFFFFFF));
                        }
                        mapChunks.push_back(chunk);
                    }
                }
            }
        }
    }

    void Game::BlitTile(Surface* targetSurface, int frameIndex, int dstX, int dstY)
    {
        if (!targetSurface || tileWidth <= 0) return;

        int tileCols = targetSurface->width / tileWidth;
        if (tileCols <= 0) return;

        int tileRows = targetSurface->height / tileHeight;
        int maxFrames = tileCols * tileRows;

        if (frameIndex < 0 || frameIndex >= maxFrames) return;

        int srcX = (frameIndex % tileCols) * tileWidth;
        int srcY = (frameIndex / tileCols) * tileHeight;

        uint* dst = screen->pixels;

        for (int y = 0; y < tileHeight; ++y)
        {
            int sy = srcY + y;
            if (sy >= targetSurface->height) break;
            int dy = dstY + y;
            if (dy < 0 || dy >= screen->height) continue;

            for (int x = 0; x < tileWidth; ++x)
            {
                int sx = srcX + x;
                if (sx >= targetSurface->width) break;
                int dx = dstX + x;
                if (dx < 0 || dx >= screen->width) continue;

                uint c = targetSurface->pixels[sy * targetSurface->width + sx];

                // Render non-transparent pixels with solid alpha bit set
                if ((c & 0xFFFFFF) != 0)
                {
                    dst[dy * screen->width + dx] = c | 0xFF000000;
                }
            }
        }
    }

    void Game::Tick(float /* deltaTime */)
    {
        screen->Clear(0x1e1e1e);

        if (loadedTilesets.empty() || mapChunks.empty()) return;

        int cameraX = -200;
        int cameraY = 0;

        for (const auto& chunk : mapChunks)
        {
            for (int cy = 0; cy < chunk.height; ++cy)
            {
                for (int cx = 0; cx < chunk.width; ++cx)
                {
                    int tileIndex = cx + cy * chunk.width;
                    if (tileIndex >= chunk.data.size()) continue;

                    int tileId = chunk.data[tileIndex];

                    if (tileId > 0)
                    {
                        // Match tileId to the highest matching firstGid
                        LoadedTileset* bestTileset = nullptr;
                        for (auto& ts : loadedTilesets)
                        {
                            if (tileId >= ts.firstGid)
                            {
                                if (!bestTileset || ts.firstGid > bestTileset->firstGid)
                                {
                                    bestTileset = &ts;
                                }
                            }
                        }

                        if (bestTileset && bestTileset->surface)
                        {
                            int frameIndex = tileId - bestTileset->firstGid;

                            int worldTileX = chunk.x + cx;
                            int worldTileY = chunk.y + cy;

                            int scrX = (worldTileX * tileWidth) + cameraX;
                            int scrY = (worldTileY * tileHeight) + cameraY;

                            if (scrX >= -tileWidth && scrX < SCRWIDTH &&
                                scrY >= -tileHeight && scrY < SCRHEIGHT)
                            {
                                BlitTile(bestTileset->surface, frameIndex, scrX, scrY);
                            }
                        }
                    }
                }
            }
        }
    }

    void Game::Init()
    {
        LoadTiledMap("maps/superslug.json");

        
    }

} // namespace Tmpl8