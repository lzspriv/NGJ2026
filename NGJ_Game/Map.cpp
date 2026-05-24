#include "Map.h"
#include <math.h>
#include <cstdlib>
#include <ctime>

Map::Map(int screenWidth, int screenHeight) {
	tileWidth = 50;
	tileHeight = 50;
	GenerateDefaultMapFiles();
	InitLayer(DungeonLayer::LAYER_1);
}

Map::~Map() {}

void Map::GenerateDefaultMapFiles() {
	const int W = 38;
	const int H = 22;

	Color WHT = { 255, 255, 255, 255 };
	Color BLK = { 0, 0, 0, 255 };
	Color GRN = { 0, 255, 0, 255 };
	Color BLU = { 0, 0, 255, 255 };
	Color RED_ = { 255, 0, 0, 255 };
	Color PRP = { 200, 0, 200, 255 };

	if (FileExists("map1.png") && FileExists("map2.png") && FileExists("map3.png") && FileExists("boss.png")) {
		return;
	}

	srand((unsigned)time(NULL));

	// Map 1: Classic Maze
	std::vector<Color> m1(W * H, BLK);
	for (int y = 0; y < H; y++) {
		for (int x = 0; x < W; x++) {
			if (y > 0 && y < H - 1 && x > 0 && x < W - 1) m1[y * W + x] = WHT;
			if (y == 7 && x > 5 && x < 33) m1[y * W + x] = BLK;
			if (y == 15 && x > 3 && x < 30) m1[y * W + x] = BLK;
			if (x == 12 && y > 3 && y < 13) m1[y * W + x] = BLK;
			if (x == 25 && y > 8 && y < 18) m1[y * W + x] = BLK;
		}
	}
	m1[2 * W + 2] = WHT;
	m1[2 * W + 3] = WHT;
	m1[3 * W + 2] = WHT;
	m1[3 * W + 3] = WHT;
	m1[2 * W + 2] = GRN;

	// Add random keys to map1
	for (int keyCount = 0; keyCount < 3; keyCount++) {
		bool placed = false;
		for (int attempts = 0; attempts < 100 && !placed; attempts++) {
			int randomX = 4 + (rand() % (W - 8));
			int randomY = 4 + (rand() % (H - 8));
			int idx = randomY * W + randomX;

			if (m1[idx].r == 255 && m1[idx].g == 255 && m1[idx].b == 255) {
				bool tooCloseToStart = false;
				for (int sy = 1; sy < 5; sy++) {
					for (int sx = 1; sx < 5; sx++) {
						if (randomX == sx && randomY == sy) tooCloseToStart = true;
					}
				}

				bool overlapped = false;
				for (int y = 0; y < H && !overlapped; y++) {
					for (int x = 0; x < W && !overlapped; x++) {
						Color c = m1[y * W + x];
						if ((c.b > 200 && c.r < 50 && c.g < 50) || 
							(c.r > 150 && c.g < 50 && c.b > 150)) {
							if (x == randomX && y == randomY) overlapped = true;
						}
					}
				}

				if (!tooCloseToStart && !overlapped) {
					m1[idx] = BLU;
					placed = true;
				}
			}
		}
	}

	// Add random chest to map1
	bool chestPlaced = false;
	for (int attempts = 0; attempts < 100 && !chestPlaced; attempts++) {
		int randomX = 4 + (rand() % (W - 8));
		int randomY = 4 + (rand() % (H - 8));
		int idx = randomY * W + randomX;

		if (m1[idx].r == 255 && m1[idx].g == 255 && m1[idx].b == 255) {
			bool tooCloseToStart = false;
			for (int sy = 1; sy < 5; sy++) {
				for (int sx = 1; sx < 5; sx++) {
					if (randomX == sx && randomY == sy) tooCloseToStart = true;
				}
			}

			bool overlapped = false;
			for (int y = 0; y < H && !overlapped; y++) {
				for (int x = 0; x < W && !overlapped; x++) {
					Color c = m1[y * W + x];
					if ((c.b > 200 && c.r < 50 && c.g < 50) || 
						(c.r > 150 && c.g < 50 && c.b > 150)) {
						if (x == randomX && y == randomY) overlapped = true;
					}
				}
			}

			if (!tooCloseToStart && !overlapped) {
				m1[idx] = PRP;
				chestPlaced = true;
			}
		}
	}

	// Map 2: Spiral Maze
	std::vector<Color> m2(W * H, BLK);
	for (int y = 0; y < H; y++) {
		for (int x = 0; x < W; x++) {
			if (!(y == 0 || y == H - 1 || x == 0 || x == W - 1)) m2[y * W + x] = WHT;
			if (y == 3 && x >= 3 && x <= W - 4) m2[y * W + x] = BLK;
			if (x == W - 4 && y >= 3 && y <= H - 4) m2[y * W + x] = BLK;
			if (y == H - 4 && x >= 3 && x <= W - 4) m2[y * W + x] = BLK;
			if (x == 3 && y >= 6 && y <= H - 4) m2[y * W + x] = BLK;
			if (y == 6 && x >= 3 && x <= W - 7) m2[y * W + x] = BLK;
			if (x == W - 7 && y >= 6 && y <= H - 7) m2[y * W + x] = BLK;
			if (y == H - 7 && x >= 6 && x <= W - 7) m2[y * W + x] = BLK;
		}
	}
	m2[2 * W + 2] = WHT;
	m2[2 * W + 3] = WHT;
	m2[3 * W + 2] = WHT;
	m2[3 * W + 3] = WHT;
	m2[2 * W + 2] = GRN;

	// Add random keys to map2
	for (int keyCount = 0; keyCount < 3; keyCount++) {
		bool placed = false;
		for (int attempts = 0; attempts < 100 && !placed; attempts++) {
			int randomX = 4 + (rand() % (W - 8));
			int randomY = 4 + (rand() % (H - 8));
			int idx = randomY * W + randomX;

			if (m2[idx].r == 255 && m2[idx].g == 255 && m2[idx].b == 255) {
				bool tooCloseToStart = false;
				for (int sy = 1; sy < 5; sy++) {
					for (int sx = 1; sx < 5; sx++) {
						if (randomX == sx && randomY == sy) tooCloseToStart = true;
					}
				}

				bool overlapped = false;
				for (int y = 0; y < H && !overlapped; y++) {
					for (int x = 0; x < W && !overlapped; x++) {
						Color c = m2[y * W + x];
						if ((c.b > 200 && c.r < 50 && c.g < 50) || 
							(c.r > 150 && c.g < 50 && c.b > 150)) {
							if (x == randomX && y == randomY) overlapped = true;
						}
					}
				}

				if (!tooCloseToStart && !overlapped) {
					m2[idx] = BLU;
					placed = true;
				}
			}
		}
	}

	chestPlaced = false;
	for (int attempts = 0; attempts < 100 && !chestPlaced; attempts++) {
		int randomX = 4 + (rand() % (W - 8));
		int randomY = 4 + (rand() % (H - 8));
		int idx = randomY * W + randomX;

		if (m2[idx].r == 255 && m2[idx].g == 255 && m2[idx].b == 255) {
			bool tooCloseToStart = false;
			for (int sy = 1; sy < 5; sy++) {
				for (int sx = 1; sx < 5; sx++) {
					if (randomX == sx && randomY == sy) tooCloseToStart = true;
				}
			}

			bool overlapped = false;
			for (int y = 0; y < H && !overlapped; y++) {
				for (int x = 0; x < W && !overlapped; x++) {
					Color c = m2[y * W + x];
					if ((c.b > 200 && c.r < 50 && c.g < 50) || 
						(c.r > 150 && c.g < 50 && c.b > 150)) {
						if (x == randomX && y == randomY) overlapped = true;
					}
				}
			}

			if (!tooCloseToStart && !overlapped) {
				m2[idx] = PRP;
				chestPlaced = true;
			}
		}
	}

	// Map 3: Rooms Maze
	std::vector<Color> m3(W * H, BLK);
	for (int y = 0; y < H; y++) {
		for (int x = 0; x < W; x++) {
			if (!(y == 0 || y == H - 1 || x == 0 || x == W - 1)) m3[y * W + x] = WHT;
			if (x == 12 || x == 25) m3[y * W + x] = BLK;
			if (y == 11) m3[y * W + x] = BLK;
		}
	}
	m3[5 * W + 12] = WHT;
	m3[16 * W + 12] = WHT;
	m3[6 * W + 25] = WHT;
	m3[11 * W + 6] = WHT;
	m3[11 * W + 18] = WHT;
	m3[11 * W + 32] = WHT;
	m3[2 * W + 2] = WHT;
	m3[2 * W + 3] = WHT;
	m3[3 * W + 2] = WHT;
	m3[3 * W + 3] = WHT;
	m3[2 * W + 2] = GRN;

	// Add random keys to map3
	for (int keyCount = 0; keyCount < 3; keyCount++) {
		bool placed = false;
		for (int attempts = 0; attempts < 100 && !placed; attempts++) {
			int randomX = 4 + (rand() % (W - 8));
			int randomY = 4 + (rand() % (H - 8));
			int idx = randomY * W + randomX;

			if (m3[idx].r == 255 && m3[idx].g == 255 && m3[idx].b == 255) {
				bool tooCloseToStart = false;
				for (int sy = 1; sy < 5; sy++) {
					for (int sx = 1; sx < 5; sx++) {
						if (randomX == sx && randomY == sy) tooCloseToStart = true;
					}
				}

				bool overlapped = false;
				for (int y = 0; y < H && !overlapped; y++) {
					for (int x = 0; x < W && !overlapped; x++) {
						Color c = m3[y * W + x];
						if ((c.b > 200 && c.r < 50 && c.g < 50) || 
							(c.r > 150 && c.g < 50 && c.b > 150)) {
							if (x == randomX && y == randomY) overlapped = true;
						}
					}
				}

				if (!tooCloseToStart && !overlapped) {
					m3[idx] = BLU;
					placed = true;
				}
			}
		}
	}

	chestPlaced = false;
	for (int attempts = 0; attempts < 100 && !chestPlaced; attempts++) {
		int randomX = 4 + (rand() % (W - 8));
		int randomY = 4 + (rand() % (H - 8));
		int idx = randomY * W + randomX;

		if (m3[idx].r == 255 && m3[idx].g == 255 && m3[idx].b == 255) {
			bool tooCloseToStart = false;
			for (int sy = 1; sy < 5; sy++) {
				for (int sx = 1; sx < 5; sx++) {
					if (randomX == sx && randomY == sy) tooCloseToStart = true;
				}
			}

			bool overlapped = false;
			for (int y = 0; y < H && !overlapped; y++) {
				for (int x = 0; x < W && !overlapped; x++) {
					Color c = m3[y * W + x];
					if ((c.b > 200 && c.r < 50 && c.g < 50) || 
						(c.r > 150 && c.g < 50 && c.b > 150)) {
						if (x == randomX && y == randomY) overlapped = true;
					}
				}
			}

			if (!tooCloseToStart && !overlapped) {
				m3[idx] = PRP;
				chestPlaced = true;
			}
		}
	}

	// Map 4: Boss Arena
	std::vector<Color> mb(W * H, WHT);
	for (int y = 0; y < H; y++) {
		for (int x = 0; x < W; x++) {
			if (x < 8 && y < 6) mb[y * W + x] = BLK;
			if (x > 29 && y < 6) mb[y * W + x] = BLK;
			if (x < 8 && y > 15) mb[y * W + x] = BLK;
			if (x > 29 && y > 15) mb[y * W + x] = BLK;
			if (y == 0 || y == H - 1 || x == 0 || x == W - 1) mb[y * W + x] = BLK;
		}
	}
	mb[2 * W + 2] = WHT;
	mb[2 * W + 3] = WHT;
	mb[3 * W + 2] = WHT;
	mb[3 * W + 3] = WHT;
	mb[2 * W + 2] = GRN;

	// Add random keys to boss map
	for (int keyCount = 0; keyCount < 3; keyCount++) {
		bool placed = false;
		for (int attempts = 0; attempts < 100 && !placed; attempts++) {
			int randomX = 4 + (rand() % (W - 8));
			int randomY = 4 + (rand() % (H - 8));
			int idx = randomY * W + randomX;

			if (mb[idx].r == 255 && mb[idx].g == 255 && mb[idx].b == 255) {
				bool tooCloseToStart = false;
				for (int sy = 1; sy < 5; sy++) {
					for (int sx = 1; sx < 5; sx++) {
						if (randomX == sx && randomY == sy) tooCloseToStart = true;
					}
				}

				bool overlapped = false;
				for (int y = 0; y < H && !overlapped; y++) {
					for (int x = 0; x < W && !overlapped; x++) {
						Color c = mb[y * W + x];
						if ((c.b > 200 && c.r < 50 && c.g < 50) || 
							(c.r > 150 && c.g < 50 && c.b > 150)) {
							if (x == randomX && y == randomY) overlapped = true;
						}
					}
				}

				if (!tooCloseToStart && !overlapped) {
					mb[idx] = BLU;
					placed = true;
				}
			}
		}
	}

	chestPlaced = false;
	for (int attempts = 0; attempts < 100 && !chestPlaced; attempts++) {
		int randomX = 4 + (rand() % (W - 8));
		int randomY = 4 + (rand() % (H - 8));
		int idx = randomY * W + randomX;

		if (mb[idx].r == 255 && mb[idx].g == 255 && mb[idx].b == 255) {
			bool tooCloseToStart = false;
			for (int sy = 1; sy < 5; sy++) {
				for (int sx = 1; sx < 5; sx++) {
					if (randomX == sx && randomY == sy) tooCloseToStart = true;
				}
			}

			bool overlapped = false;
			for (int y = 0; y < H && !overlapped; y++) {
				for (int x = 0; x < W && !overlapped; x++) {
					Color c = mb[y * W + x];
					if ((c.b > 200 && c.r < 50 && c.g < 50) || 
						(c.r > 150 && c.g < 50 && c.b > 150)) {
						if (x == randomX && y == randomY) overlapped = true;
					}
				}
			}

			if (!tooCloseToStart && !overlapped) {
				mb[idx] = PRP;
				chestPlaced = true;
			}
		}
	}

	const char* names[4] = { "map1.png", "map2.png", "map3.png", "boss.png" };
	std::vector<Color>* dataPtrs[4] = { &m1, &m2, &m3, &mb };

	for (int i = 0; i < 4; i++) {
		std::vector<Color>& mapData = *dataPtrs[i];

		// Place door randomly
		bool doorPlaced = false;
		for (int attempts = 0; attempts < 500 && !doorPlaced; attempts++) {
			int randomX = 3 + (rand() % (W - 6));
			int randomY = 3 + (rand() % (H - 6));
			int idx = randomY * W + randomX;

			if (mapData[idx].r == 255 && mapData[idx].g == 255 && mapData[idx].b == 255) {
				bool tooCloseToStart = false;
				for (int y = 0; y < H; y++) {
					for (int x = 0; x < W; x++) {
						if (mapData[y * W + x].r == 0 && mapData[y * W + x].g > 200 && mapData[y * W + x].b == 0) {
							int distSq = (randomX - x) * (randomX - x) + (randomY - y) * (randomY - y);
							if (distSq < 25) {
								tooCloseToStart = true;
							}
						}
					}
				}

				if (!tooCloseToStart) {
					mapData[idx] = RED_;
					doorPlaced = true;
				}
			}
		}

		if (!doorPlaced) {
			mapData[11 * W + 22] = RED_;
		}

		// Export image
		Image img = GenImageColor(W, H, BLK);
		Color* imgData = (Color*)img.data;

		for (int j = 0; j < W * H; j++) {
			imgData[j] = mapData[j];
		}

		ExportImage(img, names[i]);
		UnloadImage(img);
	}
}

