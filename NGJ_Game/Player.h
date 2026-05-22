#pragma once
#include "raylib.h"
#include <vector>

// 玩家子彈結構體
struct Bullet {
    Vector2 pos;
    Vector2 speed;
    bool active;
};

class PlayerManager {
public:
    Vector2 playerPos;
    float playerSpeed;
    int maxHp;
    int currentHp;          // 改成血量制增加容錯，而非一擊必殺
    int score;
    std::vector<Bullet> bullets;

    // 視窗目前的寬高與在螢幕上的絕對座標（用於控制視窗變化）
    int currentWinWidth;
    int currentWinHeight;
    Vector2 currentWinPos;

    PlayerManager();
    ~PlayerManager();

    // 初始化玩家屬性與初始 400x400 視窗設定
    void InitPlayer();

    // 處理 WASD 移動、滑鼠點擊發射子彈
    void HandleInput();

    // 更新子彈位置與核心的「四向視窗推動/擴張」邏輯
    void UpdatePlayerAndWindow(float dt);

    // 繪製玩家小藍方塊與所有發射出去的子彈
    void DrawPlayer();
};