#include "Map.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm> // 給 std::min 使用

// 【修改】接收螢幕尺寸
Map::Map(int screenWidth, int screenHeight, int monW, int monH) {
	tileWidth = 50;
	tileHeight = 50;
	monitorW = monW;
	monitorH = monH;
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
	bossExitDoorActive = false;

	// 1. 地圖大小嚴格綁定為螢幕大小，不再無限擴張！
	mapCols = monitorW / tileWidth;
	mapRows = monitorH / tileHeight;
	if (mapCols < 10) mapCols = 10; // 防呆
	if (mapRows < 10) mapRows = 10;

	grid.assign(mapRows, std::vector<int>(mapCols, 0));
	keyPositions.clear();
	keyCollected.clear();
	chestPositions.clear();
	chestOpened.clear();

	// 2. 玩家出生點嚴格鎖定在「螢幕實體正中央」
	playerStartPos = { monitorW / 2.0f, monitorH / 2.0f };
	int startTileX = (int)(playerStartPos.x / tileWidth);
	int startTileY = (int)(playerStartPos.y / tileHeight);

	// 3. 生成地形
	if (IsBossLevel()) {
		for (int y = 0; y < mapRows; y++) {
			for (int x = 0; x < mapCols; x++) {
				if (x == 0 || y == 0 || x == mapCols - 1 || y == mapRows - 1) {
					grid[y][x] = 1;
				}
				else if (x % 15 == 0 && y % 15 == 0 && x > 5 && y > 5 && x < mapCols - 5 && y < mapRows - 5) {
					grid[y][x] = 1;
				}
			}
		}
	}
	else {
		bool connected = false;
		while (!connected) {
			for (int y = 0; y < mapRows; y++) {
				for (int x = 0; x < mapCols; x++) {
					if (x == 0 || y == 0 || x == mapCols - 1 || y == mapRows - 1) grid[y][x] = 1;
					else grid[y][x] = 0;
				}
			}

			// 牆壁密度稍微調降，避免螢幕內太擠 (15% ~ 25%)
			float density = 0.15f + std::min(level * 0.01f, 0.10f);
			int numWalls = (int)(mapCols * mapRows * density);

			for (int i = 0; i < numWalls; i++) {
				int rx = 1 + rand() % (mapCols - 2);
				int ry = 1 + rand() % (mapRows - 2);

				// 【關鍵修復】：挖出一個 5x5 的絕對安全區，保證出生點(螢幕正中央)絕對沒有牆壁！
				if (abs(rx - startTileX) <= 2 && abs(ry - startTileY) <= 2) continue;

				grid[ry][rx] = 1;
			}
			connected = IsMapConnected();
		}
	}

	// 4. 放置物件：Boss 第五層不生成鑰匙、門與寶箱
	if (!IsBossLevel()) {
		for (int i = 0; i < 3; i++) {
			keyPositions.push_back(GetRandomFreePosition(2));
			keyCollected.push_back(false);
		}

		chestPositions.push_back(GetRandomFreePosition(2));
		chestOpened.push_back(false);

		doorPos = GetRandomFreePosition(2);
		while (sqrtf(powf(doorPos.x - playerStartPos.x, 2) + powf(doorPos.y - playerStartPos.y, 2)) < (mapCols * tileWidth * 0.3f)) {
			doorPos = GetRandomFreePosition(2);
		}
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
				// 【空地/地板】：極深的灰藍色
				DrawRectangle(x * tileWidth, y * tileHeight, tileWidth, tileHeight, Color{ 20, 24, 30, 255 });
				// 加上極暗的網格線，視覺上會有一種科技地磚的感覺
				DrawRectangleLines(x * tileWidth, y * tileHeight, tileWidth, tileHeight, Color{ 30, 35, 45, 255 });
			}
			else {
				// 【牆壁】：幾乎全黑的底色，配上稍微亮一點的邊框凸顯立體感
				DrawRectangle(x * tileWidth, y * tileHeight, tileWidth, tileHeight, Color{ 5, 8, 12, 255 });
				DrawRectangleLines(x * tileWidth, y * tileHeight, tileWidth, tileHeight, Color{ 40, 50, 70, 255 });
			}
		}
	}
}

