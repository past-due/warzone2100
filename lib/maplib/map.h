/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2021  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

#include "map_debug.h"
#include "map_io.h"
#include "terrain_type.h"
#include "map_terrain_types.h"

// MARK: - Various defines needed by both maplib and the game's map-handling code

#define MAP_MAXWIDTH	256
#define MAP_MAXHEIGHT	256
#define MAP_MAXAREA		(256*256)

#define TILE_MAX_HEIGHT (255 * ELEVATION_SCALE)
#define TILE_MIN_HEIGHT 0

/* Flags for whether texture tiles are flipped in X and Y or rotated */
#define TILE_XFLIP		0x8000
#define TILE_YFLIP		0x4000
#define TILE_ROTMASK	0x3000
#define TILE_ROTSHIFT	12
#define TILE_TRIFLIP	0x0800	// This bit describes the direction the tile is split into 2 triangles (same as triangleFlip)
#define TILE_NUMMASK	0x01ff

static inline unsigned short TileNumber_tile(unsigned short tilenumber)
{
	return tilenumber & TILE_NUMMASK;
}

static inline unsigned short TileNumber_texture(unsigned short tilenumber)
{
	return tilenumber & ~TILE_NUMMASK;
}

/*
 * Usage-Example:
 * tile_coordinate = (world_coordinate / TILE_UNITS) = (world_coordinate >> TILE_SHIFT)
 * world_coordinate = (tile_coordinate * TILE_UNITS) = (tile_coordinate << TILE_SHIFT)
 */

/* The shift on a world coordinate to get the tile coordinate */
#define TILE_SHIFT 7

/* The mask to get internal tile coords from a full coordinate */
#define TILE_MASK 0x7f

/* The number of units accross a tile */
#define TILE_UNITS (1<<TILE_SHIFT)

static inline int32_t world_coord(int32_t mapCoord)
{
	return (uint32_t)mapCoord << TILE_SHIFT;  // Cast because -1 << 7 is undefined, but (unsigned)-1 << 7 gives -128 as desired.
}

static inline int32_t map_coord(int32_t worldCoord)
{
	return worldCoord >> TILE_SHIFT;
}

/// Only for graphics!
static inline float map_coordf(int32_t worldCoord)
{
	return (float)worldCoord / TILE_UNITS;
}

static inline int32_t round_to_nearest_tile(int32_t worldCoord)
{
	return (worldCoord + TILE_UNITS/2) & ~TILE_MASK;
}

/* maps a position down to the corner of a tile */
#define map_round(coord) ((coord) & (TILE_UNITS - 1))

//

namespace WzMap {

// MARK: - Handling game.map / MapData files

struct MapData
{
	struct Gateway
	{
		uint8_t x1, y1, x2, y2;
	};

	/* Information stored with each tile */
	struct MapTile
	{
		uint32_t height;	  // The height at the top left of the tile
		uint16_t texture; 	  // Which graphics texture is on this tile
	};

	uint32_t crcSumMapTiles(uint32_t crc);

	uint32_t height = 0;
	uint32_t width = 0;
	uint32_t mapVersion = 0;

	std::vector<Gateway> mGateways;
	std::vector<MapTile> mMapTiles;
};

// Load the map data
std::shared_ptr<MapData> loadMapData(const std::string &mapFile, IOProvider& mapIO, LoggingProtocol* pCustomLogger = nullptr);

// MARK: - Structures for map objects

struct WorldPos
{
	int x = 0;
	int y = 0;
};

struct Structure
{
	optional<uint32_t> id;
	std::string name;
	WorldPos position;
	uint16_t direction = 0;
	int8_t player = 0;  // -1 = scavs
	uint8_t modules = 0; // capacity
};
struct Droid
{
	optional<uint32_t> id;
	std::string name;
	WorldPos position;
	uint16_t direction = 0;
	int8_t player = 0;  // -1 = scavs
};
struct Feature
{
	optional<uint32_t> id; // an explicit feature id# override, used by older formats
	std::string name;
	WorldPos position;
	uint16_t direction = 0;
	optional<int8_t> player;
};

// MARK: - High-level interface for loading a map

enum class MapType
{
	CAMPAIGN,
	SAVEGAME,
	SKIRMISH
};

class Map
{
private:
	Map(const std::string& mapFolderPath, MapType mapType, uint32_t mapMaxPlayers, std::unique_ptr<LoggingProtocol> logger, std::unique_ptr<IOProvider> mapIO = std::unique_ptr<IOProvider>(new StdIOProvider()));

public:
	// Construct an empty Map, for modification
	Map();

	// Load a map from a specified folder path + mayType + maxPlayers + random seed (only used for script-generated maps), optionally supplying:
	// - previewOnly (set to true to shortcut processing of map details that don't factor into preview generation)
	// - a logger
	// - a WzMap::IOProvider
	static std::unique_ptr<Map> loadFromPath(const std::string& mapFolderPath, MapType mapType, uint32_t mapMaxPlayers, uint32_t seed, bool previewOnly = false, std::unique_ptr<LoggingProtocol> logger = nullptr, std::unique_ptr<IOProvider> mapIO = std::unique_ptr<IOProvider>(new StdIOProvider()));

	// High-level data loading functions

	// Get the map data
	// Returns nullptr if constructed for loading and the loading failed
	std::shared_ptr<MapData> mapData();

	// Get the structures
	// Returns nullptr if constructed for loading and the loading failed
	std::shared_ptr<std::vector<Structure>> mapStructures();

	// Get the droids
	// Returns nullptr if constructed for loading and the loading failed
	std::shared_ptr<std::vector<Droid>> mapDroids();

	// Get the features
	// Returns nullptr if constructed for loading and the loading failed
	std::shared_ptr<std::vector<Feature>> mapFeatures();

	// Get the terrain type map
	std::shared_ptr<TerrainTypeData> mapTerrainTypes();

	// Obtaining CRC values
	uint32_t crcSumMapTiles(uint32_t crc);
	uint32_t crcSumStructures(uint32_t crc);
	uint32_t crcSumDroids(uint32_t crc);
	uint32_t crcSumFeatures(uint32_t crc);

	// Other Map instance data
	bool wasScriptGenerated() const { return m_wasScriptGenerated; }
	const std::string& mapFolderPath() const { return m_mapFolderPath; }

private:
	std::string m_mapFolderPath;
	MapType m_mapType;
	uint32_t m_mapMaxPlayers = 8;
	std::unique_ptr<LoggingProtocol> m_logger;
	std::unique_ptr<WzMap::IOProvider> m_mapIO;
	bool m_wasScriptGenerated = false;
	std::shared_ptr<MapData> m_mapData;
	std::shared_ptr<std::vector<Structure>> m_structures;
	std::shared_ptr<std::vector<Droid>> m_droids;
	std::shared_ptr<std::vector<Feature>> m_features;
	std::shared_ptr<TerrainTypeData> m_terrainTypes;
};

} // namespace WzMap
