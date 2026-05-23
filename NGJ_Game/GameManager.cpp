#include "GameManager.h"
#include <iostream>
#include <string>

// 注：明早組員們把檔案推上來後，記得在最上方 #include 他們的標頭檔：
// #include "Player.h"
// #include "Map.h"
// #include "Enemy.h"
// #include "AssetManager.h"

// 這裡先建立外部全域物件的指標，方便大腦調用（或者你可以宣告在 GameManager 類別內）
// 外掛模組宣告範例：
// PlayerManager* player = nullptr;
// MapManager* gameMap = nullptr;
// EnemyManager* enemy = nullptr;
// AssetManager* assets = nullptr;

GameManager::GameManager() {
    currentState = GameState::MENU;
    globalTimer = 0.0f;
    isPaused = false;
}

GameManager::~GameManager() {
    // 析構函數，釋放動態記憶體
}

void GameManager::InitGame() {
    currentState = GameState::PLAYING_LV1;
    globalTimer = 0.0f;
    isPaused = false;

    // TODO: 呼叫其他組員的初始化函式
    // player->InitPlayer();
    // gameMap->LoadMapData(1);
    // enemy->activeEnemies.clear();
}

void GameManager::Update(float dt) {
    // 1. 處理全域按鍵：隨時可以按下 ESC 鍵切換暫停狀態
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (currentState == GameState::PLAYING_LV1 ||
            currentState == GameState::PLAYING_LV2 ||
            currentState == GameState::PLAYING_LV3 ||
            currentState == GameState::BOSS_BATTLE) {

            isPaused = !isPaused;
            if (isPaused) {
                currentState = GameState::PAUSE;
            }
            else {
                // 解除暫停時，根據目前計時器自動彈回對應關卡
                if (globalTimer < 45.0f) currentState = GameState::PLAYING_LV1;
                else if (globalTimer < 90.0f) currentState = GameState::PLAYING_LV2;
                else if (globalTimer < 135.0f) currentState = GameState::PLAYING_LV3;
                else currentState = GameState::BOSS_BATTLE;
            }
        }
    }

    // 2. 依據不同狀態執行對應的計時器與關卡切換大腦邏輯
    switch (currentState) {
    case GameState::MENU:
        // 主選單邏輯：檢測滑鼠是否點擊 START 鍵
        if (IsKeyPressed(KEY_ENTER)) { // 暫時用 Enter 鍵代替點擊測試
            InitGame();
        }
        if (IsKeyPressed(KEY_Q)) { // 按 Q 退出
            // 這裡可以連動主循環關閉
        }
        break;

    case GameState::PLAYING_LV1:
        globalTimer += dt;

        // TODO: 呼叫組員的 Update 邏輯
        // player->HandleInput();
        // player->UpdatePlayerAndWindow(dt);
        // enemy->SpawnEnemy(1, player->playerPos);
        // enemy->UpdateEnemiesAndBoss(...);

        // 時間軸切換：撐過 45 秒強進第二關（或者拿到鑰匙）
        if (globalTimer >= 45.0f) {
            currentState = GameState::PLAYING_LV2;
            // gameMap->LoadMapData(2); // 載入第二關阻礙
        }

        // 失敗判定
        // if (player->currentHp <= 0) currentState = GameState::LOSE;
        break;

    case GameState::PLAYING_LV2:
        globalTimer += dt;

        // TODO: 呼叫組員邏輯（此時 EnemyManager 開始刷出雷射陷阱）

        // 時間軸切換：90 秒進入第三關
        if (globalTimer >= 90.0f) {
            currentState = GameState::PLAYING_LV3;
            // gameMap->LoadMapData(3);
        }
        break;

    case GameState::PLAYING_LV3:
        globalTimer += dt;

        // TODO: 呼叫組員邏輯（此時彈幕數量暴增）

        // 時間軸切換：135 秒（2分15秒）迎來最終大壓縮 Boss 戰
        if (globalTimer >= 135.0f) {
            currentState = GameState::BOSS_BATTLE;
            // enemy->isBossSpawned = true;
        }
        break;

    case GameState::BOSS_BATTLE:
        globalTimer += dt;

        // TODO: 呼叫組員邏輯（EnemyManager 開始強制每秒扣減視窗寬高）

        // 勝利與失敗條件判定
        // if (enemy->bossHp <= 0) currentState = GameState::WIN;
        // if (player->currentHp <= 0) currentState = GameState::LOSE;
        break;

    case GameState::PAUSE:
        // 暫停中：按 Enter 鍵返回遊戲，按 M 鍵回主選單
        if (IsKeyPressed(KEY_ENTER)) {
            isPaused = false;
            // 彈回計時器對應關卡
            if (globalTimer < 45.0f) currentState = GameState::PLAYING_LV1;
            else if (globalTimer < 90.0f) currentState = GameState::PLAYING_LV2;
            else if (globalTimer < 135.0f) currentState = GameState::PLAYING_LV3;
            else currentState = GameState::BOSS_BATTLE;
        }
        if (IsKeyPressed(KEY_M)) {
            currentState = GameState::MENU;
        }
        break;

    case GameState::WIN:
    case GameState::LOSE:
        // 結算畫面：按 R 鍵重新開始，按 M 鍵回主畫面
        if (IsKeyPressed(KEY_R)) {
            InitGame();
        }
        if (IsKeyPressed(KEY_M)) {
            currentState = GameState::MENU;
        }
        break;
    }
}

