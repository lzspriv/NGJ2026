#pragma once
#include "raylib.h"
#include <vector>

// 怪物結構體
struct Enemy {
    Vector2 pos;
    float speed;
    int hp;
    int type;       // 1: 直線追蹤玩家, 2: 吃豆人式包抄AI, 3: 遠程子彈怪
    bool active;
};

// Boss 戰核心節點結構體
struct BossCore {
    Vector2 pos;
    int hp;
    bool active;    // 玩家必須尋找並擊破這些散落的核心，才能對 Boss 造成傷害
};

class EnemyManager {
public:
    std::vector<Enemy> activeEnemies;
    std::vector<BossCore> bossCores;

    // Boss 屬性
    Vector2 bossPos;
    float bossSpeed;
    int bossHp;
    int bossMaxHp;
    bool isBossSpawned;

    EnemyManager();
    ~EnemyManager();

    // 根據目前關卡時間或波次，在視窗邊界外生成小怪
    void SpawnEnemy(int level, Vector2 playerPos);

    // 更新怪物的追蹤 AI 邏輯、敵方彈幕、以及 Boss 戰時「視窗每秒縮小」的邏輯
    void UpdateEnemiesAndBoss(Vector2 playerPos, int& winWidth, int& winHeight, float dt);

    // 繪製所有小怪、Boss、敵方彈幕、雷射預警線以及 Boss 的保護核心
    void DrawEnemies();
};