void Map::LoadMapFromImageFile(const char* fileName) {
	Image mapImg = LoadImage(fileName);
	mapRows = mapImg.height;
	mapCols = mapImg.width;

	grid.assign(mapRows, std::vector<int>(mapCols, 0));
	keyPositions.clear();
	keyCollected.clear();
	chestPositions.clear();
	chestOpened.clear();

	for (int y = 0; y < mapRows; y++) {
		for (int x = 0; x < mapCols; x++) {
			Color pixel = GetImageColor(mapImg, x, y);

			if (pixel.r == 0 && pixel.g == 0 && pixel.b == 0) {
				grid[y][x] = 1;
			}
			else if (pixel.g > 200 && pixel.r < 50 && pixel.b < 50) {
				playerStartPos = { (float)x * tileWidth + tileWidth / 2.0f, (float)y * tileHeight + tileHeight / 2.0f };
				grid[y][x] = 0;
			}
			else if (pixel.b > 200 && pixel.r < 50 && pixel.g < 50) {
				// Blue = Keys
				keyPositions.push_back({ (float)x * tileWidth + tileWidth / 2.0f, (float)y * tileHeight + tileHeight / 2.0f });
				keyCollected.push_back(false);
				grid[y][x] = 0;
			}
			else if (pixel.r > 150 && pixel.g < 50 && pixel.b > 150) {
				// Purple = Chest
				chestPositions.push_back({ (float)x * tileWidth + tileWidth / 2.0f, (float)y * tileHeight + tileHeight / 2.0f });
				chestOpened.push_back(false);
				grid[y][x] = 0;
			}
			else if (pixel.r > 200 && pixel.g < 50 && pixel.b < 50) {
				// Red = Door
				doorPos = { (float)x * tileWidth + tileWidth / 2.0f, (float)y * tileHeight + tileHeight / 2.0f };
				grid[y][x] = 0;
			}
			else {
				grid[y][x] = 0;
			}
		}
	}
	UnloadImage(mapImg);
}

