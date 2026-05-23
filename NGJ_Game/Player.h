#pragma once
#include "raylib.h"
#include <vector>

struct Bullet {
    Vector2 pos;
    Vector2 speed;
    bool active;
};

enum AttackMode {
    MODE_SHOOT,
    MODE_MELEE
};

// 近戰（動態揮砍劍）結構體
struct SwordAttack {
    Vector2 center;       // 揮劍的圓心 (玩家中心)
    float startAngle;     // 揮砍起點角度 (度)
    float endAngle;       // 揮砍終點角度 (度)
    float currentAngle;   // 目前長方形劍身所在的實時角度 (度)
    float duration;       // 整個揮砍動作總共要花多少秒
    float activeTimer;    // 揮劍計時器 (秒)
    float cooldownTimer;  // 揮劍冷卻計時器 (秒)
    bool active;          // 目前是否正在揮劍中
};

class PlayerManager {
public:
    Vector2 playerPos;
    float playerSpeed;
    int maxHp;
    int currentHp;
    int score;
    std::vector<Bullet> bullets;

    AttackMode currentMode;
    SwordAttack sword;

    int currentWinWidth;
    int currentWinHeight;
    Vector2 currentWinPos;

    PlayerManager();
    ~PlayerManager();

    void InitPlayer();
    void HandleInput();
    // 僅處理攻擊輸入（不改變視窗或玩家位置）
    void ProcessCombatInput();
    // 更新攻擊相關（劍、子彈）計時與位置
    void UpdateCombat(float dt);
    void UpdatePlayerAndWindow(float dt);
    void DrawPlayer();
};
