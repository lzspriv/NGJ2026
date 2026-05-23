#include "raylib.h"
#include "Player.h"

int main() {
    // 1. 實例化玩家與視窗管理員
    PlayerManager player;

    // 2. 初始化視窗（預設 400x400，置於螢幕中間偏左上，方便測試往右下的擴張）
    InitWindow(player.currentWinWidth, player.currentWinHeight, "NGJ2026 - 4D Window & Combat Test!");
    SetWindowPosition((int)player.currentWinPos.x, (int)player.currentWinPos.y);
    SetTargetFPS(60);

    // 遊戲主迴圈
    while (!WindowShouldClose()) {
        // 獲取 Delta Time
        float dt = GetFrameTime();

        // 3. 處理輸入 (WASD/方向鍵移動、1/2切換模式、滑鼠攻擊)
        player.HandleInput();

        // 4. 更新核心邏輯 (包含子彈移動、近戰計時器、四向視窗推動與防卡牆修正)
        player.UpdatePlayerAndWindow(dt);

        // 5. 渲染流程
        BeginDrawing();
        ClearBackground(BLACK);

        // --- 測試輔助視覺效果：從玩家中心畫一條虛擬線指向滑鼠 ---
        Vector2 playerCenter = { player.playerPos.x + 10.0f, player.playerPos.y + 10.0f };
        Vector2 mousePos = GetMousePosition();
        DrawLineV(playerCenter, mousePos, DARKGRAY);

        // 6. 繪製主角（藍/橘方塊）、子彈、以及近戰的長方形劍
        player.DrawPlayer();

        // --- 測試輔助視覺效果：近戰冷卻條提示 ---
        if (player.currentMode == MODE_MELEE && player.sword.cooldownTimer > 0.0f) {
            // 在玩家頭頂畫一條小紅線代表冷卻中 (總冷卻 0.4 秒)
            float cooldownProgress = player.sword.cooldownTimer / 0.4f;
            DrawRectangle((int)player.playerPos.x, (int)player.playerPos.y - 8, (int)(20.0f * cooldownProgress), 4, RED);
        }

        // --- 7. 繪製除錯與操作 UI 面板 ---
        DrawRectangle(10, 10, 220, 140, Fade(GRAY, 0.2f)); // 背景裝飾框
        DrawText("--- COMBAT TEST ---", 20, 20, 16, GREEN);
        DrawText(TextFormat("HP: %d/%d", player.currentHp, player.maxHp), 20, 45, 16, RED);
        DrawText(TextFormat("Win Size: %dx%d", player.currentWinWidth, player.currentWinHeight), 20, 65, 14, RAYWHITE);

        // 顯示目前模式與操作提示
        if (player.currentMode == MODE_SHOOT) {
            DrawText("CURRENT: SHOOTING", 20, 90, 14, BLUE);
            DrawText("[Press 2 to Switch Melee]", 20, 130, 12, LIGHTGRAY);
        }
        else {
            DrawText("CURRENT: MELEE (SWORD)", 20, 90, 14, ORANGE);
            if (player.sword.cooldownTimer > 0.0f) {
                DrawText("[COOLDOWN...]", 20, 130, 12, RED);
            }
            else {
                DrawText("[Ready to Swing! - LeftClick]", 20, 130, 12, GREEN);
            }
        }

        EndDrawing();
    }

    // 8. 釋放資源並關閉視窗
    CloseWindow();
    return 0;
}