void Map::InitLayer(DungeonLayer layer) {
	currentLayer = layer;
	keysCollected = 0;
	doorUnlocked = false;
	playerStartPos = { 120.0f, 120.0f };

	if (layer == DungeonLayer::LAYER_1) LoadMapFromImageFile("map1.png");
	else if (layer == DungeonLayer::LAYER_2) LoadMapFromImageFile("map2.png");
	else if (layer == DungeonLayer::LAYER_3) LoadMapFromImageFile("map3.png");
	else if (layer == DungeonLayer::BOSS_ROOM) LoadMapFromImageFile("boss.png");
}

void Map::Update(Vector2 playerMapPos) {
	// Check key collection
	for (size_t i = 0; i < keyPositions.size(); i++) {
		if (!keyCollected[i]) {
			float distToKey = sqrtf(powf(playerMapPos.x - keyPositions[i].x, 2) + powf(playerMapPos.y - keyPositions[i].y, 2));
			if (distToKey < 20.0f) {
				keyCollected[i] = true;
				keysCollected++;

				if (keysCollected == (int)keyPositions.size()) {
					doorUnlocked = true;
				}
			}
		}
	}

	// Check door collision
	if (doorUnlocked) {
		float distToDoor = sqrtf(powf(playerMapPos.x - doorPos.x, 2) + powf(playerMapPos.y - doorPos.y, 2));
		if (distToDoor < 20.0f) {
			AdvanceLayer();
		}
	}
}

