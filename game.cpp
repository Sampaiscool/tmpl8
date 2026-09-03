// Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#include "precomp.h"
#include "game.h"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Tmpl8
{

void Game::LoadTiledMap( const std::string& jsonPath )
{
	std::ifstream file( jsonPath );
	if (!file.is_open()) return;

	json mapJson;
	file >> mapJson;

	tileWidth = mapJson.value( "tilewidth", 16 );
	tileHeight = mapJson.value( "tileheight", 16 );

	// Pak de firstgid van de tileset uit de JSON
	if (mapJson.contains("tilesets") && !mapJson["tilesets"].empty())
	{
		firstGid = mapJson["tilesets"][0].value("firstgid", 1);
	}

	mapChunks.clear();

	if (mapJson.contains( "layers" ))
	{
		for (const auto& layer : mapJson["layers"])
		{
			std::string layerType = layer.value( "type", layer.value( "class", "" ) );

			if (layerType == "tilelayer")
			{
				if (layer.contains( "chunks" ))
				{
					for (const auto& chunkJson : layer["chunks"])
					{
						TileChunk chunk;
						chunk.x = chunkJson.value( "x", 0 );
						chunk.y = chunkJson.value( "y", 0 );
						chunk.width = chunkJson.value( "width", 0 );
						chunk.height = chunkJson.value( "height", 0 );
						chunk.data = chunkJson["data"].get<std::vector<int>>();
						mapChunks.push_back( chunk );
					}
				}
				else if (layer.contains( "data" ))
				{
					TileChunk chunk;
					chunk.x = 0;
					chunk.y = 0;
					chunk.width = mapJson.value( "width", 0 );
					chunk.height = mapJson.value( "height", 0 );
					chunk.data = layer["data"].get<std::vector<int>>();
					mapChunks.push_back( chunk );
				}
			}
		}
	}

	tileSetSurface = new Surface( "../assets/MetalSlugBackground.png" );
	if (tileSetSurface->width > 0 && tileWidth > 0)
	{
		tilesetCols = tileSetSurface->width / tileWidth;
	}
}

void Game::BlitTile( int frameIndex, int dstX, int dstY )
{
	if (!tileSetSurface || tilesetCols <= 0) return;
	int tileCols = tilesetCols;
	int tileRows = tileSetSurface->height / tileHeight;
	int maxFrames = tileCols * tileRows;
	if (frameIndex < 0 || frameIndex >= maxFrames) return;

	int srcX = (frameIndex % tileCols) * tileWidth;
	int srcY = (frameIndex / tileCols) * tileHeight;

	uint* src = tileSetSurface->pixels + srcY * tileSetSurface->width + srcX;
	uint* dst = screen->pixels + dstY * screen->width + dstX;

	for (int y = 0; y < tileHeight; ++y)
	{
		int sy = srcY + y;
		if (sy >= tileSetSurface->height) break;
		int dy = dstY + y;
		if (dy < 0 || dy >= screen->height) { src += tileSetSurface->width; continue; }

		for (int x = 0; x < tileWidth; ++x)
		{
			int sx = srcX + x;
			if (sx >= tileSetSurface->width) break;
			int dx = dstX + x;
			if (dx < 0 || dx >= screen->width) continue;

			uint c = src[x];
			if (c & 0xff000000) dst[dy * screen->width + dx] = c;
		}
	}
}

void Game::Tick( float /* deltaTime */ )
{
	screen->Clear( 0x1e1e1e );

	if (!tileSetSurface || mapChunks.empty()) return;

	int cameraX = 100;
	int cameraY = 0;

	for (const auto& chunk : mapChunks)
	{
		for (int cy = 0; cy < chunk.height; ++cy)
		{
			for (int cx = 0; cx < chunk.width; ++cx)
			{
				int tileIndex = cx + cy * chunk.width;

				unsigned int rawTileId = (unsigned int)chunk.data[tileIndex];
				unsigned int cleanTileId = rawTileId & 0x0FFFFFFF;

				if (cleanTileId >= firstGid)
				{
					int frameIndex = cleanTileId - firstGid;

					int worldTileX = chunk.x + cx;
					int worldTileY = chunk.y + cy;

					int scrX = (worldTileX * tileWidth) + cameraX;
					int scrY = (worldTileY * tileHeight) + cameraY;

					if (scrX >= -tileWidth && scrX < SCRWIDTH &&
					    scrY >= -tileHeight && scrY < SCRHEIGHT)
					{
						BlitTile( frameIndex, scrX, scrY );
					}
				}
			}
		}
	}
}

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
void Game::Init()
{
	// Laad je JSON-kaart bij de start
	LoadTiledMap( "../maps/test.json" );
}

} // namespace Tmpl8
