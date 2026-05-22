#include "raylib.h"

int main() {
    int currentWidth = 400;
    int currentHeight = 400;

    // 先讓視窗出現在螢幕中間偏左上的位置，方便測試擴張
    InitWindow(currentWidth, currentHeight, "NGJ2026 - 4D Window Expand!");
    SetWindowPosition(500, 300);
    SetTargetFPS(60);

    Vector2 playerPos = { 200.0f, 200.0f };
    float playerSpeed = 5.0f;

    while (!WindowShouldClose()) {
        // 基礎移動控制
        if (IsKeyDown(KEY_RIGHT)) playerPos.x += playerSpeed;
        if (IsKeyDown(KEY_LEFT))  playerPos.x -= playerSpeed;
        if (IsKeyDown(KEY_DOWN))  playerPos.y += playerSpeed;
        if (IsKeyDown(KEY_UP))    playerPos.y -= playerSpeed;

        // 獲取目前視窗在螢幕上的絕對座標
        Vector2 winPos = GetWindowPosition();

        // 1. ➡️ 往右擴張 (維持原樣)
        if (playerPos.x >= currentWidth - 20) {
            currentWidth += 10;
            SetWindowSize(currentWidth, currentHeight);
        }

        // 2. ⬇️ 往下擴張 (維持原樣)
        if (playerPos.y >= currentHeight - 20) {
            currentHeight += 10;
            SetWindowSize(currentWidth, currentHeight);
        }

        // 3. ⬅️ 往左擴張 (關鍵！)
        // 當主角逼近左邊界 (x <= 20) 時：
        if (playerPos.x <= 20) {
            currentWidth += 10; // 1. 視窗變寬
            // 2. 把整個視窗往左移 10 像素
            SetWindowPosition(winPos.x - 10, winPos.y);
            // 3. 強迫將視窗大小同步更新
            SetWindowSize(currentWidth, currentHeight);
            // 4. 因為視窗往左長了 10 像素，主角在視窗內的相對座標必須右移 10 像素，才不會卡在牆壁裡
            playerPos.x += 10;
        }

        // 4. ⬆️ 往上擴張 (關鍵！)
        // 當主角逼近上邊界 (y <= 20) 時：
        if (playerPos.y <= 20) {
            currentHeight += 10; // 1. 視窗變高
            // 2. 把整個視窗往上移 10 像素
            SetWindowPosition(winPos.x, winPos.y - 10);
            SetWindowSize(currentWidth, currentHeight);
            // 3. 主角在視窗內的相對座標下移 10 像素
            playerPos.y += 10;
        }

        // 繪製畫面
        BeginDrawing();
        ClearBackground(BLACK);

        DrawText("4-Directional Expansion!", 20, 20, 18, GREEN);

        // 畫出主角小藍方塊
        DrawRectangleV(playerPos, { 20, 20 }, BLUE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}