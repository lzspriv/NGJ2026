#ifndef MAP_H
#define MAP_H

#include "raylib.h"
#include <vector>

enum class DungeonLayer {
    LAYER_1,
    LAYER_2,
    LAYER_3,
    BOSS_ROOM,
    VICTORY
};

class Map {
private:
    DungeonLayer currentLayer;
    int tileWidth;
    int tileHeight;

    std::vector<std::vector<int>> grid;
    int mapRows;
    int mapCols;

    Vector2 playerStartPos;
    Vector2 keyPos;
    Vector2 doorPos;
    bool hasKey;

    void GenerateDefaultMapFiles();
    void LoadMapFromImageFile(const char* fileName);
    void InitLayer(DungeonLayer layer);

public:
    Map(int screenWidth, int screenHeight);
    ~Map();

    void Update(Vector2 playerMapPos);
    void DrawBaseMap();
    void DrawObjects();

    DungeonLayer GetCurrentLayer() const { return currentLayer; }
    bool PlayerHasKey() const { return hasKey; }
    Vector2 GetPlayerStartPos() const { return playerStartPos; }
    bool IsWall(float worldX, float worldY) const;
    int GetTotalWidth() const { return mapCols * tileWidth; }
    int GetTotalHeight() const { return mapRows * tileHeight; }

    // Enemy-branch helpers: 提供隨機可刷位置及牆碰撞檢查
    Vector2 GetRandomFreePosition();
    bool CheckWallCollision(Vector2 pos, Vector2 size);

    void AdvanceLayer();
};

#endif // MAP_H
