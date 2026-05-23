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
    std::vector<Vector2> keyPositions;      // 儲存3把鑰匙的位置
    std::vector<bool> keyCollected;         // 追蹤3把鑰匙的收集狀態
    int keysCollected;                      // 已收集的鑰匙總數
    Vector2 doorPos;
    bool doorUnlocked;                      // 當所有3把鑰匙都收集時為true

    std::vector<Vector2> chestPositions;    // 儲存寶箱位置
    std::vector<bool> chestOpened;          // 追蹤寶箱是否已打開

    void GenerateDefaultMapFiles();
    void LoadMapFromImageFile(const char* fileName);
    void InitLayer(DungeonLayer layer);
    bool IsMapConnected();                  // 驗證地圖連通性

public:
    Map(int screenWidth, int screenHeight);
    ~Map();

    void Update(Vector2 playerMapPos);
    void DrawBaseMap();
    void DrawObjects();

    DungeonLayer GetCurrentLayer() const { return currentLayer; }
    int GetKeysCollected() const { return keysCollected; }
    int GetTotalKeys() const { return 3; }
    bool IsDoorUnlocked() const { return doorUnlocked; }

    // 寶箱相關 getter
    const std::vector<Vector2>& GetChestPositions() const { return chestPositions; }
    const std::vector<bool>& GetChestOpened() const { return chestOpened; }
    int GetChestCount() const { return chestPositions.size(); }
    void OpenChest(int chestIndex) { if (chestIndex >= 0 && chestIndex < (int)chestOpened.size()) chestOpened[chestIndex] = true; }
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
