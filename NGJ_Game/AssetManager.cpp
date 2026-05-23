#include "AssetManager.h"
#include <iostream>
#include <vector>

const int MAX_PARTICLES = 50; // 一次最多出現 50 個粒子
static Particle particles[MAX_PARTICLES];

// ============================================================================
// 隱藏的實體變數區 (鎖定在 cpp 內部，防止跨檔案衝突)
// ============================================================================
static Texture2D menuBackground;
static Texture2D playerTexture;//check
static Texture2D enemyTexture;
static Texture2D bossTexture;
static Texture2D itemKeyTexture;
static Texture2D buffItemTexture;

static Music bgmMenu;//check
static Music bgmGameplay;//check
static Music bgmBoss;//check

static Sound soundShoot;//check
static Sound soundSlash;//check
static Sound soundExpand;//check
static Sound soundHit;//check

static Sound soundClick;//check
static Sound soundPickup;//check

static Font gameFont;//check

//anim
static float playerTimer = 0.0f;
static int playerCurrentFrame = 0;
const int PLAYER_MAX_FRAMES = 4;      // 你的 player.png 橫向有 4 格
const float PLAYER_FRAME_SPEED = 0.1f; // 每 0.1 秒換一格影格

// ============================================================================
// 系統生命週期管理
// ============================================================================
void AssetManager::LoadAllAssets() {
    std::cout << "[AssetManager] Loading assets..." << std::endl;

    // 載入貼圖
    menuBackground = LoadTexture("assets/bg_menu.png");
    playerTexture = LoadTexture("assets/player.png");
    bossTexture = LoadTexture("assets/boss.png");
    itemKeyTexture = LoadTexture("assets/key.png");
    buffItemTexture = LoadTexture("assets/buff.png");

    // 載入聲音串流
    bgmMenu = LoadMusicStream("assets/bgm_menu.mp3");
    bgmGameplay = LoadMusicStream("assets/bgm_gameplay.mp3");
    bgmBoss = LoadMusicStream("assets/bgm_boss.mp3");

    soundShoot = LoadSound("assets/shoot.wav");
    soundSlash = LoadSound("assets/slash.wav");
    soundExpand = LoadSound("assets/expand.wav");
    soundClick = LoadSound("assets/click.wav");
    soundHit = LoadSound("assets/hit.wav");
    soundPickup = LoadSound("assets/pickup.wav");

    // 載入字體
    gameFont = LoadFont("assets/mainfont.otf");

    //粒子效果
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].active = false;
    }

    std::cout << "[AssetManager] Assets loaded successfully" << std::endl;
}



void AssetManager::UnloadAllAssets() {
    std::cout << "[AssetManager] Unloading assets..." << std::endl;

    UnloadTexture(menuBackground);
    UnloadTexture(playerTexture);
    UnloadTexture(bossTexture);
    UnloadTexture(itemKeyTexture);
    UnloadTexture(buffItemTexture);

    UnloadMusicStream(bgmMenu);
    UnloadMusicStream(bgmGameplay);
    UnloadMusicStream(bgmBoss);

    UnloadSound(soundShoot);
    UnloadSound(soundSlash);
    UnloadSound(soundClick);
    UnloadSound(soundExpand);
    UnloadSound(soundHit);
    UnloadSound(soundPickup);

    UnloadFont(gameFont);

    std::cout << "[AssetManager] Memory cleared safely!" << std::endl;

}

// ============================================================================
// 隊友調用接口實作 (Getter Functions)
// ============================================================================
Texture2D AssetManager::GetMenuBackground() { return menuBackground; }
Texture2D AssetManager::GetPlayerTexture() { return playerTexture; }
Texture2D AssetManager::GetBossTexture() { return bossTexture; }
Texture2D AssetManager::GetItemKeyTexture() { return itemKeyTexture; }
Texture2D AssetManager::GetBuffItemTexture() { return buffItemTexture; }

Music AssetManager::GetBgmMenu() { return bgmMenu; }
Music AssetManager::GetBgmGameplay() { return bgmGameplay; }
Music AssetManager::GetBgmBoss() { return bgmBoss; }

