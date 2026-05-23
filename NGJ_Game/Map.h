#ifndef MAP_H
#define MAP_H

#include "raylib.h"
#include <vector>

enum class DungeonLayer { LAYER_1, LAYER_2, LAYER_3, BOSS_ROOM, VICTORY };

class Map {
private:
    DungeonLayer currentLayer;
    int tileWidth, tileHeight;
    std::vector<std::vector<int>> grid;
    int mapRows, mapCols;

    // debug: store sampled pixel colors from the map image
    std::vector<std::vector<Color>> sampledColors;
    bool debugMode = false;
    double ignoreInputUntil = 0.0; // when < GetTime(), input allowed

    Vector2 playerStartPos; // 綠色：上一層入口
    Vector2 keyPos;         // 藍色：鑰匙
    Vector2 doorPos;        // 紅色：下一層入口
    Vector2 lastPlayerMapPos; // 用於在 DrawObjects 判斷與玩家的距離（顯示提示）
    // debug / input tracking
    float lastDistToKey = 0.0f;
    float lastDistToDoor = 0.0f;
    float lastDistToStart = 0.0f;
    bool lastSpacePressed = false;

    bool layerKeys[4];      // 記錄每一層是否拿過鑰匙
    bool spawnAtDoor;       // 用於判斷是從哪裡生成

    void GenerateDefaultMapFiles();
    void LoadMapFromImageFile(const char* fileName);
    void InitLayer(DungeonLayer layer);


public:
    Map(int screenWidth, int screenHeight);
    void Update(Vector2 playerMapPos);
    void DrawBaseMap();
    void DrawObjects();
    void SetDebugMode(bool enabled);

    DungeonLayer GetCurrentLayer() const { return currentLayer; }
    bool PlayerHasKey() const { return layerKeys[(int)currentLayer]; }
    Vector2 GetCurrentSpawnPos() const { return spawnAtDoor ? doorPos : playerStartPos; }
    Vector2 GetPlayerStartPos() const { return spawnAtDoor ? doorPos : playerStartPos; }
    bool IsWall(float worldX, float worldY) const;
    void AdvanceLayer();
    void ReturnLayer();
};

#endif