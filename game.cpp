// Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#include "precomp.h"
#include "game.h"
#include <iostream>

#ifdef None
#undef None
#endif

#include "tileson.hpp"

namespace Tmpl8
{
    void Game::LoadTiledMap(const char* jsonPath)
    {
        /*
        This code:
        initializes the Tileson parser to process the JSON file.
        It parses the raw file and builds a complex data tree on top of the
        heap. Instead of returning a raw pointer (*) it uses a unique_ptr, this way
        you dont have to keep track of it with delete and all that stuff.

        Why a unique_ptr?
        This uses RAII (Resource Acquisition Is Initialization). When
        'parsedMap' goes away, the allocated heap memory for the map
        is automatically freed, preventing memory leaks which makes for more free RAM.

        If the JSON is not found or corrupted or something, it triggers an early
        return. This prevents undefined behavior and crashes
        */
        tson::Tileson tileson;
        // Parse the map JSON straight into a unique_ptr on the heap
        std::unique_ptr<tson::Map> parsedMap = tileson.parse(std::filesystem::path(jsonPath));

        // Stop early if file is broken or missing to avoid crashing later
        if (!parsedMap || parsedMap->getStatus() != tson::ParseStatus::OK)
        {
            std::cout << "Tileson failed to parse map JSON: " << jsonPath << std::endl;
            return;
        }

        // Grab global tile dimensions from parsed map
        tileWidth = parsedMap->getTileSize().x;
        tileHeight = parsedMap->getTileSize().y;

        // Reset old map chunks before loading the new one
        mapChunks.clear();

        /*
        This code:
        Because this project uses legacy raw Surface pointers, we
        manually delete all old surface instances before allocating new ones.
        Otherwise, we would leave allocations on the heap that no longer
        have active pointers, wich is a waste of memory.

        The FOR loop iterates over our custom 'loadedTilesets' container. For
        each 'LoadedTileset' (lts), we delete its allocated surface image
        and clear the container.

        Path Processing:
        Tiled often exports image paths as absolute or relative paths from its
        own project root (like "C:/maps/assets/tiles.png"). We remove all folder
        prefixes using std::filesystem::path::filename() to leave just the
        pure filename ("tiles.png"). Then we add our local relative directory
        ("../maps/assets/") using snprintf to reconstruct the full pathname.

        We store 'firstGid' alongside each Surface inside the 'LoadedTileset'
        struct. 'firstGid' represents the starting Global ID index for that
        specific tileset. Saving this allows the engine later to map any global
        tile index to its correct image sheet.
        */
        // Clean up heap memory from previously loaded surfaces
        for (int i = 0; i < loadedTilesets.size(); ++i)
        {
            if (loadedTilesets[i].surface != nullptr)
            {
                delete loadedTilesets[i].surface;
                loadedTilesets[i].surface = nullptr;
            }
        }
        loadedTilesets.clear();

        // Loop through all tilesets defined in the map JSON
        std::vector<tson::Tileset>& tilesets = parsedMap->getTilesets();
        for (size_t i = 0; i < tilesets.size(); ++i)
        {
            tson::Tileset& ts = tilesets[i];
            LoadedTileset lts;
            lts.firstGid = ts.getFirstgid(); // Save starting tile ID for this sheet

            // Strip the directory structure from Tiled, keep only tha pure filename
            std::string rawPath = ts.getImagePath().string();
            std::filesystem::path path(rawPath);
            std::string filename = path.filename().string();

            // Make local relative path to assets
            char imagePath[256];
            snprintf(imagePath, sizeof(imagePath), "../maps/assets/%s", filename.c_str());

            std::cout << "Loading tileset: [" << imagePath << "]" << std::endl;

            // Allocate image surface on the heap
            lts.surface = new Surface(imagePath);

            if (!lts.surface || lts.surface->width == 0)
            {
                std::cout << "ERROR: Failed to load surface at " << imagePath << std::endl;
            }

            loadedTilesets.push_back(lts);
        }

        /*
        This code:
        Checks whether your tilemap is bounded (vast/fixed) or infinite.
        If the map is infinite, Tiled splits it into 16x16 chunks spread
        dynamically across world space. If the map is bounded,
        Tiled generates 1 single chunk spanning the full map dimensions.
        This way we support both bounded and infinite..

        Bitwise Tile Masking:
        Tiled packs additional metadata into tile id ints like as horizontal,
        vertical and diagonal. using the highest 4 bits (31-28).
        To get the pure tile id, wee apply a bitwise AND
        mask: (data & 0x0FFFFFFF). This turns the upper 4 flag bits to 0,
        leaving us with the pure id needed for mapping.

        Notice that we read directly from layer.getData()[d] or chunkData.getData()[d]
        and push it into our custom List<int> container, preventing any external
        std::vector dynamic alloocations inside our game logic
        */
        std::vector<tson::Layer>& layers = parsedMap->getLayers();
        for (size_t i = 0; i < layers.size(); ++i)
        {
            tson::Layer& layer = layers[i];

            if (layer.getType() == tson::LayerType::TileLayer)
            {
                std::vector<tson::Chunk>& chunks = layer.getChunks();

                // Bounded map: treat layer as 1 big single chunk at (0,0)
                if (chunks.empty())
                {
                    TileChunk chunk;
                    chunk.x = 0;
                    chunk.y = 0;
                    chunk.width = layer.getSize().x;
                    chunk.height = layer.getSize().y;

                    // Strip rotation flags (upper 4 bits) and store pure ID into my crazy List
                    for (size_t d = 0; d < layer.getData().size(); ++d)
                    {
                        chunk.data.push_back(static_cast<int>(layer.getData()[d] & 0x0FFFFFFF));
                    }
                    mapChunks.push_back(chunk);
                }
                // Infinite map: load separate 16x16 grid chunks dynamically hohoho
                else
                {
                    for (size_t c = 0; c < chunks.size(); ++c)
                    {
                        tson::Chunk& chunkData = chunks[c];
                        TileChunk chunk;
                        chunk.x = chunkData.getPosition().x;
                        chunk.y = chunkData.getPosition().y;
                        chunk.width = chunkData.getSize().x;
                        chunk.height = chunkData.getSize().y;

                        // Clean upper bits and store directly into our custom List<int>
                        for (size_t d = 0; d < chunkData.getData().size(); ++d)
                        {
                            chunk.data.push_back(static_cast<int>(chunkData.getData()[d] & 0x0FFFFFFF));
                        }
                        mapChunks.push_back(chunk);
                    }
                }
            }
        }
    }