void Map::DrawBaseMap() {
	// 先畫所有路面為灰色
	for (int y = 0; y < mapRows; y++) {
		for (int x = 0; x < mapCols; x++) {
			if (grid[y][x] == 0) {
				DrawRectangle(x * tileWidth, y * tileHeight, tileWidth, tileHeight, LIGHTGRAY);
			} else {
				DrawRectangle(x * tileWidth, y * tileHeight, tileWidth, tileHeight, BLACK);
			}
		}
	}
}

void Map::DrawObjects() {
	// Draw keys
	for (size_t i = 0; i < keyPositions.size(); i++) {
		if (!keyCollected[i]) {
			DrawCircleV(keyPositions[i], 8.0f, YELLOW);
		}
	}

	// Draw chest
	for (size_t i = 0; i < chestPositions.size(); i++) {
		if (!chestOpened[i]) {
			Rectangle chestRect = { chestPositions[i].x - 14.0f, chestPositions[i].y - 10.0f, 28.0f, 20.0f };
			DrawRectangleRec(chestRect, GOLD);
			DrawRectangleLines((int)chestRect.x, (int)chestRect.y, (int)chestRect.width, (int)chestRect.height, BROWN);
		}
	}

	// Draw door
	Color doorColor = doorUnlocked ? Color{ 0, 200, 120, 255 } : MAROON;
	DrawCircleV(doorPos, 14.0f, doorColor);
}

