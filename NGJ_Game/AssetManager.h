#pragma once
#include "raylib.h"

struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float alpha;
    float lifeTime; // 粒子剩餘壽命
    bool active;    // 粒子是否在使用中
    float size;
};

class AssetManager {
public:
    // 1. 生命週期管理 (只在 main.cpp 呼叫)
    static void LoadAllAssets();
    static void UnloadAllAssets();

    // 2. 美術貼圖獲取函式 (Getters)
    static Texture2D GetMenuBackground();
    static Texture2D GetPlayerTexture();
    static Texture2D GetBossTexture();
    static Texture2D GetItemKeyTexture();
    static Texture2D GetBuffItemTexture();

    // 3. 音樂與音效獲取函式 (Getters)
    static Music GetBgmMenu();
    static Music GetBgmGameplay();
    static Music GetBgmBoss();

    static Sound GetSoundShoot();
    static Sound GetSoundSlash();

    static Sound GetSoundClick();
    static Sound GetSoundExpand();
    static Sound GetSoundHit();
    static Sound GetSoundPickup();

    // 4. 字體獲取函式 (Getter)
    static Font GetGameFont();

    static void EmitParticle(Vector2 position, Color color, float size); // 觸發粒子
    static void UpdateParticles();                           // 更新所有粒子位置
    static void DrawParticles();                             // 繪製所有粒子

    static void DrawPlayerAnimated(Vector2 position, Color tint);

    static void DrawEnemyAnimated(Vector2 position, float* animTimer, int* currentFrame, Color tint);
};