    /*
    This code:
    2D pixels are stored sequentially in RAM as a 1D array
    (row by row). We use standard (the one we talked about) 2D-to-1D index offset formulas:
    Index = Y * Width + X

    Boundary Protection & Clipping:
    It checks wheter the drawn pixel would be in/out of the screen
    preventing pixels from being drawn outside to screen to prevent errors

    Color & Transparency Logic:
    We remove the alpha channel (c & 0xFFFFFF) to inspect pure RGB values.
    If a pixel is black (0x000000) or transparent, it is skipped.
    For valid pixels, we force the highest 8 bits (alpha bits) to max
    (c | 0xFF000000) to guarantee full opacity when written directly to
    the screen's pixel buffer array.
    */
    void Game::BlitTile(Surface* targetSurface, int frameIndex, int dstX, int dstY)
    {
        if (!targetSurface || tileWidth <= 0) return;

        // Calculate columns/rows on the tileset texture sheet
        int tileCols = targetSurface->width / tileWidth;
        if (tileCols <= 0) return;

        int tileRows = targetSurface->height / tileHeight;
        int maxFrames = tileCols * tileRows;

        if (frameIndex < 0 || frameIndex >= maxFrames) return;

        // Convert 1D frame index into 2D X/Y source coordinates inside tileset
        int srcX = (frameIndex % tileCols) * tileWidth;
        int srcY = (frameIndex / tileCols) * tileHeight;

        uint* dst = screen->pixels; // Direct pointer to screen pixel buffer

        // Copy pixels line by line
        for (int y = 0; y < tileHeight; ++y)
        {
            // This is Checking against the bounds Y
            int sy = srcY + y;
            if (sy >= targetSurface->height) break; // Source Y bound check
            int dy = dstY + y;
            if (dy < 0 || dy >= screen->height) continue; // Screen Y bound check (clipping)

            for (int x = 0; x < tileWidth; ++x)
            {
                // This is Checking against the bounds Y
                int sx = srcX + x;
                if (sx >= targetSurface->width) break; // Source X bound check
                int dx = dstX + x;
                if (dx < 0 || dx >= screen->width) continue; // Screen X bound check (clipping)

                // 2D -> 1D offset formula: Y * Width + X
                uint c = targetSurface->pixels[sy * targetSurface->width + sx];

                // Skip black/transparent background pixels
                if ((c & 0xFFFFFF) != 0)
                {
                    // Force alpha to 255 (0xFF) so the pixel is fully opaque
                    dst[dy * screen->width + dx] = c | 0xFF000000;
                }
            }
        }
    }

