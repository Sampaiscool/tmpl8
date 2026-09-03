// Tiled:emplate, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#pragma once

#include <vector>
#include <string>

namespace Tmpl8
{



struct TileChunk {
    int x = 0;          // Tile offset X
    int y = 0;          // Tile offset Y
    int width = 0;      // Chunk breedte in tiles (meestal 16)
    int height = 0;     // Chunk hoogte in tiles (meestal 16)
    std::vector<int> data;
};

class Game : public TheApp
{
public:
	// game flow methods
	void Init();
	void Tick( float deltaTime );
	void Shutdown() { delete tileSetSurface; }
	void BlitTile( int frameIndex, int dstX, int dstY );
	
	// input handling
	void MouseUp( int ) { /* implement if you want to detect mouse button presses */ }
	void MouseDown( int ) { /* implement if you want to detect mouse button presses */ }
	void MouseMove( int x, int y ) { mousePos.x = x, mousePos.y = y; }
	void MouseWheel( float ) { /* implement if you want to handle the mouse wheel */ }
	void KeyUp( int ) { /* implement if you want to handle keys */ }
	void KeyDown( int ) { /* implement if you want to handle keys */ }

	// Tiled Loader helper
	void LoadTiledMap( const std::string& jsonPath );

	// data members
	int2 mousePos;

private:
	// Map Properties
	int mapWidth = 0;
	int mapHeight = 0;
	int tileWidth = 0;
	int tileHeight = 0;
  int firstGid = 1;
  std::vector<TileChunk> mapChunks;
	// Tileset rendering
	Surface* tileSetSurface = nullptr;
	int tilesetCols = 0;
};

} // namespace Tmpl8
