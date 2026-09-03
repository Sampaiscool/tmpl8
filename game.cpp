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

void Game::LoadTiledMap( const std::string& jsonPath )
{
  tson::Tileson t;
	
	// Parse het bestand
	std::unique_ptr<tson::Map> parsedMap = t.parse( std::filesystem::path( jsonPath ) );

	if (!parsedMap || parsedMap->getStatus() != tson::ParseStatus::OK)
	{
		std::cout << "Tileson kon de map niet laden of het is geen geldige JSON: " << jsonPath << std::endl;
		return;
	}

	tileWidth = parsedMap->getTileSize().x;
	tileHeight = parsedMap->getTileSize().y;

	mapChunks.clear();

	// Loop door alle lagen
	for (auto& layer : parsedMap->getLayers())
	{
		if (layer.getType() == tson::LayerType::TileLayer)
		{
			// Unchunked / Normale lagen
			if (layer.getChunks().empty())
			{
				TileChunk chunk;
				chunk.x = 0;
				chunk.y = 0;
				chunk.width = layer.getSize().x;
				chunk.height = layer.getSize().y;

				// Verkrijg alle tile IDs
				for (auto& [pos, tile] : layer.getTileData())
				{
					chunk.data.push_back( tile->getId() );
				}
				mapChunks.push_back( chunk );
			}
			// Chunked / Infinite map lagen
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
						chunk.data.push_back( static_cast<int>( tileId & 0x0FFFFFFF ) );
					}
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

	int cameraX = 0;
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

				// Met Tileson is een ID van 0 leeg. 
				// 1 is de allereerste tile op je tileset!
				if (tileId > 0)
				{
					int frameIndex = tileId - 1; // 0-indexed voor je blitter

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

void Game::Init()
{
	LoadTiledMap( "../maps/test2.json" );
}

} // namespace Tmpl8
