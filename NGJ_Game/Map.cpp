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

    // 明確定義顏色，包括 Alpha 通道
    Color WHT = { 255, 255, 255, 255 };
    Color BLK = { 0, 0, 0, 255 };
    Color GRN = { 0, 255, 0, 255 };
    Color BLU = { 0, 0, 255, 255 };
    Color RED_ = { 255, 0, 0, 255 };

    if (FileExists("map1.png") && FileExists("map2.png") && FileExists("map3.png") && FileExists("boss.png")) {
        return;
    }

    // --- 1. map1.png (Classic Maze) ---
    std::vector<Color> m1(W * H, BLK);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (y > 0 && y < H - 1 && x > 0 && x < W - 1) m1[y * W + x] = WHT;
            if (y == 7 && x > 5 && x < 33) m1[y * W + x] = BLK;
            if (y == 15 && x > 3 && x < 30) m1[y * W + x] = BLK;
            if (x == 12 && y > 3 && y < 13)  m1[y * W + x] = BLK;
            if (x == 25 && y > 8 && y < 18) m1[y * W + x] = BLK;
        }
    }
    // 確保中心位置(2-3行, 2-3列)是路
    m1[2 * W + 2] = WHT;
    m1[2 * W + 3] = WHT;
    m1[3 * W + 2] = WHT;
    m1[3 * W + 3] = WHT;

    // 設置起點(綠色)在中心附近
    m1[2 * W + 2] = GRN;

    // 設置3把鑰匙(藍色)
    m1[9 * W + 10] = BLU;
    m1[9 * W + 28] = BLU;
    m1[17 * W + 19] = BLU;

    // --- 2. map2.png (Spiral Maze) ---
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
    // 確保中心位置是路
    m2[2 * W + 2] = WHT;
    m2[2 * W + 3] = WHT;
    m2[3 * W + 2] = WHT;
    m2[3 * W + 3] = WHT;

    // 設置起點(綠色)在中心
    m2[2 * W + 2] = GRN;

    // 設置3把鑰匙(藍色)
    m2[7 * W + 12] = BLU;
    m2[12 * W + 28] = BLU;
    m2[18 * W + 10] = BLU;

    // --- 3. map3.png (Rooms Maze) ---
    std::vector<Color> m3(W * H, BLK);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (!(y == 0 || y == H - 1 || x == 0 || x == W - 1)) m3[y * W + x] = WHT;
            if (x == 12 || x == 25) m3[y * W + x] = BLK;
            if (y == 11) m3[y * W + x] = BLK;
        }
    }
    m3[5 * W + 12] = WHT; m3[16 * W + 12] = WHT; m3[6 * W + 25] = WHT; m3[11 * W + 6] = WHT; m3[11 * W + 18] = WHT; m3[11 * W + 32] = WHT;

    // 確保中心位置是路
    m3[2 * W + 2] = WHT;
    m3[2 * W + 3] = WHT;
    m3[3 * W + 2] = WHT;
    m3[3 * W + 3] = WHT;

    // 設置起點(綠色)在中心
    m3[2 * W + 2] = GRN;

    // 設置3把鑰匙(藍色)
    m3[8 * W + 6] = BLU;
    m3[8 * W + 22] = BLU;
    m3[16 * W + 32] = BLU;

    // --- 4. boss.png (Boss Arena) ---
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
    // 確保中心位置是路
    mb[2 * W + 2] = WHT;
    mb[2 * W + 3] = WHT;
    mb[3 * W + 2] = WHT;
    mb[3 * W + 3] = WHT;

    // Boss房間起點在中心
    mb[2 * W + 2] = GRN;

    // Boss房間的3把鑰匙(藍色)
    mb[7 * W + 10] = BLU;
    mb[7 * W + 28] = BLU;
    mb[14 * W + 19] = BLU;

    const char* names[4] = { "map1.png", "map2.png", "map3.png", "boss.png" };
    std::vector<Color>* dataPtrs[4] = { &m1, &m2, &m3, &mb };

    for (int i = 0; i < 4; i++) {
        // 在導出前，隨機重新放置門的位置
        std::vector<Color>& mapData = *dataPtrs[i];

        // 尋找所有紅色像素（門的舊位置）並替換為白色
        for (int j = 0; j < W * H; j++) {
            if (mapData[j].r > 200 && mapData[j].g < 50 && mapData[j].b < 50) {
                mapData[j] = WHT;
            }
        }

        // 隨機選擇一個可通行的位置放置門
        bool doorPlaced = false;
        for (int attempts = 0; attempts < 500 && !doorPlaced; attempts++) {
            int randomX = 3 + (rand() % (W - 6));  // 避免邊界
            int randomY = 3 + (rand() % (H - 6));
            int idx = randomY * W + randomX;

            // 檢查該位置是否為白色（可通行）且不是綠色（起點）和藍色（鑰匙）
            if (mapData[idx].r == 255 && mapData[idx].g == 255 && mapData[idx].b == 255) {
                // 確保足夠遠離起點（至少距離為5）
                bool tooCloseToStart = false;
                for (int y = 0; y < H; y++) {
                    for (int x = 0; x < W; x++) {
                        if (mapData[y * W + x].r == 0 && mapData[y * W + x].g > 200 && mapData[y * W + x].b == 0) {
                            // 找到了起點
                            int distSq = (randomX - x) * (randomX - x) + (randomY - y) * (randomY - y);
                            if (distSq < 25) { // 距離小於5個瓦片
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

        // 如果隨機放置失敗，就用預設位置
        if (!doorPlaced) {
            mapData[11 * W + 22] = RED_;
        }

        // 創建一個簡單的圖像，每個像素對應一個瓦片
        Image img = { 0 };
        img.width = W;
        img.height = H;
        img.mipmaps = 1;
        img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

        // 分配內存
        img.data = malloc(W * H * sizeof(Color));

        // 複製像素數據
        Color* imgData = (Color*)img.data;
        for (int j = 0; j < W * H; j++) {
            imgData[j] = (*dataPtrs[i])[j];
        }

        ExportImage(img, names[i]);
        free(img.data);
    }
}

void Map::LoadMapFromImageFile(const char* fileName) {
    Image mapImg = LoadImage(fileName);
    mapRows = mapImg.height;
    mapCols = mapImg.width;

    grid.assign(mapRows, std::vector<int>(mapCols, 0));
    keyPositions.clear();
    keyCollected.clear();

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
                // 藍色表示鑰匙 - 支援多個鑰匙
                keyPositions.push_back({ (float)x * tileWidth + tileWidth / 2.0f, (float)y * tileHeight + tileHeight / 2.0f });
                keyCollected.push_back(false);
                grid[y][x] = 0;
            }
            else if (pixel.r > 200 && pixel.g < 50 && pixel.b < 50) {
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

bool Map::IsWall(float worldX, float worldY) const {
    int cellX = (int)(worldX / tileWidth);
    int cellY = (int)(worldY / tileHeight);
    if (cellX < 0 || cellX >= mapCols || cellY < 0 || cellY >= mapRows) return true;
    return grid[cellY][cellX] == 1;
}

void Map::Update(Vector2 playerMapPos) {
    if (currentLayer == DungeonLayer::VICTORY) return;

    // 檢查是否收集到未收集的鑰匙
    for (size_t i = 0; i < keyPositions.size(); i++) {
        if (!keyCollected[i]) {
            float distToKey = sqrtf(powf(playerMapPos.x - keyPositions[i].x, 2) + powf(playerMapPos.y - keyPositions[i].y, 2));
            if (distToKey < 35.0f) {
                keyCollected[i] = true;
                keysCollected++;

                // 當所有鑰匙都收集時，解鎖門
                if (keysCollected == (int)keyPositions.size()) {
                    doorUnlocked = true;
                }
            }
        }
    }

    // 檢查是否可以通過門進入下一關
    if (doorUnlocked) {
        float distToDoor = sqrtf(powf(playerMapPos.x - doorPos.x, 2) + powf(playerMapPos.y - doorPos.y, 2));
        if (distToDoor < 45.0f) AdvanceLayer();
    }
}

void Map::AdvanceLayer() {
    if (currentLayer == DungeonLayer::LAYER_1) InitLayer(DungeonLayer::LAYER_2);
    else if (currentLayer == DungeonLayer::LAYER_2) InitLayer(DungeonLayer::LAYER_3);
    else if (currentLayer == DungeonLayer::LAYER_3) InitLayer(DungeonLayer::BOSS_ROOM);
    else if (currentLayer == DungeonLayer::BOSS_ROOM) InitLayer(DungeonLayer::VICTORY);
}

bool Map::IsMapConnected() {
    // 使用泛洪填充(BFS)來驗證所有可通行格子是否連通
    if (mapRows <= 0 || mapCols <= 0) return false;

    // 找到起始可通行格子
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

    if (startX == -1) return false; // 沒有可通行格子

    // BFS 訪問所有連通的格子
    std::vector<std::vector<bool>> visited(mapRows, std::vector<bool>(mapCols, false));
    std::vector<std::pair<int, int>> queue;
    queue.push_back({ startX, startY });
    visited[startY][startX] = true;
    int connectedCount = 1;

    int dx[] = { -1, 1, 0, 0 };
    int dy[] = { 0, 0, -1, 1 };

    size_t queueIdx = 0;
    while (queueIdx < queue.size()) {
        auto [x, y] = queue[queueIdx++];

        for (int dir = 0; dir < 4; dir++) {
            int nx = x + dx[dir];
            int ny = y + dy[dir];

            if (nx >= 0 && nx < mapCols && ny >= 0 && ny < mapRows &&
                !visited[ny][nx] && grid[ny][nx] == 0) {
                visited[ny][nx] = true;
                queue.push_back({ nx, ny });
                connectedCount++;
            }
        }
    }

    // 計算總共有多少可通行格子
    int totalFloor = 0;
    for (int y = 0; y < mapRows; y++) {
        for (int x = 0; x < mapCols; x++) {
            if (grid[y][x] == 0) totalFloor++;
        }
    }

    // 如果所有可通行格子都是連通的，則地圖連通
    return connectedCount == totalFloor;
}

void Map::DrawBaseMap() {
    Color wallColor = { 55, 55, 65, 255 };
    Color floorColor = { 25, 25, 30, 255 };

    if (currentLayer == DungeonLayer::BOSS_ROOM) {
        wallColor = { 90, 35, 35, 255 };
        floorColor = { 40, 20, 20, 255 };
    }

    for (int y = 0; y < mapRows; y++) {
        for (int x = 0; x < mapCols; x++) {
            Rectangle rect = { (float)x * tileWidth, (float)y * tileHeight, (float)tileWidth, (float)tileHeight };
            if (grid[y][x] == 1) {
                DrawRectangleRec(rect, wallColor);
                DrawRectangleLinesEx(rect, 1.0f, { 80, 80, 95, 255 });
            }
            else {
                DrawRectangleRec(rect, floorColor);
            }
        }
    }
}

void Map::DrawObjects() {
    if (currentLayer == DungeonLayer::VICTORY) return;

    // 門的顏色取決於是否所有鑰匙都已收集
    Color doorColor = doorUnlocked ? Color{ 0, 200, 120, 255 } : MAROON;
    DrawRectangleV({ doorPos.x - 25, doorPos.y - 35 }, { 50, 70 }, doorColor);
    DrawRectangleLinesEx({ doorPos.x - 25, doorPos.y - 35, 50, 70 }, 3.0f, GOLD);

    // 繪製所有未收集的鑰匙
    for (size_t i = 0; i < keyPositions.size(); i++) {
        if (!keyCollected[i]) {
            DrawCircleV(keyPositions[i], 14.0f, GOLD);
            DrawCircleV(keyPositions[i], 8.0f, YELLOW);
        }
    }
}

// Enemy helpers
Vector2 Map::GetRandomFreePosition() {
    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        seeded = true;
    }
    for (int attempt = 0; attempt < 500; ++attempt) {
        int ry = std::rand() % mapRows;
        int rx = std::rand() % mapCols;
        if (grid[ry][rx] == 0) {
            float x = rx * tileWidth + tileWidth * 0.5f;
            float y = ry * tileHeight + tileHeight * 0.5f;
            return Vector2{ x, y };
        }
    }
    for (int y = 0; y < mapRows; ++y) {
        for (int x = 0; x < mapCols; ++x) {
            if (grid[y][x] == 0) return Vector2{ x * tileWidth + tileWidth * 0.5f, y * tileHeight + tileHeight * 0.5f };
        }
    }
    return Vector2{ tileWidth * 0.5f, tileHeight * 0.5f };
}

bool Map::CheckWallCollision(Vector2 pos, Vector2 size) {
    float left = pos.x - size.x * 0.5f;
    float right = pos.x + size.x * 0.5f;
    float top = pos.y - size.y * 0.5f;
    float bottom = pos.y + size.y * 0.5f;

    int tx0 = (int)(left / tileWidth);
    int tx1 = (int)(right / tileWidth);
    int ty0 = (int)(top / tileHeight);
    int ty1 = (int)(bottom / tileHeight);

    if (tx0 < 0) tx0 = 0; if (ty0 < 0) ty0 = 0;
    if (tx1 >= mapCols) tx1 = mapCols - 1; if (ty1 >= mapRows) ty1 = mapRows - 1;

    for (int y = ty0; y <= ty1; ++y) {
        for (int x = tx0; x <= tx1; ++x) {
            if (grid[y][x] == 1) return true;
        }
    }
    return false;
}
