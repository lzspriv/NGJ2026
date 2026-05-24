#ifndef MAP_H
#define MAP_H

#include "raylib.h"
#include <vector>

class Map {
private:
    // ---- 關卡核心數值 ----
    int currentLevel;                       // 徹底取代舊的 DungeonLayer enum
    int tileWidth;
    int tileHeight;

    // ---- 地圖陣列與尺寸 ----
    std::vector<std::vector<int>> grid;
    int mapRows;
    int mapCols;

    // ---- 遊戲物件狀態 ----
    Vector2 playerStartPos;
    std::vector<Vector2> keyPositions;      // 儲存 3 把鑰匙的位置
    std::vector<bool> keyCollected;         // 追蹤 3 把鑰匙的收集狀態
    int keysCollected;                      // 已收集的鑰匙總數
    Vector2 doorPos;
    bool doorUnlocked;                      // 當所有 3 把鑰匙都收集時為 true
    bool bossExitDoorActive;                // Boss 擊敗後出現的出口門

    std::vector<Vector2> chestPositions;    // 儲存寶箱位置
    std::vector<bool> chestOpened;          // 追蹤寶箱是否已打開

    // ---- Boss 階段臨時障礙物 ----
    struct BossObstacle {
        Rectangle rect;
        bool active;
    };
    std::vector<BossObstacle> bossObstacles;

    // ---- 內部演算法 ----
    bool IsMapConnected();                  // 驗證隨機迷宮連通性

    int monitorW;
    int monitorH;

public:
    // 【修改】建構子現在需要接收螢幕的真實寬高
    Map(int screenWidth, int screenHeight, int monW, int monH);
    ~Map();

    // 【新增】當玩家把視窗拖到另一個不同大小的螢幕時，更新地圖認知
    void SetMonitorSize(int w, int h) { monitorW = w; monitorH = h; }

    // ---- 每幀更新與繪製 ----
    void Update(Vector2 playerMapPos);
    void DrawBaseMap();
    void DrawObjects();

    // ---- 關鍵外部介面 (Getters) ----
    int GetKeysCollected() const { return keysCollected; }
    int GetTotalKeys() const { return 3; }
    bool IsDoorUnlocked() const { return doorUnlocked; }

    // ---- 升級版無限關卡系統介面 ----
    int GetCurrentLevel() const { return currentLevel; }
    bool IsBossLevel() const { return currentLevel == 5; } // 只在第五層出現 Boss

    // ---- 動態地圖生成核心 ----
    void InitLevel(int level);
    void AdvanceLevel();

    // ---- 寶箱與地圖邊界 Helpers ----
    const std::vector<Vector2>& GetChestPositions() const { return chestPositions; }
    const std::vector<bool>& GetChestOpened() const { return chestOpened; }
    int GetChestCount() const { return chestPositions.size(); }
    void OpenChest(int chestIndex) { if (chestIndex >= 0 && chestIndex < (int)chestOpened.size()) chestOpened[chestIndex] = true; }

    // ---- Boss 階段障礙物控制 ----
    void ClearBossObstacles();
    void SpawnBossObstacles(int count);
    bool IsBossObstacleAt(float worldX, float worldY) const;
    const std::vector<BossObstacle>& GetBossObstacles() const { return bossObstacles; }

    void ActivateBossExitDoor();
    bool IsBossExitDoorActive() const { return bossExitDoorActive; }

    Vector2 GetPlayerStartPos() const { return playerStartPos; }
    bool IsWall(float worldX, float worldY) const;
    int GetTotalWidth() const { return mapCols * tileWidth; }
    int GetTotalHeight() const { return mapRows * tileHeight; }

    // ---- 怪物 AI 分支需要的 Helper 函式 ----
    Vector2 GetRandomFreePosition(int marginTiles = 1);
    bool CheckWallCollision(Vector2 pos, Vector2 size);
};

#endif // MAP_H
