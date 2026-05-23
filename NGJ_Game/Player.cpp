#include "Player.h"
#include <cmath> // 用於 sqrtf, atan2f, PI 運算

PlayerManager::PlayerManager() {
    InitPlayer();
}

PlayerManager::~PlayerManager() {
}

void PlayerManager::InitPlayer() {
    playerPos = { 200.0f, 200.0f };
    playerSpeed = 5.0f;
    maxHp = 10;
    currentHp = maxHp;
    score = 0;
    attackRange = 50.0f;   // 預設攻擊距離
    bullets.clear();

    currentMode = MODE_SHOOT;

    // 初始化動態揮劍屬性
    sword.center = { 0, 0 };
    sword.startAngle = 0.0f;
    sword.endAngle = 0.0f;
    sword.currentAngle = 0.0f;
    sword.duration = 0.15f;    // 揮劍動作花費 0.15 秒，可以調大來測試慢動作
    sword.activeTimer = 0.0f;
    sword.cooldownTimer = 0.0f;
    sword.active = false;

    currentWinWidth = 400;
    currentWinHeight = 400;
    currentWinPos = { 500.0f, 300.0f };
}

void PlayerManager::HandleInput() {
    currentWinPos = GetWindowPosition();
    int monitor = GetCurrentMonitor();
    int maxWidth = GetMonitorWidth(monitor);
    int maxHeight = GetMonitorHeight(monitor);

    // --- WASD + 方向鍵移動 ---
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

    }

    void PlayerManager::ProcessCombatInput() {
        // 攻擊模式切換
        if (IsKeyPressed(KEY_ONE)) {
            currentMode = MODE_SHOOT;
        }
        if (IsKeyPressed(KEY_TWO)) {
            currentMode = MODE_MELEE;
        }

        // 滑鼠點擊攻擊
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mousePos = GetMousePosition();
            Vector2 playerCenter = { playerPos.x + 10.0f, playerPos.y + 10.0f };
            Vector2 dir = { mousePos.x - playerCenter.x, mousePos.y - playerCenter.y };
            float length = sqrtf(dir.x * dir.x + dir.y * dir.y);

            if (length > 0.0f) {
                dir.x /= length;
                dir.y /= length;

                if (currentMode == MODE_SHOOT) {
                    Bullet newBullet;
                    newBullet.pos = playerCenter; // spawn from player's center
                    newBullet.speed = { dir.x * 8.0f, dir.y * 8.0f };
                    newBullet.active = true;
                    bullets.push_back(newBullet);
                }
                else if (currentMode == MODE_MELEE && sword.cooldownTimer <= 0.0f) {
                    sword.active = true;
                    sword.activeTimer = sword.duration;
                    sword.cooldownTimer = 0.4f;

                    float targetAngleRad = atan2f(dir.y, dir.x);
                    float targetAngleDeg = targetAngleRad * (180.0f / PI);
                    float slashArc = 90.0f;
                    sword.startAngle = targetAngleDeg - (slashArc / 2.0f);
                    sword.endAngle = targetAngleDeg + (slashArc / 2.0f);
                    sword.currentAngle = sword.startAngle;
                    sword.center = playerCenter;
                }
            }
        }
    }

