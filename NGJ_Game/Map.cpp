#include "Map.h"
#include <math.h>
#include <cstdlib>
#include <ctime>

Map::Map(int screenWidth, int screenHeight) {
    tileWidth = 80;
    tileHeight = 80;
    GenerateDefaultMapFiles();
    InitLayer(DungeonLayer::LAYER_1);
}

Map::~Map() {}

void Map::GenerateDefaultMapFiles() {
    const int W = 24;
    const int H = 14;

    Color WHT = WHITE;
    Color BLK = BLACK;
    Color GRN = GREEN;
    Color BLU = BLUE;
    Color RED_ = RED;

    if (FileExists("map1.png") && FileExists("map2.png") && FileExists("map3.png") && FileExists("boss.png")) {
        return;
    }

    // --- 1. map1.png (Classic Maze) ---
    std::vector<Color> m1(W * H, BLK);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (y > 0 && y < H - 1 && x > 0 && x < W - 1) m1[y * W + x] = WHT;
            if (y == 4 && x > 3 && x < 20) m1[y * W + x] = BLK;
            if (y == 9 && x > 1 && x < 18) m1[y * W + x] = BLK;
            if (x == 8 && y > 2 && y < 8)  m1[y * W + x] = BLK;
            if (x == 16 && y > 5 && y < 12) m1[y * W + x] = BLK;
        }
    }
    m1[2 * W + 2] = GRN;
    m1[6 * W + 12] = BLU;
    m1[12 * W + 22] = RED_;

    // --- 2. map2.png (Spiral Maze) ---
    std::vector<Color> m2(W * H, BLK);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (!(y == 0 || y == H - 1 || x == 0 || x == W - 1)) m2[y * W + x] = WHT;
            if (y == 2 && x >= 2 && x <= W - 3) m2[y * W + x] = BLK;
            if (x == W - 3 && y >= 2 && y <= H - 3) m2[y * W + x] = BLK;
            if (y == H - 3 && x >= 2 && x <= W - 3) m2[y * W + x] = BLK;
            if (x == 2 && y >= 4 && y <= H - 3) m2[y * W + x] = BLK;
            if (y == 4 && x >= 2 && x <= W - 5) m2[y * W + x] = BLK;
            if (x == W - 5 && y >= 4 && y <= H - 5) m2[y * W + x] = BLK;
            if (y == H - 5 && x >= 4 && x <= W - 5) m2[y * W + x] = BLK;
        }
    }
    m2[1 * W + 1] = GRN;
    m2[6 * W + 11] = BLU;
    m2[12 * W + 22] = RED_;

    // --- 3. map3.png (Rooms Maze) ---
    std::vector<Color> m3(W * H, BLK);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (!(y == 0 || y == H - 1 || x == 0 || x == W - 1)) m3[y * W + x] = WHT;
            if (x == 8 || x == 16) m3[y * W + x] = BLK;
            if (y == 7) m3[y * W + x] = BLK;
        }
    }
    m3[3 * W + 8] = WHT; m3[10 * W + 8] = WHT; m3[4 * W + 16] = WHT; m3[7 * W + 4] = WHT; m3[7 * W + 12] = WHT; m3[7 * W + 20] = WHT;
    m3[2 * W + 2] = GRN;
    m3[11 * W + 3] = BLU;
    m3[3 * W + 21] = RED_;

    // --- 4. boss.png (Boss Arena) ---
    std::vector<Color> mb(W * H, WHT);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (x < 5 && y < 4) mb[y * W + x] = BLK;
            if (x > 18 && y < 4) mb[y * W + x] = BLK;
            if (x < 5 && y > 9) mb[y * W + x] = BLK;
            if (x > 18 && y > 9) mb[y * W + x] = BLK;
            if (y == 0 || y == H - 1 || x == 0 || x == W - 1) mb[y * W + x] = BLK;
        }
    }
    mb[1 * W + 12] = RED_;
    mb[12 * W + 12] = GRN;
    mb[6 * W + 12] = BLU;

    const char* names[4] = { "map1.png", "map2.png", "map3.png", "boss.png" };
    std::vector<Color>* dataPtrs[4] = { &m1, &m2, &m3, &mb };

    for (int i = 0; i < 4; i++) {
        Image img = { 0 };
        img.data = dataPtrs[i]->data();
        img.width = W;
        img.height = H;
        img.mipmaps = 1;
        img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        ExportImage(img, names[i]);
    }
}

void Map::LoadMapFromImageFile(const char* fileName) {
    Image mapImg = LoadImage(fileName);
    mapRows = mapImg.height;
    mapCols = mapImg.width;

    grid.assign(mapRows, std::vector<int>(mapCols, 0));

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
                keyPos = { (float)x * tileWidth + tileWidth / 2.0f, (float)y * tileHeight + tileHeight / 2.0f };
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
    hasKey = false;
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

    if (!hasKey) {
        float distToKey = sqrtf(powf(playerMapPos.x - keyPos.x, 2) + powf(playerMapPos.y - keyPos.y, 2));
        if (distToKey < 35.0f) hasKey = true;
    }

    if (hasKey) {
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

    Color doorColor = hasKey ? Color{ 0, 200, 120, 255 } : MAROON;
    DrawRectangleV({ doorPos.x - 25, doorPos.y - 35 }, { 50, 70 }, doorColor);
    DrawRectangleLinesEx({ doorPos.x - 25, doorPos.y - 35, 50, 70 }, 3.0f, GOLD);

    if (!hasKey) {
        DrawCircleV(keyPos, 14.0f, GOLD);
        DrawCircleV(keyPos, 8.0f, YELLOW);
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