bool Map::IsWall(float worldX, float worldY) const {
	int tileX = (int)(worldX / tileWidth);
	int tileY = (int)(worldY / tileHeight);

	if (tileX < 0 || tileX >= mapCols || tileY < 0 || tileY >= mapRows) {
		return true;
	}

	return grid[tileY][tileX] == 1;
}

Vector2 Map::GetRandomFreePosition() {
	Vector2 pos = { 100, 100 };
	bool found = false;

	for (int attempts = 0; attempts < 100 && !found; attempts++) {
		int randomX = rand() % mapCols;
		int randomY = rand() % mapRows;

		if (grid[randomY][randomX] == 0) {
			pos = { (float)(randomX * tileWidth + tileWidth / 2), (float)(randomY * tileHeight + tileHeight / 2) };
			found = true;
		}
	}

	return pos;
}

bool Map::CheckWallCollision(Vector2 pos, Vector2 size) {
	float left = pos.x - size.x / 2;
	float right = pos.x + size.x / 2;
	float top = pos.y - size.y / 2;
	float bottom = pos.y + size.y / 2;

	return IsWall(left, top) || IsWall(right, top) || IsWall(left, bottom) || IsWall(right, bottom);
}

bool Map::IsMapConnected() {
	if (mapRows <= 0 || mapCols <= 0) return false;

	int startX = -1, startY = -1;
	for (int y = 0; y < mapRows && startX == -1; y++) {
		for (int x = 0; x < mapCols; x++) {
			if (grid[y][x] == 0) {
				startX = x;
				startY = y;
				break;
			}
		}
	}

	if (startX == -1) return false;

	std::vector<std::vector<bool>> visited(mapRows, std::vector<bool>(mapCols, false));
	std::vector<std::pair<int, int>> queue;
	queue.push_back({ startX, startY });
	visited[startY][startX] = true;

	int dx[] = { 0, 0, 1, -1 };
	int dy[] = { 1, -1, 0, 0 };

	while (!queue.empty()) {
		auto [x, y] = queue.front();
		queue.erase(queue.begin());

		for (int i = 0; i < 4; i++) {
			int nx = x + dx[i];
			int ny = y + dy[i];

			if (nx >= 0 && nx < mapCols && ny >= 0 && ny < mapRows &&
				!visited[ny][nx] && grid[ny][nx] == 0) {
				visited[ny][nx] = true;
				queue.push_back({ nx, ny });
			}
		}
	}

	int totalFloor = 0;
	for (int y = 0; y < mapRows; y++) {
		for (int x = 0; x < mapCols; x++) {
			if (grid[y][x] == 0) totalFloor++;
		}
	}

	int visitedFloor = 0;
	for (int y = 0; y < mapRows; y++) {
		for (int x = 0; x < mapCols; x++) {
			if (visited[y][x]) visitedFloor++;
		}
	}

	return visitedFloor == totalFloor;
}

void Map::AdvanceLayer() {
	if (currentLayer == DungeonLayer::LAYER_1) {
		InitLayer(DungeonLayer::LAYER_2);
	} else if (currentLayer == DungeonLayer::LAYER_2) {
		InitLayer(DungeonLayer::LAYER_3);
	} else if (currentLayer == DungeonLayer::LAYER_3) {
		InitLayer(DungeonLayer::BOSS_ROOM);
	} else if (currentLayer == DungeonLayer::BOSS_ROOM) {
		InitLayer(DungeonLayer::VICTORY);
	}
}