void GameManager::DrawUI() {
    // 獲取目前視窗的即時寬高（因為組員A會一直改動視窗大小，UI文字必須動態靠邊置中）
    int winWidth = GetScreenWidth();
    int winHeight = GetScreenHeight();

    switch (currentState) {
    case GameState::MENU:
        ClearBackground(BLACK);
        DrawText("NGJ 2026: ABSOLUTE EXPANSION", winWidth / 2 - 160, winHeight / 2 - 60, 20, GREEN);
        DrawText("Press [ENTER] to Start Game", winWidth / 2 - 130, winHeight / 2, 16, RAYWHITE);
        DrawText("Press [Q] to Quit", winWidth / 2 - 70, winHeight / 2 + 40, 16, GRAY);
        break;

    case GameState::PLAYING_LV1:
    case GameState::PLAYING_LV2:
    case GameState::PLAYING_LV3:
    case GameState::BOSS_BATTLE:
        // 這裡交給各個模組畫地圖跟主角，大腦只負責在最上層覆蓋顯示時間與血量
        // TODO: 
        // gameMap->DrawMap(winWidth, winHeight);
        // player->DrawPlayer();
        // enemy->DrawEnemies();

        // 頂部 HUD 資訊顯示
        DrawText(TextFormat("TIME: %.1fs", globalTimer), 20, 20, 16, GREEN);
        DrawText("HP: ||||||||", 20, 45, 16, RED); // 之後連動 player->currentHp

        if (currentState == GameState::BOSS_BATTLE) {
            DrawText("CRITICAL: WINDOW COMPRESSING!", winWidth / 2 - 120, 20, 16, ORANGE);
        }
        break;

    case GameState::PAUSE:
        // 暫停時繪製一層半透明灰色黑幕遮罩，很有質感
        DrawRectangle(0, 0, winWidth, winHeight, ColorAlpha(BLACK, 0.6f));
        DrawText("GAME PAUSED", winWidth / 2 - 65, winHeight / 2 - 30, 20, LIGHTGRAY);
        DrawText("Press [ENTER] to Resume", winWidth / 2 - 100, winHeight / 2 + 10, 16, RAYWHITE);
        DrawText("Press [M] for Main Menu", winWidth / 2 - 100, winHeight / 2 + 40, 16, GRAY);
        break;

    case GameState::WIN:
        ClearBackground(DARKBLUE);
        DrawText("SYSTEM DECRYPTED - YOU WIN!", winWidth / 2 - 150, winHeight / 2 - 40, 20, GREEN);
        DrawText(TextFormat("Total Time: %.1f seconds", globalTimer), winWidth / 2 - 100, winHeight / 2, 16, RAYWHITE);
        DrawText("Press [R] to Restart / [M] for Menu", winWidth / 2 - 160, winHeight / 2 + 50, 16, LIGHTGRAY);
        break;

    case GameState::LOSE:
        ClearBackground(MAROON);
        DrawText("CONNECTION LOST - GAME OVER", winWidth / 2 - 150, winHeight / 2 - 40, 20, RED);
        DrawText(TextFormat("Survived Time: %.1f seconds", globalTimer), winWidth / 2 - 100, winHeight / 2, 16, RAYWHITE);
        DrawText("Press [R] to Retry / [M] for Menu", winWidth / 2 - 150, winHeight / 2 + 50, 16, LIGHTGRAY);
        break;
    }
}