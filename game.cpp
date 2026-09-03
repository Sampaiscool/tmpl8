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
	if (tileSetSurface->width > 0 && tileHeight > 0)
	{
		int tilesX = tileSetSurface->width / tileWidth;
		int tilesY = tileSetSurface->height / tileHeight;
		tileSetSprite = new Sprite( tileSetSurface, std::max( 1, tilesX * tilesY ) );
	}
}

void Game::Tick( float /* deltaTime */ )
{
	screen->Clear( 0x1e1e1e );

	if (!tileSetSprite || mapChunks.empty()) return;

	int maxFrames = (tileSetSurface->width / tileWidth) * (tileSetSurface->height / tileHeight);

	// Als de map op negatieve of hoge coördinaten getekend is in Tiled, 
	// kun je hiermee de camera verschuiven (bijv. 200px naar rechts/beneden offsetten):
	int cameraX = 0; 
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
					// Converteer GID naar Sprite frame-index
					int frameIndex = cleanTileId - firstGid;

					if (frameIndex < maxFrames)
					{
						int worldTileX = chunk.x + cx;
						int worldTileY = chunk.y + cy;

						int screenX = (worldTileX * tileWidth) + cameraX;
						int screenY = (worldTileY * tileHeight) + cameraY;

						// Teken alleen als de tile binnen het zichtbare scherm valt
						if (screenX >= -tileWidth && screenX < SCRWIDTH &&
						    screenY >= -tileHeight && screenY < SCRHEIGHT)
						{
							tileSetSprite->SetFrame( frameIndex );
							tileSetSprite->Draw( screen, screenX, screenY );
						}
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
