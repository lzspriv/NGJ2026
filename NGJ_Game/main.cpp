// Raylib visual test harness: spawn window and enemies
#include "raylib.h"
#include "Enemy.h"
#include <vector>
#include <string>

int main() {
    const int screenW = 800;
    const int screenH = 600;
    InitWindow(screenW, screenH, "NGJ Enemy Visual Test");
    SetTargetFPS(60);

    // Player
    NGJ::Vec2 playerPos((float)screenW / 2.0f, (float)screenH / 2.0f);
    float playerSpeed = 200.0f; // px/s
    int playerHP = 30;

    // Spawn some enemies around the room
    std::vector<NGJ::Enemy> enemies;
    enemies.emplace_back("Goblin", 8, 2, 0, 40.0f, 180.0f, 24.0f, 1.0f, NGJ::Vec2(100.0f, 100.0f));
    enemies.emplace_back("Wolf", 12, 3, 1, 80.0f, 220.0f, 20.0f, 1.2f, NGJ::Vec2(700.0f, 120.0f));
    enemies.emplace_back("Slime", 6, 1, 0, 30.0f, 120.0f, 18.0f, 0.8f, NGJ::Vec2(150.0f, 500.0f));
    enemies.emplace_back("Bat", 5, 1, 0, 120.0f, 160.0f, 14.0f, 0.6f, NGJ::Vec2(650.0f, 420.0f));

    // Set patrol targets for some
    enemies[0].patrolTarget = NGJ::Vec2(200.0f, 120.0f);
    enemies[2].patrolTarget = NGJ::Vec2(120.0f, 520.0f);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // Player input
        if (IsKeyDown(KEY_RIGHT)) playerPos.x += playerSpeed * dt;
        if (IsKeyDown(KEY_LEFT)) playerPos.x -= playerSpeed * dt;
        if (IsKeyDown(KEY_DOWN)) playerPos.y += playerSpeed * dt;
        if (IsKeyDown(KEY_UP)) playerPos.y -= playerSpeed * dt;

        // Keep player on screen
        if (playerPos.x < 0) playerPos.x = 0;
        if (playerPos.y < 0) playerPos.y = 0;
        if (playerPos.x > screenW) playerPos.x = screenW;
        if (playerPos.y > screenH) playerPos.y = screenH;

        // Update enemies
        for (auto &e : enemies) {
            e.Update(dt, playerPos);
            if (e.GetState() == NGJ::EnemyState::Attack && e.CanAttack() && !e.GetIsDead()) {
                int dmg = e.Attack();
                playerHP -= dmg;
            }
        }

        // Draw
        BeginDrawing();
        ClearBackground(BLACK);

        // Draw player
        DrawRectangle((int)playerPos.x - 10, (int)playerPos.y - 10, 20, 20, BLUE);
        DrawText(TextFormat("Player HP: %d", playerHP), 10, 10, 20, WHITE);

        // Draw enemies
        for (const auto &e : enemies) {
            NGJ::Vec2 p = e.GetPosition();
            Color col = GRAY;
            switch (e.GetState()) {
            case NGJ::EnemyState::Idle: col = LIGHTGRAY; break;
            case NGJ::EnemyState::Patrol: col = GREEN; break;
            case NGJ::EnemyState::Chase: col = ORANGE; break;
            case NGJ::EnemyState::Attack: col = RED; break;
            case NGJ::EnemyState::Dead: col = DARKGRAY; break;
            }
            // Draw body
            DrawCircle((int)p.x, (int)p.y, 12, col);
            // HP bar
            int barW = 30;
            int barH = 5;
            int hpW = (int)((float)e.GetCurrentHP() / (float)e.GetMaxHP() * barW);
            DrawRectangle((int)p.x - barW/2, (int)p.y - 20, barW, barH, DARKGRAY);
            DrawRectangle((int)p.x - barW/2, (int)p.y - 20, hpW, barH, RED);
            DrawText(e.name.c_str(), (int)p.x - 16, (int)p.y + 16, 10, BLACK);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