    /*
    This code:
    Handles camera movement input and iterates over all chunks in world space.
    It uses explicit index-based loops over our custom List<T> structures

    For each tile, it selects the correct tileset sheet based on 'firstGid',
    calculates world coordinates shifted by the camera offset, and invokes
    BlitTile for on-screen tiles.
    */
    void Game::Tick(float deltaTime)
    {
        screen->Clear(0x1e1e1e); // Clear screen with dark background

        // Delta-time normalization
        float dt = (deltaTime > 1.0f) ? (deltaTime / 1000.0f) : deltaTime;
        if (dt > 0.05f) dt = 0.05f;

        const float speed = 400.0f;

        // Camera controls via array lookup
        if (keys['a'] || keys['A'] || keys[GLFW_KEY_LEFT])  cameraX -= speed * dt;
        if (keys['d'] || keys['D'] || keys[GLFW_KEY_RIGHT]) cameraX += speed * dt;
        if (keys['w'] || keys['W'] || keys[GLFW_KEY_UP])    cameraY -= speed * dt;
        if (keys['s'] || keys['S'] || keys[GLFW_KEY_DOWN])  cameraY += speed * dt;

        if (loadedTilesets.empty() || mapChunks.empty()) return;

        // Loop through chunks using custom List index iteration
        for (int c = 0; c < mapChunks.size(); ++c)
        {
            const TileChunk& chunk = mapChunks[c];

            for (int cy = 0; cy < chunk.height; ++cy)
            {
                for (int cx = 0; cx < chunk.width; ++cx)
                {
                    // Convert 2D tile position inside chunk to 1D List index
                    int tileIndex = cx + cy * chunk.width;
                    if (tileIndex >= chunk.data.size()) continue;

                    int tileId = chunk.data[tileIndex];

                    // Skip empty tile slots
                    if (tileId > 0)
                    {
                        LoadedTileset* bestTileset = nullptr;

                        // Find which tileset image owns this specific tileId
                        for (int t = 0; t < loadedTilesets.size(); ++t)
                        {
                            LoadedTileset& ts = loadedTilesets[t];
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
                            // Calculate local frame index inside the matched tileset
                            int frameIndex = tileId - bestTileset->firstGid;

                            // Calculate final screen position factoring in camera
                            int worldTileX = chunk.x + cx;
                            int worldTileY = chunk.y + cy;

                            int scrX = static_cast<int>((worldTileX * tileWidth) + cameraX);
                            int scrY = static_cast<int>((worldTileY * tileHeight) + cameraY);

                            // Frustum culling: render only tiles visible on screen
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
        LoadTiledMap("../maps/superslug.json");

        cameraX = 0.0f;
        cameraY = 0.0f;
    }

} // namespace Tmpl8
