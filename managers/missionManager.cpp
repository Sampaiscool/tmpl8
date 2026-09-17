#include "precomp.h"
#include "missionManager.h"
#include "renderManager.h"
#include <iostream>

/*
X11 defines a macro called 'None', and tileson has a member with that same
name. Without undefining it first the preprocessor rewrites tileson's code
and the compile fails with errors that make no sense. Only needed on Linux,
but the #ifdef makes it harmless everywhere.
*/
#ifdef None
#undef None
#endif

#include "tileson.hpp"

namespace Tmpl8
{
    /*
    This code:
    Copies the tile ids of one layer or chunk into our own List, so that from
    here on the engine only ever touches List<T> and never an STL container.

    Why a template on the CONTAINER and not on the element?
    tileson stores its layer data and its chunk data in two different
    container types that hold two different int types, and both of them are
    tilesons own storage that we may only read. Templating on the container
    means we never have to name or declare an STL container ourselves: we
    borrow whatever tileson hands us for the length of this one loop and copy
    it straight into our own List<unsigned int>.

    The static_cast to unsigned keeps the bit pattern exactly as it was, flip
    flags included; it does not change any bits, it only changes how we are
    allowed to read them.
    */
    template<typename Container>
    static void FillChunkData(TileChunk& chunk, const Container& data)
    {
        for (size_t d = 0; d < data.size(); ++d)
        {
            chunk.data.push_back(static_cast<unsigned int>(data[d]));
        }
    }

    /*
    This code:
    Gives back all the heap memory the tileset images took.

    Because these are legacy raw Surface pointers (no unique_ptr here) we have
    to clean up by hand, otherwise the pixel buffers stay on the heap for the
    rest of the program wich is a waste of RAM. Setting each pointer to
    nullptr after the delete makes a dangling pointer impossible if this runs
    twice.

    chunks.clear() only resets the element count, the List keeps its block so
    loading the next mission does not have to allocate again from scratch.
    */
    void MissionManager::Unload()
    {
        for (int i = 0; i < tilesets.size(); ++i)
        {
            delete tilesets[i].surface;
            tilesets[i].surface = nullptr;
        }
        tilesets.clear();
        chunks.clear();

        tileWidth = 0;
        tileHeight = 0;
    }

    /*
    This code:
    Initializes the tileson parser to process the JSON file. It parses the raw
    file and builds a complex data tree on the heap. Instead of returning a
    raw pointer it gives back a unique_ptr, so you dont have to keep track of
    it with delete and all that stuff.

    Why a unique_ptr?
    This uses RAII (Resource Acquisition Is Initialization). When 'parsedMap'
    goes out of scope at the end of this function, the heap memory for the
    whole parsed tree is freed automaticly, preventing a leak. We only copy
    out the few things we actually need first.

    If the JSON is missing or corrupted we return false before touching
    anything, so a failed load leaves the previous mission intact instead of
    half destroying it.

    Path processing:
    Tiled exports image paths relative to its own project root (like
    "assets/tiles.png" or even "C:/maps/assets/tiles.png"). We throw away all
    folder prefixes with std::filesystem::path::filename() to leave just the
    pure filename ("tiles.png"), then glue our own relative directory in front
    of it so the path works from the build folder.
    */
    bool MissionManager::Load(const char* jsonPath)
    {
        tson::Tileson tileson;
        std::unique_ptr<tson::Map> parsedMap = tileson.parse(std::filesystem::path(jsonPath));

        // Stop early if the file is broken or missing, to avoid crashing later
        if (!parsedMap || parsedMap->getStatus() != tson::ParseStatus::OK)
        {
            std::cout << "Tileson failed to parse map JSON: " << jsonPath << std::endl;
            return false;
        }

        // Out with the old mission before we put the new one in
        Unload();

        // Global tile size, this is what the zoom is calculated from later
        tileWidth = parsedMap->getTileSize().x;
        tileHeight = parsedMap->getTileSize().y;

        for (size_t i = 0; i < parsedMap->getTilesets().size(); ++i)
        {
            tson::Tileset& parsedTileset = parsedMap->getTilesets()[i];

            LoadedTileset loaded;
            loaded.firstGid = parsedTileset.getFirstgid(); // starting tile id of this sheet

            // Strip Tiled's directory structure, keep only the pure filename
            std::filesystem::path imagePath(parsedTileset.getImagePath().string());
            std::string filename = imagePath.filename().string();

            char localPath[256];
            snprintf(localPath, sizeof(localPath), "../maps/assets/%s", filename.c_str());

            loaded.surface = new Surface(localPath);

            if (loaded.surface->width == 0)
            {
                std::cout << "ERROR: failed to load tileset image at " << localPath << std::endl;
            }

            tilesets.push_back(loaded);
        }

        /*
        This code:
        Checks whether the tilemap is bounded (fixed size) or infinite. An
        infinite map is split by Tiled into small chunks spread over world
        space; a bounded map is one single chunk spanning the full size. We
        support both by turning either one into the same TileChunk struct.

        We only care about tile layers here. Object layers (spawns, triggers,
        collision shapes) are skipped for now, they get their own pass once
        there is gameplay that needs them.
        */
        for (size_t i = 0; i < parsedMap->getLayers().size(); ++i)
        {
            tson::Layer& layer = parsedMap->getLayers()[i];
            if (layer.getType() != tson::LayerType::TileLayer) continue;

            // Bounded map: treat the whole layer as one big chunk at (0,0)
            if (layer.getChunks().empty())
            {
                TileChunk chunk;
                chunk.width = layer.getSize().x;
                chunk.height = layer.getSize().y;
                FillChunkData(chunk, layer.getData());
                chunks.push_back(chunk);
                continue;
            }

            // Infinite map: every chunk has its own offset in world tiles
            for (size_t c = 0; c < layer.getChunks().size(); ++c)
            {
                tson::Chunk& parsedChunk = layer.getChunks()[c];

                TileChunk chunk;
                chunk.x = parsedChunk.getPosition().x;
                chunk.y = parsedChunk.getPosition().y;
                chunk.width = parsedChunk.getSize().x;
                chunk.height = parsedChunk.getSize().y;
                FillChunkData(chunk, parsedChunk.getData());
                chunks.push_back(chunk);
            }
        }

        std::cout << "Mission loaded: " << jsonPath
                  << " (" << chunks.size() << " chunks, "
                  << tilesets.size() << " tilesets)" << std::endl;

        return IsLoaded();
    }

