#pragma once
#include "raylib.h"
#include <vector>

// 道具/鑰匙結構體
struct Item {
    Vector2 pos;
    int type;       // 1: 下一關鑰匙, 2: 補血道具, 3: 速度Buff增益
    bool active;
};

class MapManager {
public:
    // 用二維陣列定義 50x50 大地圖（1 代表牆壁，0 代表空地）
    int worldMap[50][50];
    int tileSize;       // 每個格子的寬高（例如 40 像素）
    Item levelKey;      // 每一關通往下一關的鑰匙
    std::vector<Item> buffItems; // 散落在地圖各處的增益道具

    MapManager();
    ~MapManager();

    // 根據不同關卡（LV1 迷宮、BOSS戰核心位置）初始化地圖數據
    void LoadMapData(int level);

    // 隨機在地圖空地上生成增益道具與鑰匙
    void SpawnItems();

    // 從地圖上的空地隨機取得一個世界座標位置（回傳 Raylib 的 Vector2）
    Vector2 GetRandomFreePosition();

    // 檢查實體（玩家或怪物）是否與牆壁發生碰撞（防止穿牆）
    bool CheckWallCollision(Vector2 pos, Vector2 size);

    // 視野裁剪繪製：關鍵！只有當地圖牆壁/道具的座標在「目前視窗寬高內」才呼叫 Raylib 畫出來
    void DrawMap(int winWidth, int winHeight);
};