Sound AssetManager::GetSoundShoot() { return soundShoot; }
Sound AssetManager::GetSoundSlash() { return soundSlash; }
Sound AssetManager::GetSoundClick() { return soundClick; }
Sound AssetManager::GetSoundExpand() { return soundExpand; }
Sound AssetManager::GetSoundHit() { return soundHit; }
Sound AssetManager::GetSoundPickup() { return soundPickup; }

Font AssetManager::GetGameFont() { return gameFont; }

void AssetManager::DrawPlayerAnimated(Vector2 position, Color tint) {
    // 每一格的實際寬度（例如圖片總寬 128 / 4 格 = 32 像素）
    int frameWidth = playerTexture.width / PLAYER_MAX_FRAMES;

    playerTimer += GetFrameTime(); // 累加 Raylib 的每影格三角時間 (Delta Time)
    if (playerTimer >= PLAYER_FRAME_SPEED) {
        playerTimer = 0.0f;
        playerCurrentFrame = (playerCurrentFrame + 1) % PLAYER_MAX_FRAMES; // 0->1->2->3->0 循環
    }
    // 計算要裁剪的方形範圍
    Rectangle srcRec = { (float)playerCurrentFrame * frameWidth, 0.0f, (float)frameWidth, (float)playerTexture.height };

    // 設定要在畫面上畫多大
    Rectangle destRec = { position.x, position.y, (float)frameWidth, (float)playerTexture.height };

    // 將中心錨點（Anchor）設定在正中央，這樣程式旋轉或算碰撞時最精準
    Vector2 origin = { (float)frameWidth / 2.0f, (float)playerTexture.height / 2.0f };

    // 呼叫 Raylib 進階繪製
    DrawTexturePro(playerTexture, srcRec, destRec, origin, 0.0f, tint);
}

// 2. 怪物動畫繪製：因為場上怪物很多，各自的計時器（指標）由怪物自己保管，你只負責算公式
void AssetManager::DrawEnemyAnimated(Vector2 position, float* animTimer, int* currentFrame, Color tint) {
    int maxFrames = 4; // 假設怪物也是 4 格動畫
    int frameWidth = enemyTexture.width / maxFrames;
    float frameSpeed = 0.15f; // 怪物動得稍微慢一點點

    // 透過指標更新該隻怪物自己的計時器
    *animTimer += GetFrameTime();
    if (*animTimer >= frameSpeed) {
        *animTimer = 0.0f;
        *currentFrame = (*currentFrame + 1) % maxFrames;
    }

    Rectangle srcRec = { (float)(*currentFrame) * frameWidth, 0.0f, (float)frameWidth, (float)enemyTexture.height };
    Rectangle destRec = { position.x, position.y, (float)frameWidth, (float)enemyTexture.height };
    Vector2 origin = { (float)frameWidth / 2.0f, (float)enemyTexture.height / 2.0f };

    DrawTexturePro(enemyTexture, srcRec, destRec, origin, 0.0f, tint);
}

// 產生粒子 (只需給座標和顏色)
void AssetManager::EmitParticle(Vector2 position, Color color, float size) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            particles[i].active = true;
            particles[i].position = position;
            // 給一個隨機的散發方向
            particles[i].velocity = { (float)GetRandomValue(-30, 30) / 40.0f, (float)GetRandomValue(-30, 30) / 40.0f };
            particles[i].color = color;
            particles[i].alpha = 0.3f;
            particles[i].lifeTime = 1.0f; // 存活 1 秒
            particles[i].size = size;
            //TraceLog(LOG_INFO, "Particle emitted at: %f, %f", position.x, position.y); // 檢查有沒有噴出來
            break;
        }
    }
}

// 更新邏輯
void AssetManager::UpdateParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            particles[i].position.x += particles[i].velocity.x;
            particles[i].position.y += particles[i].velocity.y;
            particles[i].lifeTime -= GetFrameTime();
            particles[i].alpha = particles[i].lifeTime; // 漸漸透明

            if (particles[i].lifeTime <= 0) particles[i].active = false;
        }
    }
}

// 繪製邏輯
void AssetManager::DrawParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            //TraceLog(LOG_DEBUG, "Drawing particle %d at: %f, %f", i, particles[i].position.x, particles[i].position.y);
            DrawCircleV(particles[i].position, particles[i].size, Fade(particles[i].color, particles[i].alpha));
        }
    }
}