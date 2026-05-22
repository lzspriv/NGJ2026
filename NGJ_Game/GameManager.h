#pragma once
#include "raylib.h"

// 遊戲八大狀態
enum class GameState {
    MENU,           // 主畫面（開始按鍵、設定、退出）
    PLAYING_LV1,    // 第一關：尋找鑰匙、推視窗開迷霧
    PLAYING_LV2,    // 第二關：引入雷射與新怪物、增加阻礙
    PLAYING_LV3,    // 第三關：火力升級、瘋狂敵方彈幕
    BOSS_BATTLE,    // 最終關：Boss戰、核心擊破、視窗自動壓縮縮小
    PAUSE,          // 暫停畫面（設定、回到主畫面）
    WIN,            // 勝利結算（計分數、遊戲時間，可回到主畫面）
    LOSE            // 失敗結算（計分數、遊戲時間，可回到主畫面）
};

// 負責統整全域遊戲邏輯的核心類別介面
class GameManager {
public:
    GameState currentState;
    float globalTimer;      // 記錄總遊玩時間，確保超過 3 分鐘 
    bool isPaused;

    GameManager();
    ~GameManager();

    // 初始化遊戲（重新開始時呼叫）
    void InitGame();

    // 每一幀更新遊戲邏輯（控管計時器與狀態切換）
    void Update(float dt);

    // 負責繪製目前狀態對應的 UI 畫面（主畫面、暫停、結算、時間分數顯示）
    void DrawUI();
};