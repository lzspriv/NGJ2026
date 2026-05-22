#pragma once
#include "raylib.h"

class AssetManager {
public:
    // ---- 美術貼圖 (Textures) ----
    Texture2D menuBackground;    // 主畫面 AI 產出的帥氣黑客風背景圖
    Texture2D playerTexture;     // 主角小藍方塊的替代精細貼圖
    Texture2D bossTexture;       // 最終巨大 Boss 的外觀貼圖
    Texture2D itemKeyTexture;    // 鑰匙的圖示
    Texture2D buffItemTexture;   // 增益道具的圖示

    // ---- 音效與音樂 (Audio) ----
    Music bgmMenu;               // 主選單的背景音樂
    Music bgmGameplay;           // 戰鬥關卡隨時間緊迫的熱血 BGM
    Music bgmBoss;               // Boss 戰的極度壓迫感電子音樂
    Sound soundShoot;            // 玩家滑鼠點擊發射子彈音效
    Sound soundExpand;           // 視窗被肉身推大、長大時的機械音效
    Sound soundHit;              // 玩家被怪物撞擊、扣血的警報音效
    Sound soundPickup;           // 吃到增益道具或鑰匙的音效

    // ---- 視覺字體 (Fonts) ----
    Font gameFont;               // 讓遊戲 UI 文字變帥的黑客風字體

    AssetManager();
    ~AssetManager();

    // 關鍵函式：在遊戲一啟動時，負責去硬碟把所有圖片、音效讀取進來
    void LoadAllAssets();

    // 關鍵函式：在遊戲關閉時，負責把所有記憶體釋放（Raylib 規定一定要做，否則會記憶體洩漏）
    void UnloadAllAssets();
};