#include "Map.h"
#include <cstdlib>
#include <ctime>

// Simple MapManager implementation: 50x50 tiles
static void SeedRandOnceMap() {
	static bool seeded = false;
	if (!seeded) {
		std::srand(static_cast<unsigned int>(std::time(nullptr)));
		seeded = true;
	}
}

MapManager::MapManager() {
	tileSize = 16; // each tile is 16x16 pixels
	levelKey.active = false;
	buffItems.clear();
	// initialize empty map
	for (int y = 0; y < 50; ++y) {
		for (int x = 0; x < 50; ++x) {
			worldMap[y][x] = 0;
		}
	}
}

MapManager::~MapManager() {}

void MapManager::LoadMapData(int level) {
	SeedRandOnceMap();
	// Clear map
	for (int y = 0; y < 50; ++y) {
		for (int x = 0; x < 50; ++x) {
			worldMap[y][x] = 0;
		}
	}

	// set solid border walls
	for (int x = 0; x < 50; ++x) {
		worldMap[0][x] = 1;
		worldMap[49][x] = 1;
	}
	for (int y = 0; y < 50; ++y) {
		worldMap[y][0] = 1;
		worldMap[y][49] = 1;
	}

	// scatter some random walls for a basic maze-like feel
	int walls = 300 + (level - 1) * 100;
	for (int i = 0; i < walls; ++i) {
		int rx = std::rand() % 50;
		int ry = std::rand() % 50;
		if (rx > 0 && rx < 49 && ry > 0 && ry < 49) worldMap[ry][rx] = 1;
	}
}

void MapManager::SpawnItems() {
	// not used in this change; placeholder
}

Vector2 MapManager::GetRandomFreePosition() {
	SeedRandOnceMap();
	// try random sampling first
	for (int attempt = 0; attempt < 500; ++attempt) {
		int tx = std::rand() % 50;
		int ty = std::rand() % 50;
		if (worldMap[ty][tx] == 0) {
			float x = tx * tileSize + tileSize * 0.5f;
			float y = ty * tileSize + tileSize * 0.5f;
			return Vector2{ x, y };
		}
	}
	// fallback linear search
	for (int y = 0; y < 50; ++y) {
		for (int x = 0; x < 50; ++x) {
			if (worldMap[y][x] == 0) {
				float fx = x * tileSize + tileSize * 0.5f;
				float fy = y * tileSize + tileSize * 0.5f;
				return Vector2{ fx, fy };
			}
		}
	}
	// as a last resort return center of screen-like area
	return Vector2{ tileSize * 0.5f, tileSize * 0.5f };
}

bool MapManager::CheckWallCollision(Vector2 pos, Vector2 size) {
	// treat pos as center, size as width/height
	float left = pos.x - size.x * 0.5f;
	float right = pos.x + size.x * 0.5f;
	float top = pos.y - size.y * 0.5f;
	float bottom = pos.y + size.y * 0.5f;

	int tx0 = (int)(left / tileSize);
	int tx1 = (int)(right / tileSize);
	int ty0 = (int)(top / tileSize);
	int ty1 = (int)(bottom / tileSize);

	if (tx0 < 0) tx0 = 0; if (ty0 < 0) ty0 = 0;
	if (tx1 > 49) tx1 = 49; if (ty1 > 49) ty1 = 49;

	for (int y = ty0; y <= ty1; ++y) {
		for (int x = tx0; x <= tx1; ++x) {
			if (worldMap[y][x] == 1) return true;
		}
	}
	return false;
}

void MapManager::DrawMap(int winWidth, int winHeight) {
	// draw all walls; simple implementation
	for (int y = 0; y < 50; ++y) {
		for (int x = 0; x < 50; ++x) {
			if (worldMap[y][x] == 1) {
				DrawRectangle(x * tileSize, y * tileSize, tileSize, tileSize, DARKGRAY);
			}
		}
	}
}