void Map::DrawObjects() {
	if (!IsBossLevel()) {
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

	if (bossExitDoorActive) {
		Rectangle exitRect = { playerStartPos.x - 18.0f, playerStartPos.y - 26.0f, 36.0f, 52.0f };
		DrawRectangleRec(exitRect, GREEN);
		DrawRectangleLines((int)exitRect.x, (int)exitRect.y, (int)exitRect.width, (int)exitRect.height, WHITE);
	}

	// Draw boss obstacles
	for (const auto& obstacle : bossObstacles) {
		if (!obstacle.active) continue;
		DrawRectangleRec(obstacle.rect, DARKGRAY);
		DrawRectangleLines((int)obstacle.rect.x, (int)obstacle.rect.y, (int)obstacle.rect.width, (int)obstacle.rect.height, BLACK);
	}
}

bool Map::IsWall(float worldX, float worldY) const {
	int tileX = (int)(worldX / tileWidth);
	int tileY = (int)(worldY / tileHeight);

	if (tileX < 0 || tileX >= mapCols || tileY < 0 || tileY >= mapRows) {
		return true;
	}

	return grid[tileY][tileX] == 1;
}

void Map::ClearBossObstacles() {
	bossObstacles.clear();
}

void Map::SpawnBossObstacles(int count) {
	bossObstacles.clear();
	if (count <= 0) return;

	int usableW = mapCols * tileWidth;
	int usableH = mapRows * tileHeight;
	int obstacleW = 60;
	int obstacleH = 60;
	int centerX = usableW / 2;
	int centerY = usableH / 2;
	int offsetX = 120;
	int offsetY = 90;

	for (int i = 0; i < count; ++i) {
		BossObstacle ob{};
		ob.active = true;
		switch (i % 4) {
		case 0: ob.rect = { (float)(centerX - offsetX - obstacleW / 2), (float)(centerY - obstacleH / 2), (float)obstacleW, (float)obstacleH }; break;
		case 1: ob.rect = { (float)(centerX + offsetX - obstacleW / 2), (float)(centerY - obstacleH / 2), (float)obstacleW, (float)obstacleH }; break;
		case 2: ob.rect = { (float)(centerX - obstacleW / 2), (float)(centerY - offsetY - obstacleH / 2), (float)obstacleW, (float)obstacleH }; break;
		default: ob.rect = { (float)(centerX - obstacleW / 2), (float)(centerY + offsetY - obstacleH / 2), (float)obstacleW, (float)obstacleH }; break;
		}
		bossObstacles.push_back(ob);
	}
}

bool Map::IsBossObstacleAt(float worldX, float worldY) const {
	for (const auto& obstacle : bossObstacles) {
		if (!obstacle.active) continue;
		if (worldX >= obstacle.rect.x && worldX <= obstacle.rect.x + obstacle.rect.width &&
			worldY >= obstacle.rect.y && worldY <= obstacle.rect.y + obstacle.rect.height) {
			return true;
		}
	}
	return false;
}

void Map::ActivateBossExitDoor() {
	bossExitDoorActive = true;
}

// 【修改】加入 marginTiles 參數，限制隨機座標不要貼在極限邊緣
Vector2 Map::GetRandomFreePosition(int marginTiles) {
	Vector2 pos = { playerStartPos.x, playerStartPos.y }; // 預設防呆回傳中心點
	bool found = false;

	for (int attempts = 0; attempts < 200 && !found; attempts++) {
		// 限制隨機範圍在 marginTiles 之外
		int randomX = marginTiles + rand() % (mapCols - 2 * marginTiles);
		int randomY = marginTiles + rand() % (mapRows - 2 * marginTiles);

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