    /*
    This code:
    Finds out wich image a global tile id belongs to.

    Tiled numbers the tiles of every tileset in one continuous sequence: the
    first sheet might own ids 1..4662, the next one starts at 4663, and so on.
    So the sheet we want is the one with the HIGHEST firstGid that is still
    less than or equal to our tile id. We scan them all and keep the best
    match instead of stopping at the first hit, so the order the tilesets
    happen to sit in does not matter.

    Subtracting firstGid turns the global id into a local cell number inside
    that one sheet, wich is exactly what RenderManager::DrawFrame wants.
    */
    Surface* MissionManager::ResolveTile(int tileId, int& frameIndex) const
    {
        const LoadedTileset* best = nullptr;

        for (int t = 0; t < tilesets.size(); ++t)
        {
            if (tilesets[t].firstGid > tileId) continue;
            if (!best || tilesets[t].firstGid > best->firstGid)
            {
                best = &tilesets[t];
            }
        }

        if (!best || !best->surface) return nullptr;

        frameIndex = tileId - best->firstGid;
        return best->surface;
    }

    /*
    This code:
    Walks every tile of every chunk and draws the ones you can see.

    The early 'continue' guards do the heavy lifting for performance: an empty
    cell costs one comparison, and an off screen tile costs two coordinate
    calculations. We only pay the real per pixel price for tiles that are
    actually on screen, wich is what makes a 144 tile wide map cheap to draw.

    Splitting the raw Tiled value:
    0x0FFFFFFF keeps the low 28 bits = the pure tile id.
    0x80000000 is bit 31 = mirror horizontally.
    0x40000000 is bit 30 = mirror vertically.
    (bit 29, 0x20000000, is the diagonal flip wich would need a transpose, we
    dont support that one yet.)
    */
    void MissionManager::Draw(Surface* target, const RenderManager& renderer) const
    {
        if (!IsLoaded()) return;

        for (int c = 0; c < chunks.size(); ++c)
        {
            const TileChunk& chunk = chunks[c];

            for (int cy = 0; cy < chunk.height; ++cy)
            {
                for (int cx = 0; cx < chunk.width; ++cx)
                {
                    // 2D -> 1D offset formula: Y * Width + X
                    int cellIndex = cx + cy * chunk.width;
                    if (cellIndex >= chunk.data.size()) continue;

                    unsigned int rawTile = chunk.data[cellIndex];

                    // 0 means "empty cell" in Tiled, so there is nothing to draw
                    int tileId = static_cast<int>(rawTile & 0x0FFFFFFF);
                    if (tileId == 0) continue;

                    int frameIndex = 0;
                    Surface* sheet = ResolveTile(tileId, frameIndex);
                    if (!sheet) continue;

                    // Tile grid position -> world pixels -> view pixels
                    int viewX = renderer.ToViewX(static_cast<float>((chunk.x + cx) * tileWidth));
                    int viewY = renderer.ToViewY(static_cast<float>((chunk.y + cy) * tileHeight));
                    if (!renderer.IsInView(viewX, viewY, tileWidth, tileHeight)) continue;

                    renderer.DrawFrame(target, sheet, tileWidth, tileHeight, frameIndex,
                                       viewX, viewY,
                                       (rawTile & 0x80000000u) != 0,
                                       (rawTile & 0x40000000u) != 0);
                }
            }
        }
    }
}