void PlayerManager::UpdatePlayerAndWindow(float dt) {
    currentWinPos = GetWindowPosition();
    int monitor = GetCurrentMonitor();
    int maxWidth = GetMonitorWidth(monitor);
    int maxHeight = GetMonitorHeight(monitor);

    // --- 核心四向視窗推動邏輯 ---
    if (playerPos.x >= currentWinWidth - 20 && playerPos.x <= maxWidth - currentWinPos.x - 20) {
        currentWinWidth += 10;
        SetWindowSize(currentWinWidth, currentWinHeight);
    }
    if (playerPos.y >= currentWinHeight - 20 && playerPos.y <= maxHeight - currentWinPos.y - 20) {
        currentWinHeight += 10;
        SetWindowSize(currentWinWidth, currentWinHeight);
    }
    if (playerPos.x <= 0 && currentWinPos.x > 0) {
        currentWinWidth += 10;
        SetWindowPosition(currentWinPos.x - 10, currentWinPos.y);
        SetWindowSize(currentWinWidth, currentWinHeight);
        playerPos.x += 10;
    }
    if (playerPos.y <= 0 && currentWinPos.y > 40) {
        currentWinHeight += 10;
        SetWindowPosition(currentWinPos.x, currentWinPos.y - 10);
        SetWindowSize(currentWinWidth, currentWinHeight);
        playerPos.y += 10;
    }

    // --- 核心功能：更新動態揮劍的旋轉角度 ---
    if (sword.active) {
        sword.center = { playerPos.x + 10.0f, playerPos.y + 10.0f };
        sword.activeTimer -= dt;

        if (sword.activeTimer <= 0.0f) {
            sword.active = false;
        }
        else {
            // 計算揮劍的進度比率 (從 0.0 到 1.0)
            // 剛點擊時 activeTimer 等於 duration，比率為 0；快結束時 activeTimer 接近 0，比率為 1
            float progress = (sword.duration - sword.activeTimer) / sword.duration;

            // 讓目前的角度根據進度在起點與終點之間進行線性插值 (Lerp)
            sword.currentAngle = sword.startAngle + (sword.endAngle - sword.startAngle) * progress;
        }
    }

    if (sword.cooldownTimer > 0.0f) {
        sword.cooldownTimer -= dt;
    }

    // --- 更新子彈位置與邊界回收 (window-local) ---
    for (size_t i = 0; i < bullets.size(); i++) {
        if (bullets[i].active) {
            bullets[i].pos.x += bullets[i].speed.x;
            bullets[i].pos.y += bullets[i].speed.y;

            if (bullets[i].pos.x < 0 || bullets[i].pos.x > currentWinWidth ||
                bullets[i].pos.y < 0 || bullets[i].pos.y > currentWinHeight) {
                bullets[i].active = false;
            }
        }
    }

    for (auto it = bullets.begin(); it != bullets.end();) {
        if (!it->active) {
            it = bullets.erase(it);
        }
        else {
            ++it;
        }
    }
}

void PlayerManager::UpdateCombat(float dt) {
    // 更新揮劍狀態
    if (sword.active) {
        sword.center = { playerPos.x + 10.0f, playerPos.y + 10.0f };
        sword.activeTimer -= dt;

        if (sword.activeTimer <= 0.0f) {
            sword.active = false;
        }
        else {
            float progress = (sword.duration - sword.activeTimer) / sword.duration;
            sword.currentAngle = sword.startAngle + (sword.endAngle - sword.startAngle) * progress;
        }
    }

    if (sword.cooldownTimer > 0.0f) {
        sword.cooldownTimer -= dt;
    }

    // 子彈更新（已在 window-local 座標運算）
    for (size_t i = 0; i < bullets.size(); i++) {
        if (bullets[i].active) {
            bullets[i].pos.x += bullets[i].speed.x;
            bullets[i].pos.y += bullets[i].speed.y;

            if (bullets[i].pos.x < 0 || bullets[i].pos.x > currentWinWidth ||
                bullets[i].pos.y < 0 || bullets[i].pos.y > currentWinHeight) {
                bullets[i].active = false;
            }
        }
    }

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
    // 1. 畫出子彈
    for (const auto& bullet : bullets) {
        if (bullet.active) {
            DrawCircleV(bullet.pos, 4.0f, YELLOW);
        }
    }

    // 2. 核心功能：畫出沿著扇形軌跡真正移動/旋轉的長方形劍
    if (sword.active) {
        // 設定長方形劍的尺寸：細長的實體長方形
        Rectangle swordRect = { sword.center.x, sword.center.y, 55.0f, 5.0f };

        // 旋轉錨點設在劍的底部中心
        Vector2 origin = { 0.0f, 2.5f };

        // 畫出長方形劍身 (這次它會隨著每一幀的 currentAngle 產生旋轉動畫！)
        DrawRectanglePro(swordRect, origin, sword.currentAngle, RAYWHITE);

        // 可選：可以加上一個超淡的劍氣軌跡當襯底，不想看見殘影可以把下面這行刪除
        // DrawCircleSector(sword.center, 55.0f, sword.startAngle, sword.currentAngle, 16, Fade(SKYBLUE, 0.2f));
    }

    // 3. 畫出主角
    Color playerColor = (currentMode == MODE_SHOOT) ? BLUE : ORANGE;
    DrawRectangleV(playerPos, { 20.0f, 20.0f }, playerColor);

    // 印出目前的攻擊模式狀態
    DrawText(currentMode == MODE_SHOOT ? "MODE: SHOOT [1]" : "MODE: MELEE [2]", 20, 110, 16, RAYWHITE);
}
