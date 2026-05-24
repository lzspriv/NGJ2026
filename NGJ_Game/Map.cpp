#include "Map.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm> // 給 std::min 使用

Map::Map(int screenWidth, int screenHeight) {
	tileWidth = 50;
	tileHeight = 50;
	InitLevel(1); // 遊戲從第 1 關開始
}

Map::~Map() {}

void Map::AdvanceLevel() {
	InitLevel(currentLevel + 1);
}

void Map::InitLevel(int level) {
	currentLevel = level;
	keysCollected = 0;
	doorUnlocked = false;

	// 1. 動態擴大地圖尺寸：關卡越高，地圖越大！(最大限制在 80x60，避免太卡)
	mapCols = 30 + std::min(level * 4, 50);
	mapRows = 20 + std::min(level * 3, 40);

	grid.assign(mapRows, std::vector<int>(mapCols, 0));
	keyPositions.clear();
	keyCollected.clear();
	chestPositions.clear();
	chestOpened.clear();

	// 固定起點在左上角安全區
	playerStartPos = { tileWidth * 2.5f, tileHeight * 2.5f };

	// 2. 判斷是否為 Boss 關卡 (每 5 關觸發)
	if (IsBossLevel()) {
		// Boss 競技場：超大空地，四周有牆，中間散落一些柱子當掩體
		for (int y = 0; y < mapRows; y++) {
			for (int x = 0; x < mapCols; x++) {
				if (x == 0 || y == 0 || x == mapCols - 1 || y == mapRows - 1) {
					grid[y][x] = 1;
				}
				else if (x % 15 == 0 && y % 15 == 0 && x > 5 && y > 5 && x < mapCols - 5 && y < mapRows - 5) {
					grid[y][x] = 1; // 掩體柱子
				}
			}
		}
	}
	else {
		// 普通關卡：動態迷宮生成
		bool connected = false;
		while (!connected) {
			// 先鋪滿空地
			for (int y = 0; y < mapRows; y++) {
				for (int x = 0; x < mapCols; x++) {
					if (x == 0 || y == 0 || x == mapCols - 1 || y == mapRows - 1) grid[y][x] = 1;
					else grid[y][x] = 0;
				}
			}

			// 依據關卡數增加牆壁障礙物的密度 (15% ~ 35%)
			float density = 0.15f + std::min(level * 0.015f, 0.20f);
			int numWalls = (int)(mapCols * mapRows * density);

			for (int i = 0; i < numWalls; i++) {
				int rx = 2 + rand() % (mapCols - 4);
				int ry = 2 + rand() % (mapRows - 4);
				// 絕對保護玩家起點不被牆塞死 (留出 6x6 空地)
				if (rx < 6 && ry < 6) continue;
				grid[ry][rx] = 1;
			}

			// 利用完美連通性檢查，不通就打掉重做！
			connected = IsMapConnected();
		}
	}

	// 3. 放置物件 (利用 GetRandomFreePosition)
	for (int i = 0; i < 3; i++) { // 固定生成 3 把鑰匙
		keyPositions.push_back(GetRandomFreePosition());
		keyCollected.push_back(false);
	}

	chestPositions.push_back(GetRandomFreePosition());
	chestOpened.push_back(false);

	// 放置下一層的門 (確保它離起點夠遠)
	doorPos = GetRandomFreePosition();
	while (sqrtf(powf(doorPos.x - playerStartPos.x, 2) + powf(doorPos.y - playerStartPos.y, 2)) < (mapCols * tileWidth * 0.4f)) {
		doorPos = GetRandomFreePosition();
	}
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
			AdvanceLevel(); // 【修正】呼叫新的 AdvanceLevel
		}
	}
}

void Map::DrawBaseMap() {
	for (int y = 0; y < mapRows; y++) {
		for (int x = 0; x < mapCols; x++) {
			if (grid[y][x] == 0) {
				DrawRectangle(x * tileWidth, y * tileHeight, tileWidth, tileHeight, LIGHTGRAY);
			}
			else {
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