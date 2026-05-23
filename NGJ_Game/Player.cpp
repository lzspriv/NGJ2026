#include "Player.h"
#include <cmath> // 用於 sqrtf 計算子彈向量

PlayerManager::PlayerManager() {
    InitPlayer();
}

PlayerManager::~PlayerManager() {
    // 暫無動態記憶體釋放需求
}

void PlayerManager::InitPlayer() {
    // 初始狀態設定
    playerPos = { 200.0f, 200.0f };
    playerSpeed = 5.0f; // 保持與你原本 main.cpp 一致的每影格移動速度
    maxHp = 3;
    currentHp = maxHp;
    score = 0;
    bullets.clear();

    currentWinWidth = 400;
    currentWinHeight = 400;
    currentWinPos = { 500.0f, 300.0f };
}

void PlayerManager::HandleInput() {
    // 獲取目前視窗在螢幕上的最新絕對座標
    currentWinPos = GetWindowPosition();

    // 獲取目前視窗所在的螢幕資訊，確保擴張不會超出螢幕邊界
    int monitor = GetCurrentMonitor();
    int maxWidth = GetMonitorWidth(monitor);
    int maxHeight = GetMonitorHeight(monitor);

    // --- 1. 支援 WASD 與 方向鍵 移動 ---
    if ((IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) && playerPos.x <= maxWidth - currentWinPos.x - 25) {
        playerPos.x += playerSpeed;
    }
    if ((IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) && playerPos.x >= 5) {
        playerPos.x -= playerSpeed;
    }
    if ((IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) && playerPos.y <= maxHeight - currentWinPos.y - 25) {
        playerPos.y += playerSpeed;
    }
    if ((IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) && playerPos.y >= 5) {
        playerPos.y -= playerSpeed;
    }

    // --- 2. 滑鼠點擊：朝滑鼠方向發射子彈 ---
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mousePos = GetMousePosition();

        // 計算從玩家到滑鼠的方向向量 (Target - Source)
        Vector2 dir = { mousePos.x - playerPos.x, mousePos.y - playerPos.y };
        float length = sqrtf(dir.x * dir.x + dir.y * dir.y);

        if (length > 0.0f) {
            // 單位化向量 (Normalize)
            dir.x /= length;
            dir.y /= length;

            // 設定子彈速度 (假設子彈速度為 8.0f)
            Bullet newBullet;
            newBullet.pos = playerPos;
            newBullet.speed = { dir.x * 8.0f, dir.y * 8.0f };
            newBullet.active = true;

            // 加入 bullets 向量中
            bullets.push_back(newBullet);
        }
    }
}

void PlayerManager::UpdatePlayerAndWindow(float dt) {
    // 再次確保獲取最新視窗與螢幕邊界資訊
    currentWinPos = GetWindowPosition();
    int monitor = GetCurrentMonitor();
    int maxWidth = GetMonitorWidth(monitor);
    int maxHeight = GetMonitorHeight(monitor);

    // --- 1. 核心四向視窗推動/擴張邏輯 ---

    // ➡️ 往右擴張
    if (playerPos.x >= currentWinWidth - 20 && playerPos.x <= maxWidth - currentWinPos.x - 20) {
        currentWinWidth += 10;
        SetWindowSize(currentWinWidth, currentWinHeight);
    }

    // ⬇️ 往下擴張
    if (playerPos.y >= currentWinHeight - 20 && playerPos.y <= maxHeight - currentWinPos.y - 20) {
        currentWinHeight += 10;
        SetWindowSize(currentWinWidth, currentWinHeight);
    }

    // ⬅️ 往左擴張 (關鍵修正)
    if (playerPos.x <= 0 && currentWinPos.x > 0) {
        currentWinWidth += 10;                               // 1. 視窗變寬
        SetWindowPosition(currentWinPos.x - 10, currentWinPos.y); // 2. 視窗左移
        SetWindowSize(currentWinWidth, currentWinHeight);         // 3. 同步更新尺寸
        playerPos.x += 10;                                   // 4. 主角相對座標右移，防止卡牆
    }

    // ⬆️ 往上擴張 (關鍵修正)
    if (playerPos.y <= 0 && currentWinPos.y > 40) {
        currentWinHeight += 10;                              // 1. 視窗變高
        SetWindowPosition(currentWinPos.x, currentWinPos.y - 10); // 2. 視窗上移
        SetWindowSize(currentWinWidth, currentWinHeight);         // 3. 同步更新尺寸
        playerPos.y += 10;                                   // 4. 主角相對座標下移，防止卡牆
    }

    // --- 2. 更新子彈位置與邊界回收 ---
    for (size_t i = 0; i < bullets.size(); i++) {
        if (bullets[i].active) {
            bullets[i].pos.x += bullets[i].speed.x;
            bullets[i].pos.y += bullets[i].speed.y;

            // 飛出目前視窗範圍就標記為無效
            if (bullets[i].pos.x < 0 || bullets[i].pos.x > currentWinWidth ||
                bullets[i].pos.y < 0 || bullets[i].pos.y > currentWinHeight) {
                bullets[i].active = false;
            }
        }
    }

    // 清除不活躍的子彈，釋放 Vector 記憶體
    for (auto it = bullets.begin(); it != bullets.end();) {
        if (!it->active) {
            it = bullets.erase(it);
        }
        else {
            ++it;
        }
    }
}

void PlayerManager::DrawPlayer() {
    // 1. 畫出所有子彈 (黃色圓點)
    for (const auto& bullet : bullets) {
        if (bullet.active) {
            DrawCircleV(bullet.pos, 4.0f, YELLOW);
        }
    }

    // 2. 畫出主角 (20x20 藍色方塊)
    DrawRectangleV(playerPos, { 20.0f, 20.0f }, BLUE);
}