#include "raylib.h"
#include "PlatformUtils.h"
#include "AssetManager.h"
#include "Map.h"
#include "Player.h"
#include "Enemy.h"
#include <vector>
#include <ctime>
#include <cstdlib>
#include <algorithm>

enum class GameState { Menu, Playing, Boss };
GameState currentGameState = GameState::Menu;
GameState lastGameState = GameState::Menu;

void UpdateMusicState(GameState newState) {
	if (newState == lastGameState) return; // 沒有切換場景，直接返回

	// 停止目前的音樂
	StopMusicStream(AssetManager::GetBgmMenu());
	StopMusicStream(AssetManager::GetBgmGameplay());
	StopMusicStream(AssetManager::GetBgmBoss());

	// 根據新場景播放對應音樂
	switch (newState) {
	case GameState::Menu:     PlayMusicStream(AssetManager::GetBgmMenu());     break;
	case GameState::Playing:  PlayMusicStream(AssetManager::GetBgmGameplay()); break;
	case GameState::Boss:     PlayMusicStream(AssetManager::GetBgmBoss());     break;
	}

	lastGameState = newState;
}



int main() {
	// 使用 Player 的視窗初始值以保持先前行為
	int currentWidth = 400;
	int currentHeight = 400;

	// 先讓視窗出現在螢幕中間偏左上的位置，方便測試擴張
	InitWindow(currentWidth, currentHeight, "NGJ2026 - Integration: Map + Player");
	SetExitKey(0); // 【新增】禁用預設的 ESC 關閉視窗，讓我們能用它來暫停
	InitAudioDevice();

	AssetManager::SetGameVolume(0.1f);
	AssetManager::LoadAllAssets();
	Music music = AssetManager::GetBgmGameplay();
	PlayMusicStream(music);
	// 將音樂設為循環
	music.looping = true;

	SetTargetFPS(60);

	// 追蹤前一個所在的顯示器索引
	int prevMonitor = GetCurrentMonitor();

	// 初始化時置中視窗到當前顯示器 - 使用與跨螢幕相同的邏輯
	{
		int monitor = GetCurrentMonitor();
		int monLeft = 0, monTop = 0, monW = GetMonitorWidth(monitor), monH = GetMonitorHeight(monitor);
		if (GetMonitorRectForWindow(GetWindowHandle(), monLeft, monTop, monW, monH)) {
		}

		int newX = monLeft + (monW - currentWidth) / 2;
		int newY = monTop + (monH - currentHeight) / 2;
		SetWindowPosition(newX, newY);
	}

	// 取得扣除 Windows 底部工作列後的真實可用螢幕尺寸
	int initialMonW = GetMonitorWidth(GetCurrentMonitor());
	int initialMonH = GetMonitorHeight(GetCurrentMonitor());
	int tmpL = 0, tmpT = 0;
	if (!GetMonitorWorkAreaForWindow(GetWindowHandle(), tmpL, tmpT, initialMonW, initialMonH)) {
		GetMonitorRectForWindow(GetWindowHandle(), tmpL, tmpT, initialMonW, initialMonH);
	}

	// 初始化地圖系統 (把螢幕真實大小餵給它)
	Map dungeonMap(currentWidth, currentHeight, initialMonW, initialMonH);

	// 實例化玩家管理器（僅用於戰鬥/繪製相關）
	PlayerManager player;
	// 將 player 的視窗尺寸同步
	player.currentWinWidth = currentWidth;
	player.currentWinHeight = currentHeight;

	// 本地玩家在視窗內的相對位置（以地圖邏輯為準）
	Vector2 playerPos = { 200.0f, 200.0f };
	float playerSpeed = 5.0f;
	float playerRadius = 16.0f;

	Camera2D camera = { 0 };
	camera.zoom = 1.0f;

	// 寶箱和獎勵系統
	int currentChestIndex = -1;  // 正在打開的寶箱索引 (-1 表示無)
	int selectedReward = -1;      // 已選擇的獎勵 (0=血量, 1=速度, 2=距離)
	bool showRewardUI = false;    // 是否顯示獎勵選擇 UI
	double rewardUITimer = 0.0;   // 獎勵 UI 顯示計時器
	bool inChestRoom = false;     // 是否在寶箱戰鬥房間
	Vector2 returnWinPos = { 0.0f, 0.0f };
	Vector2 returnPlayerPos = { 0.0f, 0.0f };
	std::vector<NGJ::Enemy> chestRoomEnemies;
	bool showChestEntryPrompt = false;
	int pendingChestIndex = -1;
	int declinedChestIndex = -1;

	// 玩家受傷保護時間（避免每幀連續扣血）
	float playerInvincibleTimer = 0.0f;
	bool isGameOver = false;

	// 週期性怪物生成
	float enemySpawnTimer = 0.0f;
	const float enemySpawnInterval = 7.0f;

	// 怪物管理（全地圖隨機空地生成）
	std::vector<NGJ::Enemy> enemies;
	std::srand((unsigned int)std::time(nullptr));

	// 建立一個新的 Lambda 函式，向地圖請求隨機空地，並轉型為 Enemy 所需的 NGJ::Vec2
	auto spawnRandom = [&]() {
		Vector2 pos = dungeonMap.GetRandomFreePosition();
		return NGJ::Vec2(pos.x, pos.y);
		};

	auto spawnChestRoomEnemies = [&]() {
		chestRoomEnemies.clear();
		int bonus = dungeonMap.GetCurrentLevel() * 2; // <--- 在迴圈外或迴圈內算好 bonus 都可以

		for (int i = 0; i < 3; i++) {
			int typeCount = (dungeonMap.GetCurrentLevel() >= 2) ? 5 : 4;
			int t = std::rand() % typeCount;
			float px = 80.0f + (float)(std::rand() % (currentWidth - 160));
			int upperMinY = 60;
			int upperMaxY = (currentHeight / 2) - 60;
			if (upperMaxY < upperMinY) upperMaxY = upperMinY;
			float py = (float)upperMinY + (float)(std::rand() % (upperMaxY - upperMinY + 1));
			NGJ::Vec2 pos(px, py);

			// 🔻🔻🔻 注意這裡是 chestRoomEnemies 以及 pos 🔻🔻🔻
			switch (t) {
			case 0:
				chestRoomEnemies.emplace_back("Goblin", 8 + bonus, 5 + bonus, 0, 40.0f, 300.0f, 24.0f, 1.0f, pos);
				break;
			case 1:
				chestRoomEnemies.emplace_back("Wolf", 12 + bonus, 5 + bonus, 1, 80.0f, 400.0f, 20.0f, 1.2f, pos);
				chestRoomEnemies.back().SetRangedAttack(true, 260.0f, 0.7f);
				break;
			case 2:
				chestRoomEnemies.emplace_back("Slime", 6 + bonus, 5 + bonus, 0, 30.0f, 250.0f, 18.0f, 0.8f, pos);
				break;
			case 3:
				chestRoomEnemies.emplace_back("Bat", 5 + bonus, 5 + bonus, 0, 120.0f, 350.0f, 14.0f, 0.6f, pos);
				chestRoomEnemies.back().SetRangedAttack(true, 330.0f, 0.45f);
				break;
			default:
				chestRoomEnemies.emplace_back("Assassin", 7 + bonus, 6 + bonus, 0, 20.0f, 420.0f, 18.0f, 0.9f, pos);
				break;
			}
		}
		};

	auto spawnRandomMinion = [&](int count) {
		for (int i = 0; i < count; i++) {
			int typeCount = (dungeonMap.GetCurrentLevel() >= 2) ? 5 : 4;
			int t = std::rand() % typeCount;
			// 1. 先計算加成值
			int bonus = dungeonMap.GetCurrentLevel() * 2;

			// 2. 將 bonus 加到各怪物的 HP (第 2 參數) 與 Attack (第 3 參數)
			switch (t) {
			case 0:
				enemies.emplace_back("Goblin", 8 + bonus, 5 + bonus, 0, 40.0f, 300.0f, 24.0f, 1.0f, spawnRandom());
				break;
			case 1:
				enemies.emplace_back("Wolf", 12 + bonus, 5 + bonus, 1, 80.0f, 400.0f, 20.0f, 1.2f, spawnRandom());
				enemies.back().SetRangedAttack(true, 260.0f, 0.7f);
				break;
			case 2:
				enemies.emplace_back("Slime", 6 + bonus, 5 + bonus, 0, 30.0f, 250.0f, 18.0f, 0.8f, spawnRandom());
				break;
			case 3:
				enemies.emplace_back("Bat", 5 + bonus, 5 + bonus, 0, 120.0f, 350.0f, 14.0f, 0.6f, spawnRandom());
				enemies.back().SetRangedAttack(true, 330.0f, 0.45f);
				break;
			default:
				enemies.emplace_back("Assassin", 7 + bonus, 6 + bonus, 0, 20.0f, 420.0f, 18.0f, 0.9f, spawnRandom());
				break;
			}
				}
			};

	// 呼叫 spawnRandom() 讓怪物誕生在隨機角落
	enemies.emplace_back("Goblin", 8, 5, 0, 40.0f, 300.0f, 24.0f, 1.0f, spawnRandom());
	enemies.emplace_back("Wolf", 12, 5, 1, 80.0f, 400.0f, 20.0f, 1.2f, spawnRandom());
	enemies.emplace_back("Slime", 6, 3, 0, 30.0f, 250.0f, 18.0f, 0.8f, spawnRandom());
	enemies.emplace_back("Bat", 5, 3, 0, 120.0f, 350.0f, 14.0f, 0.6f, spawnRandom());
	if (dungeonMap.GetCurrentLevel() >= 2) {
		enemies.emplace_back("Assassin", 5, 6, 0, 20.0f, 420.0f, 18.0f, 0.9f, spawnRandom());
	}

	if (enemies.size() > 1) enemies[1].SetRangedAttack(true, 260.0f, 0.7f);
	if (enemies.size() > 3) enemies[3].SetRangedAttack(true, 330.0f, 0.45f);

	// 【修改】初始化關卡層級追蹤，使用 int
	int lastLevel = dungeonMap.GetCurrentLevel();

	// 近戰命中追蹤：每次揮劍每個敵人只會吃到一次傷害
	std::vector<bool> swordHitThisSwing(enemies.size(), false);
	bool wasSwordActive = false;

	// 用於檢測視窗拖曳作弊的變數
	Vector2 lastWinPos = GetWindowPosition();
	int lastMonitor = GetCurrentMonitor();

	// ==========================================
	// 動態能力值與獎勵池系統
	// ==========================================
	int currentBulletDamage = 2; // 原本是常數，現在變成可升級變數
	int currentSwordDamage = 4;
	// 在 currentBulletDamage 附近新增以下結構：
	struct HealthOrb {
		Vector2 pos;
		bool active;
	};
	std::vector<HealthOrb> healthOrbs; // 存放畫面上所有的紅點

	enum class RewardType { MAX_HP, SPEED, ATTACK_RANGE, SWORD_DMG, BULLET_DMG };

	struct RewardOption {
		RewardType type;
		const char* text;
		Color color;
	};

	// 總獎勵池（可以隨便擴充）
	std::vector<RewardOption> rewardPool = {
		{ RewardType::MAX_HP, "Max HP +20 & Heal", RED },
		{ RewardType::SPEED, "Movement Speed +1.5", SKYBLUE },
		{ RewardType::ATTACK_RANGE, "Melee Range +15", ORANGE },
		{ RewardType::SWORD_DMG, "Sword Damage +3", YELLOW },
		{ RewardType::BULLET_DMG, "Bullet Damage +2", PURPLE }
	};

	// 用來存儲當下被隨機抽出來的 3 個選項
	std::vector<RewardOption> currentRewards(3);

	bool isGameStarted = false; // 控制是否在主選單
	bool isPaused = false;      // 控制是否在暫停狀態
	bool designerMode = false;  // 設計師模式：無敵 + 穿牆
	bool bossBarragePrepared = false;
	bool bossSummonResolved = false;
	bool bossObstacleSpawned = false;

	while (!WindowShouldClose()) {
		float dt = GetFrameTime();

		if (isGameStarted && !dungeonMap.IsBossLevel()) currentGameState = GameState::Playing;
		else if (dungeonMap.IsBossLevel())				currentGameState = GameState::Boss;
		else											currentGameState = GameState::Menu;

		// 2. 更新音樂
		UpdateMusicState(currentGameState);

		// 3. 更新音樂串流 (Raylib 必須)
		UpdateMusicStream(AssetManager::GetBgmMenu());
		UpdateMusicStream(AssetManager::GetBgmGameplay());
		UpdateMusicStream(AssetManager::GetBgmBoss());

		// 1. 選單與暫停狀態控制
		if (!isGameStarted) {
			if (IsKeyPressed(KEY_ENTER)) isGameStarted = true;
		}
		else if (!isGameOver) {
			// ESC 暫停/解除暫停，P 切換設計師模式
			if (IsKeyPressed(KEY_ESCAPE)) {
				isPaused = !isPaused;
			}
			if (IsKeyPressed(KEY_P)) {
				designerMode = !designerMode;
				if (designerMode) {
					isPaused = false;
				}
			}
		}

		// 2. 終極時間停止器：將未開始與暫停加入全局凍結判定
		bool isUiPause = showRewardUI || showChestEntryPrompt || isPaused || !isGameStarted;
		if (!isGameOver && !isUiPause && playerInvincibleTimer > 0.0f && !designerMode) {
			playerInvincibleTimer -= dt;
			if (playerInvincibleTimer < 0.0f) playerInvincibleTimer = 0.0f;
		}
		if (player.currentHp <= 0 && !designerMode) {
			isGameOver = true;
		}
		if (isGameOver) {
			if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_Q)) break; // 按 ESC 或 Q 關閉遊戲
			
			BeginDrawing();
			ClearBackground(BLACK);
			if (dungeonMap.IsBossLevel()) {
				DrawText("BOSS DEFEATED - SYSTEM CLEAR", currentWidth / 2 - 170, currentHeight / 2 - 20, 20, GOLD);
				DrawText("Press [ESC] or [Q] to quit", currentWidth / 2 - 120, currentHeight / 2 + 30, 16, WHITE);
			}
			else {
				DrawText("SYSTEM CONNECTION LOST - GAME OVER", currentWidth / 2 - 180, currentHeight / 2 - 20, 20, RED);
				DrawText("Press [ESC] or [Q] to quit", currentWidth / 2 - 120, currentHeight / 2 + 30, 16, WHITE);
			}
			EndDrawing();
			continue;
		}
		Vector2 winPos = GetWindowPosition();

		int monitor = GetCurrentMonitor();
		int maxWidth = GetMonitorWidth(monitor);
		int maxHeight = GetMonitorHeight(monitor);

		if (monitor != prevMonitor) {
			int monLeft = 0, monTop = 0, monW = GetMonitorWidth(monitor), monH = GetMonitorHeight(monitor);
			if (GetMonitorRectForWindow(GetWindowHandle(), monLeft, monTop, monW, monH)) {
			}

			if (currentWidth > monW) currentWidth = monW;
			if (currentHeight > monH) currentHeight = monH;

			int newX = monLeft + (monW - currentWidth) / 2;
			int newY = monTop + (monH - currentHeight) / 2;
			SetWindowSize(currentWidth, currentHeight);
			SetWindowPosition(newX, newY);
			winPos = GetWindowPosition();
			prevMonitor = monitor;
			lastWinPos = winPos;
			lastMonitor = monitor;

			// 【新增】告訴地圖螢幕大小變了！
			dungeonMap.SetMonitorSize(monW, monH);

		}
		// 檢測同一螢幕內的視窗拖曳作弊
		else if (monitor == lastMonitor && (winPos.x != lastWinPos.x || winPos.y != lastWinPos.y)) {
			// 視窗在同一螢幕上被拖曳，恢復到原位置
			SetWindowPosition((int)lastWinPos.x, (int)lastWinPos.y);
			winPos = lastWinPos;
		}

		int monLeft = 0;
		int monTop = 0;
		int monW = maxWidth;
		int monH = maxHeight;
		if (!GetMonitorWorkAreaForWindow(GetWindowHandle(), monLeft, monTop, monW, monH)) {
			GetMonitorRectForWindow(GetWindowHandle(), monLeft, monTop, monW, monH);
		}
		bool gotWork = (monW != maxWidth || monH != maxHeight || monLeft != 0 || monTop != 0);
		if (gotWork) {
			maxWidth = monW;
			maxHeight = monH;
		}
		else {
			monLeft = 0;
			monTop = 0;
		}

		// 依目前血量調整視野範圍：血量越高，視窗越大
		const int minVisionSize = 280;
		const int maxVisionSize = 560;
		float hpRatio = (player.maxHp > 0) ? ((float)player.currentHp / (float)player.maxHp) : 0.0f;
		hpRatio = std::clamp(hpRatio, 0.0f, 1.0f);
		int targetVisionSize = (int)(minVisionSize + (maxVisionSize - minVisionSize) * hpRatio);
		targetVisionSize = std::clamp(targetVisionSize, minVisionSize, maxVisionSize);
		int targetWidth = std::min(targetVisionSize, maxWidth);
		int targetHeight = std::min(targetVisionSize, maxHeight);
		if (targetWidth != currentWidth || targetHeight != currentHeight) {
			currentWidth = targetWidth;
			currentHeight = targetHeight;
			SetWindowSize(currentWidth, currentHeight);
		}

		int relWinX = (int)winPos.x - monLeft;
		int relWinY = (int)winPos.y - monTop;
		if (relWinX < 0) relWinX = 0;
		if (relWinY < 0) relWinY = 0;
		if (relWinX > maxWidth) relWinX = maxWidth;
		if (relWinY > maxHeight) relWinY = maxHeight;

		int rightAvailable = maxWidth - relWinX - currentWidth;
		int downAvailable = maxHeight - relWinY - currentHeight;
		int leftAvailable = relWinX;
		int upAvailable = relWinY - 40;
		if (upAvailable < 0) upAvailable = 0;

		// 關卡切換重置機制
		if (dungeonMap.GetCurrentLevel() != lastLevel) {
			// 每一關都從視窗正中間開始
			float cX = currentWidth * 0.5f;
			float cY = currentHeight * 0.5f;

			// 將視窗置中到顯示器
			int newX = monLeft + (maxWidth - currentWidth) / 2;
			int newY = monTop + (maxHeight - currentHeight) / 2;
			SetWindowPosition(newX, newY);

			playerPos = { cX, cY };
			winPos = GetWindowPosition();

			// 切換到新關卡時重生怪物
			enemies.clear();
			healthOrbs.clear();

			// 找到 if (dungeonMap.GetCurrentLevel() != lastLevel) 區塊內的 Boss 生成程式碼
			if (dungeonMap.GetCurrentLevel() % 5 == 0) {
				// Boss 關卡生成：召喚 Boss
				enemies.emplace_back("GIANT BOSS", 100, 10, 1, 55.0f, 9999.0f, 36.0f, 0.8f, NGJ::Vec2((float)(monW / 2), (float)(monH / 2)));
				enemies.back().ConfigureBossPhaseOne();
				enemies.back().SetPosition(NGJ::Vec2((float)(monW / 2), (float)(monH / 2)));
				dungeonMap.ClearBossObstacles();
			}
			else {
				// 普通關卡怪
				int bonus = dungeonMap.GetCurrentLevel() * 2;

				enemies.emplace_back("Goblin", 8 + bonus, 5 + bonus, 0, 40.0f, 300.0f, 24.0f, 1.0f, spawnRandom());
				enemies.emplace_back("Wolf", 12 + bonus, 5 + bonus, 1, 80.0f, 400.0f, 20.0f, 1.2f, spawnRandom());
				enemies.emplace_back("Slime", 6 + bonus, 5 + bonus, 0, 30.0f, 250.0f, 18.0f, 0.8f, spawnRandom());
				enemies.emplace_back("Bat", 5 + bonus, 5 + bonus, 0, 120.0f, 350.0f, 14.0f, 0.6f, spawnRandom());

				if (dungeonMap.GetCurrentLevel() >= 2) {
					enemies.emplace_back("Assassin", 7 + bonus, 6 + bonus, 0, 20.0f, 420.0f, 18.0f, 0.9f, spawnRandom());
				}
				if (enemies.size() > 1) enemies[1].SetRangedAttack(true, 260.0f, 0.7f);
				if (enemies.size() > 3) enemies[3].SetRangedAttack(true, 330.0f, 0.45f);
			}

			enemySpawnTimer = 0.0f;
			swordHitThisSwing.assign(enemies.size(), false);
			wasSwordActive = false;
			declinedChestIndex = -1;

			// 【新增】：過關時徹底重置 Boss 的技能與召喚狀態，避免下一輪 Boss 戰發呆
			bossBarragePrepared = false;
			bossSummonResolved = false;
			bossObstacleSpawned = false;

			lastLevel = dungeonMap.GetCurrentLevel();
		}

		float centerX = currentWidth * 0.5f;
		float centerY = currentHeight * 0.5f;
		float margin = 5.0f;

		if (inChestRoom && !isUiPause) {
			float roomMinX = 20.0f;
			float roomMinY = 20.0f;
			float roomMaxX = (float)player.currentWinWidth - 20.0f;
			float roomMaxY = (float)player.currentWinHeight - 20.0f;
			if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) playerPos.x += playerSpeed;
			if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) playerPos.x -= playerSpeed;
			if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) playerPos.y += playerSpeed;
			if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) playerPos.y -= playerSpeed;

			if (playerPos.x < roomMinX) playerPos.x = roomMinX;
			if (playerPos.y < roomMinY) playerPos.y = roomMinY;
			if (playerPos.x > roomMaxX - 25) playerPos.x = roomMaxX - 25;
			if (playerPos.y > roomMaxY - 25) playerPos.y = roomMaxY - 25;
		}

		// 保留地圖牆壁碰撞的移動邏輯（以鍵盤控制）
		if (!inChestRoom && !isUiPause) {

			// 【新增】：一個強大的碰撞小幫手，同時檢查牆壁與 Boss 障礙物！
			auto IsBlocked = [&](float x, float y) {
				return dungeonMap.IsWall(x, y) || dungeonMap.IsBossObstacleAt(x, y);
				};

			// 右移
			if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
				float targetPlayerX = playerPos.x;
				int targetWinX = (int)winPos.x;

				if (playerPos.x < centerX) {
					targetPlayerX += playerSpeed;
					if (targetPlayerX > centerX) targetPlayerX = centerX;
				}
				else {
					int winRightDesk = (int)winPos.x + currentWidth;
					int screenRight = monLeft + monW;
					int availableRight = screenRight - winRightDesk;
					if (availableRight > 0) {
						int delta = (availableRight >= (int)playerSpeed) ? (int)playerSpeed : availableRight;
						targetWinX += delta;
						targetPlayerX = centerX;
					}
					else {
						targetPlayerX += playerSpeed;
						if (targetPlayerX > (float)(currentWidth - 25)) targetPlayerX = (float)(currentWidth - 25);
					}
				}

				float potentialMapX = (targetWinX + targetPlayerX) - monLeft;
				float currentMapY = (winPos.y + playerPos.y) - monTop;

				// 【修改】：使用 IsBlocked 替代 IsWall
				if (designerMode || (!IsBlocked(potentialMapX + playerRadius, currentMapY) &&
					!IsBlocked(potentialMapX - playerRadius, currentMapY))) {
					playerPos.x = targetPlayerX;
					if ((int)winPos.x != targetWinX) {
						SetWindowPosition(targetWinX, (int)winPos.y);
						winPos.x = (float)targetWinX;
					}
				}
			}

			// 左移
			if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
				float targetPlayerX = playerPos.x;
				int targetWinX = (int)winPos.x;

				if (playerPos.x > centerX) {
					targetPlayerX -= playerSpeed;
					if (targetPlayerX < centerX) targetPlayerX = centerX;
				}
				else {
					int winLeftDesk = (int)winPos.x;
					int screenLeft = monLeft;
					int availableLeft = winLeftDesk - screenLeft;
					if (availableLeft > 0) {
						int delta = (availableLeft >= (int)playerSpeed) ? (int)playerSpeed : availableLeft;
						targetWinX -= delta;
						targetPlayerX = centerX;
					}
					else {
						targetPlayerX -= playerSpeed;
						if (targetPlayerX < margin) targetPlayerX = margin;
					}
				}

				float potentialMapX = (targetWinX + targetPlayerX) - monLeft;
				float currentMapY = (winPos.y + playerPos.y) - monTop;

				// 【修改】：使用 IsBlocked 替代 IsWall
				if (designerMode || (!IsBlocked(potentialMapX - playerRadius, currentMapY) &&
					!IsBlocked(potentialMapX + playerRadius, currentMapY))) {
					playerPos.x = targetPlayerX;
					if ((int)winPos.x != targetWinX) {
						SetWindowPosition(targetWinX, (int)winPos.y);
						winPos.x = (float)targetWinX;
					}
				}
			}

			// 下移
			if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
				float targetPlayerY = playerPos.y;
				int targetWinY = (int)winPos.y;

				if (playerPos.y < centerY) {
					targetPlayerY += playerSpeed;
					if (targetPlayerY > centerY) targetPlayerY = centerY;
				}
				else {
					int winBottomDesk = (int)winPos.y + currentHeight;
					int screenBottom = monTop + monH;
					int availableDown = screenBottom - winBottomDesk;
					if (availableDown > 0) {
						int delta = (availableDown >= (int)playerSpeed) ? (int)playerSpeed : availableDown;
						targetWinY += delta;
						targetPlayerY = centerY;
					}
					else {
						targetPlayerY += playerSpeed;
						if (targetPlayerY > (float)(currentHeight - 25)) targetPlayerY = (float)(currentHeight - 25);
					}
				}

				float currentMapX = (winPos.x + playerPos.x) - monLeft;
				float potentialMapY = (targetWinY + targetPlayerY) - monTop;

				// 【修改】：使用 IsBlocked 替代 IsWall
				if (designerMode || (!IsBlocked(currentMapX, potentialMapY + playerRadius) &&
					!IsBlocked(currentMapX, potentialMapY - playerRadius))) {
					playerPos.y = targetPlayerY;
					if ((int)winPos.y != targetWinY) {
						SetWindowPosition((int)winPos.x, targetWinY);
						winPos.y = (float)targetWinY;
					}
				}
			}

			// 上移
			if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
				float targetPlayerY = playerPos.y;
				int targetWinY = (int)winPos.y;

				if (playerPos.y > centerY) {
					targetPlayerY -= playerSpeed;
					if (targetPlayerY < centerY) targetPlayerY = centerY;
				}
				else {
					int winTopDesk = (int)winPos.y;
					int screenTop = monTop;
					int availableUp = winTopDesk - screenTop;
					if (availableUp > 0) {
						int delta = (availableUp >= (int)playerSpeed) ? (int)playerSpeed : availableUp;
						targetWinY -= delta;
						targetPlayerY = centerX;
					}
					else {
						targetPlayerY -= playerSpeed;
						if (targetPlayerY < margin) targetPlayerY = margin;
					}
				}

				float currentMapX = (winPos.x + playerPos.x) - monLeft;
				float potentialMapY = (targetWinY + targetPlayerY) - monTop;

				// 【修改】：使用 IsBlocked 替代 IsWall
				if (designerMode || (!IsBlocked(currentMapX, potentialMapY - playerRadius) &&
					!IsBlocked(currentMapX, potentialMapY + playerRadius))) {
					playerPos.y = targetPlayerY;
					if ((int)winPos.y != targetWinY) {
						SetWindowPosition((int)winPos.x, targetWinY);
						winPos.y = (float)targetWinY;
					}
				}
			}
		}

		// 更新 lastWinPos，以便下一幀進行拖曳檢測
		lastWinPos = winPos;

		Vector2 playerMapPos = inChestRoom
			? Vector2{ playerPos.x, playerPos.y }
			: Vector2{ (winPos.x + playerPos.x) - monLeft, (winPos.y + playerPos.y) - monTop };
		auto localToCombatWorld = [&](const Vector2& localPos) {
			return inChestRoom
				? Vector2{ localPos.x, localPos.y }
				: Vector2{ (winPos.x + localPos.x) - monLeft, (winPos.y + localPos.y) - monTop };
		};
		if (!inChestRoom) {
			dungeonMap.Update(playerMapPos);
		}

		// 寶箱碰撞檢測
		const auto& chestPositions = dungeonMap.GetChestPositions();
		const auto& chestOpened = dungeonMap.GetChestOpened();

		if (!inChestRoom && !showChestEntryPrompt) {
			// 【修正】：如果目前有拒絕紀錄，檢查玩家是否已經走開了
			if (declinedChestIndex != -1 && declinedChestIndex < (int)chestPositions.size()) {
				float distToDeclined = sqrtf(powf(playerMapPos.x - chestPositions[declinedChestIndex].x, 2) +
					powf(playerMapPos.y - chestPositions[declinedChestIndex].y, 2));
				// 只要玩家和該寶箱距離大於 50 像素，就當作玩家走開了，清除拒絕紀錄，允許重新靠近觸發
				if (distToDeclined > 50.0f) {
					declinedChestIndex = -1;
				}
			}

			for (int i = 0; i < (int)chestPositions.size(); i++) {
				if (!chestOpened[i]) {
					if (i == declinedChestIndex) continue; // 如果還在拒絕範圍內，先跳過
					float distToChest = sqrtf(powf(playerMapPos.x - chestPositions[i].x, 2) +
						powf(playerMapPos.y - chestPositions[i].y, 2));
					if (distToChest < 30.0f) {
						pendingChestIndex = i;
						showChestEntryPrompt = true;
						break;
					}
				}
			}
		}

		if (showChestEntryPrompt) {
			if (IsKeyPressed(KEY_Y)) {
				currentChestIndex = pendingChestIndex;
				pendingChestIndex = -1;
				showChestEntryPrompt = false;
				declinedChestIndex = -1;
				inChestRoom = true;
				returnWinPos = winPos;
				returnPlayerPos = playerPos;
				playerPos = { currentWidth * 0.5f, currentHeight * 0.75f };
				winPos = { (float)monLeft, (float)monTop };
				SetWindowPosition(monLeft, monTop);
				spawnChestRoomEnemies();
				swordHitThisSwing.assign(chestRoomEnemies.size(), false);
				wasSwordActive = false;
			}
			else if (IsKeyPressed(KEY_N)) {
				declinedChestIndex = pendingChestIndex;
				pendingChestIndex = -1;
				showChestEntryPrompt = false;
			}
		}

		// 寶箱房怪物全滅後才開啟獎勵 UI
		if (inChestRoom && !showRewardUI && currentChestIndex >= 0) {
			bool allDead = !chestRoomEnemies.empty();
			for (const auto& ce : chestRoomEnemies) {
				if (!ce.GetIsDead()) {
					allDead = false;
					break;
				}
			}
			if (allDead) {
				showRewardUI = true;
				rewardUITimer = 0.0;
				selectedReward = -1;

				// 【新增】：隨機打亂獎勵池，抽出前 3 個不重複的獎勵
				std::vector<RewardOption> poolCopy = rewardPool;
				for (int i = 0; i < poolCopy.size(); i++) {
					int swapIdx = std::rand() % poolCopy.size();
					std::swap(poolCopy[i], poolCopy[swapIdx]);
				}
				for (int i = 0; i < 3; i++) {
					currentRewards[i] = poolCopy[i];
				}
			}
		}

		// 獎勵 UI 交互
		if (showRewardUI && currentChestIndex >= 0) {
			rewardUITimer += dt;

			if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) selectedReward = 0;
			else if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) selectedReward = 1;
			else if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) selectedReward = 2;

			// 如果選擇了獎勵，套用並關閉 UI
			if (selectedReward >= 0) {
				RewardType chosenType = currentRewards[selectedReward].type;

				int levelBonus = (int)(dungeonMap.GetCurrentLevel() * 1.35f);

				switch (chosenType) {
				case RewardType::MAX_HP:
					player.maxHp += (20 + levelBonus); // 血量上限加成
					player.currentHp = player.maxHp;
					break;
				case RewardType::SPEED:
					playerSpeed += (1.5f + (levelBonus * 0.1f)); // 速度加成
					break;
				case RewardType::ATTACK_RANGE:
					player.attackRange += (15.0f + levelBonus);
					break;
				case RewardType::SWORD_DMG:
					currentSwordDamage += (3 + levelBonus);
					break;
				case RewardType::BULLET_DMG:
					currentBulletDamage += (2 + levelBonus);
					break;
				}

				// 標記寶箱已打開並重置狀態
				dungeonMap.OpenChest(currentChestIndex);
				showRewardUI = false;
				inChestRoom = false;
				currentChestIndex = -1;
				selectedReward = -1;
				chestRoomEnemies.clear();
				SetWindowPosition((int)returnWinPos.x, (int)returnWinPos.y);
				winPos = returnWinPos;
				playerPos = returnPlayerPos;
				swordHitThisSwing.assign(enemies.size(), false);
				wasSwordActive = false;
			}
		}

		// 同步位置給 Player，再處理攻擊輸入與更新（僅處理攻擊，不再次處理移動）
		player.playerPos = playerPos;
		player.currentWinPos = winPos;
		player.currentWinWidth = currentWidth;
		player.currentWinHeight = currentHeight;
		if (!isUiPause) {
			player.ProcessCombatInput();
			player.UpdateCombat(dt);

			// 【新增】：檢查玩家子彈是否撞到迷宮牆壁
			if (!inChestRoom) {
				for (auto& b : player.bullets) {
					if (!b.active) continue;

					// 將子彈的視窗相對座標轉為大世界座標
					Vector2 bulletWorld = localToCombatWorld(b.pos);

					// 如果大世界座標對應的是牆壁，子彈立刻失效
					if (dungeonMap.IsWall(bulletWorld.x, bulletWorld.y)) {
						b.active = false;
					}
				}
			}
		}
		// ==========================================
		// 🔻 第三步貼在這裡：偵測玩家是否踩到補血紅點 🔻
		for (auto& orb : healthOrbs) {
			if (!orb.active) continue;
			float dx = playerMapPos.x - orb.pos.x;
			float dy = playerMapPos.y - orb.pos.y;
			// 如果玩家碰到紅點 (距離小於玩家半徑+紅點半徑)
			if (sqrtf(dx * dx + dy * dy) <= playerRadius + 8.0f) {
				orb.active = false;

				// 【修改】動態計算回血量：基礎值 5 + (關卡層數 * 1.5)
				int healAmount = 5 + (int)(dungeonMap.GetCurrentLevel() * 1.5f);
				player.currentHp += healAmount;

				if (player.currentHp > player.maxHp) {
					player.currentHp = player.maxHp;
				}
			}
		}
		// 🔺 第三步結束 🔺
		// ==========================================


		// 🔍 你搜尋的目標在這裡：
		// 更新敵人狀態（使用世界座標的 player 位置）
		NGJ::Vec2 playerWorldPos(playerMapPos.x, playerMapPos.y);
		NGJ::Vec2 viewMin((float)(winPos.x - monLeft), (float)(winPos.y - monTop));
		NGJ::Vec2 viewMax(viewMin.x + (float)player.currentWinWidth, viewMin.y + (float)player.currentWinHeight);
		auto& activeEnemies = inChestRoom ? chestRoomEnemies : enemies;
		NGJ::Enemy* bossEnemy = nullptr;
		if (!inChestRoom) {
			for (auto& e : activeEnemies) {
				if (e.IsBoss()) {
					bossEnemy = &e;
					break;
				}
			}
		}
		if (!isUiPause) {
			for (auto& e : activeEnemies) {
				Map* combatMap = inChestRoom ? nullptr : &dungeonMap;
				e.Update(dt, playerWorldPos, combatMap, &viewMin, &viewMax); // <--- 把地圖與可視範圍傳進去
				if (inChestRoom) {
					NGJ::Vec2 p = e.GetPosition();
					if (p.x < 34.0f) p.x = 34.0f;
					if (p.y < 34.0f) p.y = 34.0f;
					if (p.x > (float)currentWidth - 34.0f) p.x = (float)currentWidth - 34.0f;
					if (p.y > (float)currentHeight - 34.0f) p.y = (float)currentHeight - 34.0f;
					e.SetPosition(p);
				}
			}
		}

		if (bossEnemy && !isUiPause && !bossSummonResolved && bossEnemy->GetCurrentHP() <= 75) {
			bossEnemy->TriggerBossSummon();
			bossEnemy->TriggerBossIdleLock(1.5f);
			spawnRandomMinion(3);
			swordHitThisSwing.assign(enemies.size(), false);
			bossSummonResolved = true;
			// 【新增這段】重新掃描陣列，把 bossEnemy 指標綁定到搬家後的新位置！
			bossEnemy = nullptr;
			for (auto& e : enemies) {
				if (e.IsBoss()) {
					bossEnemy = &e;
					break;
				}
			}
		}

		if (bossEnemy && !isUiPause) {
			static float bossEventTimer = 0.0f;
			if (!bossEnemy->IsBossBulletBarrageActive() && !bossEnemy->IsBossIdleLockActive()) {
				bossEventTimer += dt;
				if (bossEventTimer >= 10.0f) {
					bossEventTimer = 0.0f;
					if ((std::rand() % 100) < 50) {
						bossEnemy->SetPosition(NGJ::Vec2((float)(monW / 2), (float)(monH / 2)));
						bossEnemy->TriggerBossBarrage();
						dungeonMap.SpawnBossObstacles(4);
						bossObstacleSpawned = true;
						bossBarragePrepared = true;
					}
				}
			}

			// 清除障礙物邏輯
			if (bossBarragePrepared && !bossEnemy->IsBossBulletBarrageActive() && !bossEnemy->IsBossIdleLockActive()) {
				dungeonMap.ClearBossObstacles();
				bossObstacleSpawned = false;
				bossBarragePrepared = false;
			}

			// Boss 死亡後的傳送門邏輯
			if (dungeonMap.IsBossLevel() && bossEnemy->GetIsDead()) {
				dungeonMap.ActivateBossExitDoor();

				float doorWorldX = initialMonW / 2.0f; // 假設門在螢幕正中央
				float doorWorldY = initialMonH / 2.0f;

				// 計算玩家距離 (注意：playerMapPos 是你在 main.cpp 前面算好的變數)
				float distToDoor = sqrtf(powf(playerMapPos.x - doorWorldX, 2) + powf(playerMapPos.y - doorWorldY, 2));

				if (distToDoor < 25.0f) {
					dungeonMap.AdvanceLevel();
					bossSummonResolved = false; // 重置狀態以便下一關
				}
			}
		}

		// 每 7 秒新增一隻隨機怪物（僅一般地圖，Boss關卡不刷）
		if (!inChestRoom && !isUiPause && !dungeonMap.IsBossLevel()) {
			enemySpawnTimer += dt;
			if (enemySpawnTimer >= enemySpawnInterval) {
				enemySpawnTimer -= enemySpawnInterval;
				int typeCount = (dungeonMap.GetCurrentLevel() >= 2) ? 5 : 4;
				int t = std::rand() % typeCount;
				// 1. 先計算加成值
				int bonus = dungeonMap.GetCurrentLevel() * 2;

				// 2. 將 bonus 加到各怪物的 HP (第 2 參數) 與 Attack (第 3 參數)
				switch (t) {
				case 0:
					enemies.emplace_back("Goblin", 8 + bonus, 5 + bonus, 0, 40.0f, 300.0f, 24.0f, 1.0f, spawnRandom());
					break;
				case 1:
					enemies.emplace_back("Wolf", 12 + bonus, 5 + bonus, 1, 80.0f, 400.0f, 20.0f, 1.2f, spawnRandom());
					enemies.back().SetRangedAttack(true, 260.0f, 0.7f);
					break;
				case 2:
					enemies.emplace_back("Slime", 6 + bonus, 5 + bonus, 0, 30.0f, 250.0f, 18.0f, 0.8f, spawnRandom());
					break;
				case 3:
					enemies.emplace_back("Bat", 5 + bonus, 5 + bonus, 0, 120.0f, 350.0f, 14.0f, 0.6f, spawnRandom());
					enemies.back().SetRangedAttack(true, 330.0f, 0.45f);
					break;
				default:
					enemies.emplace_back("Assassin", 7 + bonus, 6 + bonus, 0, 20.0f, 420.0f, 18.0f, 0.9f, spawnRandom());
					break;
				}
				swordHitThisSwing.push_back(false);
			}
		}

		// 玩家攻擊命中判定：子彈與近戰揮砍命中敵人時扣血
		//const int bulletDamage = 2;
		//const int swordDamage = 4;
		const float enemyHitRadius = 12.0f;

		if (!player.sword.active && wasSwordActive) {
			std::fill(swordHitThisSwing.begin(), swordHitThisSwing.end(), false);
		}
		if (player.sword.active && !wasSwordActive) {
			std::fill(swordHitThisSwing.begin(), swordHitThisSwing.end(), false);
		}
		wasSwordActive = player.sword.active;

		for (size_t ei = 0; ei < activeEnemies.size(); ++ei) {
			auto& e = activeEnemies[ei];
			if (e.GetIsDead() || isUiPause) continue;

			Vector2 enemyPos = { e.GetPosition().x, e.GetPosition().y };

			if (!designerMode && playerInvincibleTimer <= 0.0f) {
				float pdx = playerMapPos.x - enemyPos.x;
				float pdy = playerMapPos.y - enemyPos.y;
				float playerEnemyDist = sqrtf(pdx * pdx + pdy * pdy);
				if (e.GetState() == NGJ::EnemyState::Attack && e.CanAttack() &&
					playerEnemyDist <= playerRadius + enemyHitRadius + 4.0f) {
					int dmg = e.Attack();
					if (dmg > 0) {
						player.currentHp -= dmg;
						if (player.currentHp < 0) player.currentHp = 0;
						playerInvincibleTimer = 0.5f;
					}
				}
			}

			// 子彈命中：命中後子彈失效並扣敵人血量（Goblin 盾牌可擋子彈）
			for (auto& b : player.bullets) {
				if (!b.active) continue;
				Vector2 bulletWorld = localToCombatWorld(b.pos);

				// Goblin 盾牌：朝向玩家，僅擋子彈，不擋近戰
				if (e.name == "Goblin") {
					float fx = playerMapPos.x - enemyPos.x;
					float fy = playerMapPos.y - enemyPos.y;
					float fl = sqrtf(fx * fx + fy * fy);
					if (fl > 0.001f) {
						fx /= fl;
						fy /= fl;
						float px = -fy;
						float py = fx;
						float cx = enemyPos.x + fx * 14.0f;
						float cy = enemyPos.y + fy * 14.0f;
						float halfLen = 14.0f;
						float x1 = cx - px * halfLen;
						float y1 = cy - py * halfLen;
						float x2 = cx + px * halfLen;
						float y2 = cy + py * halfLen;

						float vx = x2 - x1;
						float vy = y2 - y1;
						float wx = bulletWorld.x - x1;
						float wy = bulletWorld.y - y1;
						float vv = vx * vx + vy * vy;
						float t = (vv > 0.0f) ? ((wx * vx + wy * vy) / vv) : 0.0f;
						if (t < 0.0f) t = 0.0f;
						if (t > 1.0f) t = 1.0f;
						float qx = x1 + vx * t;
						float qy = y1 + vy * t;
						float dxs = bulletWorld.x - qx;
						float dys = bulletWorld.y - qy;
						float shieldDist = sqrtf(dxs * dxs + dys * dys);
						float frontDot = (bulletWorld.x - enemyPos.x) * fx + (bulletWorld.y - enemyPos.y) * fy;
						if (frontDot > 0.0f && shieldDist <= 5.0f) {
							b.active = false;
							break;
						}
					}
				}

				float bdx = bulletWorld.x - enemyPos.x;
				float bdy = bulletWorld.y - enemyPos.y;
				float d = sqrtf(bdx * bdx + bdy * bdy);
				if (d <= enemyHitRadius + 4.0f) {
					bool wasDead = e.GetIsDead();
					e.TakeDamage(currentBulletDamage);
					if (!wasDead && e.GetIsDead()) { // 如果是這發子彈剛好打死牠
						if (std::rand() % 100 < 30) { // 30% 機率掉落紅點
							healthOrbs.push_back({ enemyPos, true });
						}
					}
					b.active = false;
					break;
				}
			}

			if (e.GetIsDead()) continue;

			if (!designerMode && playerInvincibleTimer <= 0.0f) {
				for (auto& eb : e.GetBulletsMutable()) {
					if (!eb.active) continue;
					float pdx = playerMapPos.x - eb.position.x;
					float pdy = playerMapPos.y - eb.position.y;
					float d = sqrtf(pdx * pdx + pdy * pdy);
					if (d <= playerRadius + 4.0f) {
						eb.active = false;
						player.currentHp -= 3;
						if (player.currentHp < 0) player.currentHp = 0;
						playerInvincibleTimer = 0.5f;
						break;
					}
				}
			}

			// 近戰命中：揮劍期間每個敵人只吃一次傷害
			if (player.sword.active && !swordHitThisSwing[ei]) {
				Vector2 swordCenterWorld = localToCombatWorld(player.sword.center);
				float sdx = swordCenterWorld.x - enemyPos.x;
				float sdy = swordCenterWorld.y - enemyPos.y;
				float d = sqrtf(sdx * sdx + sdy * sdy);
				if (d <= player.attackRange + enemyHitRadius) {
					bool wasDead = e.GetIsDead();
					e.TakeDamage(currentSwordDamage);
					swordHitThisSwing[ei] = true;
					if (!wasDead && e.GetIsDead()) { // 如果是這刀剛好砍死牠
						if (std::rand() % 100 < 30) { // 30% 機率掉落紅點
							healthOrbs.push_back({ enemyPos, true });
						}
					}
					swordHitThisSwing[ei] = true;
				}
			}
		}

		camera.target = playerMapPos;
		camera.offset = playerPos;

		BeginDrawing();
		ClearBackground(BLACK);

		
			BeginMode2D(camera);
			if (!inChestRoom) {
				dungeonMap.DrawBaseMap();
				dungeonMap.DrawObjects();
			}
			else {
				DrawRectangle(20, 20, currentWidth - 40, currentHeight - 40, DARKGRAY);
				DrawRectangleLines(20, 20, currentWidth - 40, currentHeight - 40, BLACK);
			}
			// 在 world-space 畫出子彈與近戰特效（將 window-local 轉為 world：world = winPos + local - monLeft/top）
			for (const auto& b : player.bullets) {
				if (b.active) {
					Vector2 worldB = localToCombatWorld(b.pos);
					DrawCircleV(worldB, 4.0f, YELLOW);
				}
			}
			if (player.sword.active) {
				Vector2 worldSwordCenter = localToCombatWorld(player.sword.center);
				Rectangle swordRect = { worldSwordCenter.x, worldSwordCenter.y, 55.0f, 5.0f };
				Vector2 origin = { 0.0f, 2.5f };
				DrawRectanglePro(swordRect, origin, player.sword.currentAngle, RAYWHITE);
			}

			// Draw enemy bullets in world-space
			const auto& drawEnemies = inChestRoom ? chestRoomEnemies : enemies;
			for (const auto& e : drawEnemies) {
				for (const auto& b : e.GetBullets()) {
					if (!b.active) continue;
					DrawCircle((int)b.position.x, (int)b.position.y, 4, MAGENTA);
				}
			}

			// Draw enemies in world-space (全面改用 4 格動畫貼圖)
			for (const auto& e : drawEnemies) {
				if (e.GetIsDead()) continue;

				Vector2 wp = { e.GetPosition().x, e.GetPosition().y };

				// 1. 根據怪物種類，動態指派對應的貼圖、格數與縮放
				Texture2D tex;
				int enemyFrames = 4;   // 同主角一樣，怪物全體皆為 4 格動態圖
				float scale = 1.0f;    // 預設一倍大
				float animSpeed = 0.1f; // 影格切換速度

				if (e.name == "Bat") tex = AssetManager::GetBatTexture();
				else if (e.name == "Wolf") tex = AssetManager::GetWolfTexture();
				else if (e.name == "Slime") tex = AssetManager::GetSlimeTexture();
				else if (e.name == "Goblin") tex = AssetManager::GetGoblinTexture();
				else if (e.name == "Assassin") tex = AssetManager::GetAssassinTexture();
				else if (e.name == "GIANT BOSS") {
					tex = AssetManager::GetBossTexture();
					scale = 2.0f;       // Boss 放大兩倍，維持 4 格動畫
				}
				else {
					tex = AssetManager::GetGoblinTexture(); // 防呆預設
				}

				// 2. 呼叫動畫繪製函式

				// 1. 計算怪物面向：Goblin 與 Wolf 才會旋轉面向玩家
				float rotationAngle = 0.0f; // 預設值
				if (e.name == "Goblin" || e.name == "Wolf") {
					// 只有在追擊或攻擊狀態時，才讓這些怪物看向玩家
					if (e.GetState() == NGJ::EnemyState::Chase || e.GetState() == NGJ::EnemyState::Attack) {
						float dirX = playerMapPos.x - wp.x;
						float dirY = playerMapPos.y - wp.y;
						rotationAngle = atan2f(dirY, dirX) * (180.0f / 3.14159f);
					}
				}

				// 2. 呼叫繪製函式
				int frameWidth = tex.width / enemyFrames;
				Rectangle srcRec = { 0.0f, 0.0f, (float)frameWidth, (float)tex.height };
				Rectangle destRec = { wp.x, wp.y, (float)frameWidth * scale, (float)tex.height * scale };
				Vector2 origin = { (float)frameWidth * scale / 2.0f, (float)tex.height * scale / 2.0f };

				// 如果是 Goblin 或 Wolf，使用算出的角度；否則使用 0 度（不旋轉）
				float finalRotation = (e.name == "Goblin" || e.name == "Wolf") ? (rotationAngle + 270.0f) : 0.0f;

				DrawTexturePro(tex, srcRec, destRec, origin, finalRotation, WHITE);

				// Goblin 盾牌防線線條（保持繪製在怪物前方）
				if (e.name == "Goblin") {
					float fx = playerMapPos.x - wp.x;
					float fy = playerMapPos.y - wp.y;
					float fl = sqrtf(fx * fx + fy * fy);
					if (fl > 0.001f) {
						fx /= fl;
						fy /= fl;
						float px = -fy;
						float py = fx;
						float cx = wp.x + fx * 14.0f;
						float cy = wp.y + fy * 14.0f;
						float halfLen = 14.0f;
						Vector2 s1 = { cx - px * halfLen, cy - py * halfLen };
						Vector2 s2 = { cx + px * halfLen, cy + py * halfLen };
						DrawLineEx(s1, s2, 3.0f, SKYBLUE);
					}
				}

				// 血條繪製
				int barW = 30;
				int hpW = (int)((float)e.GetCurrentHP() / (float)e.GetMaxHP() * barW);

				// 【關鍵修正】：根據 scale 動態上移血條位置
				// 若 scale 為 2.0 (Boss)，我們往上偏移更多 (例如 -45)，若是一般怪 (-20)
				int yOffset = (scale > 1.5f) ? 45 : 20;

				DrawRectangle((int)wp.x - barW / 2, (int)wp.y - yOffset, barW, 5, DARKGRAY);
				DrawRectangle((int)wp.x - barW / 2, (int)wp.y - yOffset, hpW, 5, RED);

				// 名字也同步上移，避免擋住頭部
				DrawText(e.name.c_str(), (int)wp.x - 16, (int)wp.y - yOffset - 15, 10, WHITE);
				// ==========================================
			// 🔻 第五步貼在這裡：畫出掉落在地上的補血紅點 🔻
				for (const auto& orb : healthOrbs) {
					if (orb.active) {
						DrawCircleV(orb.pos, 6.0f, RED); // 實心紅點
						DrawCircleLines((int)orb.pos.x, (int)orb.pos.y, 9, PINK); // 外圍加上粉紅色光圈增加辨識度
					}
				}
				// 🔺 第五步結束 🔺
				// ==========================================
			}

			EndMode2D();

			// Draw player at window-local for UI
			player.playerPos = playerPos;
			AssetManager::DrawPlayerAnimated(playerPos, WHITE);
			if (inChestRoom) {
				DrawText("REWARD ROOM", 12, currentHeight - 22, 14, GOLD);
			}

			DrawRectangle(8, 8, 260, 68, Fade(BLACK, 0.7f));
			const char* levelName = dungeonMap.IsBossLevel() ?
				TextFormat("LAYER %d [BOSS ARENA]", dungeonMap.GetCurrentLevel()) :
				TextFormat("LAYER %d", dungeonMap.GetCurrentLevel());
			DrawText(levelName, 14, 12, 14, dungeonMap.IsBossLevel() ? RED : ORANGE);
			DrawText(TextFormat("Key Status: %d/%d", dungeonMap.GetKeysCollected(), dungeonMap.GetTotalKeys()), 14, 32, 14, dungeonMap.IsDoorUnlocked() ? GREEN : RED);
			DrawText(TextFormat("HP: %d/%d", player.currentHp, player.maxHp), 14, 50, 14, player.currentHp <= 1 ? RED : WHITE);

			// 調試信息：顯示地圖大小和鑰匙數量
			DrawText(TextFormat("Map Size: %dx%d tiles", dungeonMap.GetTotalWidth() / 50, dungeonMap.GetTotalHeight() / 50), 10, 100, 12, YELLOW);
			DrawText(TextFormat("Keys Found: %d", dungeonMap.GetKeysCollected()), 10, 115, 12, YELLOW);

			

			// Debug: 顯示怪物/玩家世界座標與轉換到螢幕位置，幫助定位為何看不到怪物
			if (!drawEnemies.empty()) {
				const auto& e0 = drawEnemies[0];
				Vector2 eWorld = { e0.GetPosition().x, e0.GetPosition().y };
				Vector2 eScreen = GetWorldToScreen2D(eWorld, camera);
				DrawCircleV(eScreen, 6.0f, RED);
				DrawText(TextFormat("Enemies: %d", (int)drawEnemies.size()), 10, 10, 14, WHITE);
				DrawText(TextFormat("E0 world: %.0f, %.0f", eWorld.x, eWorld.y), 10, 28, 12, WHITE);
				DrawText(TextFormat("E0 screen: %.0f, %.0f", eScreen.x, eScreen.y), 10, 42, 12, WHITE);
			}
			// 顯示玩家的 world->screen
			Vector2 pWorld = { playerMapPos.x, playerMapPos.y };
			Vector2 pScreen = GetWorldToScreen2D(pWorld, camera);
			DrawCircleV(pScreen, 5.0f, BLUE);
			DrawText(TextFormat("Player world: %.0f, %.0f", pWorld.x, pWorld.y), 10, 58, 12, WHITE);
			DrawText(TextFormat("Player screen: %.0f, %.0f", pScreen.x, pScreen.y), 10, 74, 12, WHITE);

			// Draw player at window-local for UI
			player.playerPos = playerPos;

			AssetManager::DrawPlayerAnimated(playerPos, WHITE);
			if (inChestRoom) {
				DrawText("REWARD ROOM", 12, currentHeight - 22, 14, GOLD);
			}



			// 獎勵選擇 UI
			if (showRewardUI && currentChestIndex >= 0) {
				// 背景半透明黑色覆蓋
				DrawRectangle(0, 0, currentWidth, currentHeight, Fade(BLACK, 0.5f));

				// 獎勵選擇框
				int uiWidth = 300;
				int uiHeight = 200;
				int uiX = (currentWidth - uiWidth) / 2;
				int uiY = (currentHeight - uiHeight) / 2;

				DrawRectangle(uiX, uiY, uiWidth, uiHeight, DARKGRAY);
				DrawRectangleLines(uiX, uiY, uiWidth, uiHeight, WHITE);

				// 標題
				DrawText("Chest Reward - Choose One:", uiX + 20, uiY + 15, 16, WHITE);


				// 三個隨機抽出的獎勵選項
				for (int i = 0; i < 3; i++) {
					int optionY = uiY + 50 + i * 40;
					Color optColor = (selectedReward == i) ? YELLOW : currentRewards[i].color;
					DrawRectangle(uiX + 20, optionY, uiWidth - 40, 35, Fade(BLACK, 0.3f));
					DrawRectangleLines(uiX + 20, optionY, uiWidth - 40, 35, optColor);

					// 顯示選項數字與動態獎勵文字
					DrawText(TextFormat("[%d] %s", i + 1, currentRewards[i].text), uiX + 35, optionY + 8, 14, optColor);
				}

				DrawText("Press [1], [2], or [3] to confirm.", uiX + 20, uiY + 180, 12, GRAY);
			}
		
		// ==========================================
			// 寶箱進入確認彈出視窗 UI
			// ==========================================
		if (showChestEntryPrompt) {
			// 1. 畫一層半透明的黑幕蓋住背景，讓注意力集中在視窗上
			DrawRectangle(0, 0, currentWidth, currentHeight, Fade(BLACK, 0.6f));

			// 2. 計算對話框大小與置中位置
			int promptWidth = 280;
			int promptHeight = 120;
			int promptX = (currentWidth - promptWidth) / 2;
			int promptY = (currentHeight - promptHeight) / 2;

			// 3. 畫出對話框底色與邊框 (科技感的深藍+亮藍邊)
			DrawRectangle(promptX, promptY, promptWidth, promptHeight, DARKBLUE);
			DrawRectangleLines(promptX, promptY, promptWidth, promptHeight, SKYBLUE);

			// 4. 顯示提示文字
			DrawText("MYSTERIOUS SIGNAL DETECTED", promptX + 15, promptY + 20, 16, GOLD);
			DrawText("Enter Reward Room?", promptX + 60, promptY + 50, 16, WHITE);

			// 5. 畫出 Yes / No 選項提示
			DrawRectangle(promptX + 30, promptY + 80, 80, 25, Fade(GREEN, 0.3f));
			DrawRectangleLines(promptX + 30, promptY + 80, 80, 25, GREEN);
			DrawText("[Y] YES", promptX + 45, promptY + 85, 14, GREEN);

			DrawRectangle(promptX + 170, promptY + 80, 80, 25, Fade(RED, 0.3f));
			DrawRectangleLines(promptX + 170, promptY + 80, 80, 25, RED);
			DrawText("[N] NO", promptX + 185, promptY + 85, 14, RED);
		}
		// ==========================================
		// 主選單與暫停 UI 覆蓋層
		// ==========================================
		if (!isGameStarted) {
			// 1. 繪製背景圖 (取代原先的 ClearBackground)
			// 使用 DrawTexturePro 可以讓圖片自動縮放填滿目前視窗
			Texture2D menuTex = AssetManager::GetMenuBackground();
			Rectangle srcRec = { 0.0f, 0.0f, (float)menuTex.width, (float)menuTex.height };
			Rectangle destRec = { 0.0f, 0.0f, (float)currentWidth, (float)currentHeight };
			DrawTexturePro(menuTex, srcRec, destRec, { 0, 0 }, 0.0f, WHITE);

			// 2. 疊加一層淡淡的遮罩，確保文字清晰可見
			DrawRectangle(0, 0, currentWidth, currentHeight, Fade(BLACK, 0.6f));

			// 3. 繪製標題與說明文字
			DrawText("NGJ 2026: ABSOLUTE EXPANSION", currentWidth / 2 - 160, currentHeight / 2 - 80, 20, GREEN);
			DrawText("Press [ENTER] to Initialize System", currentWidth / 2 - 140, currentHeight / 2 - 20, 16, RAYWHITE);

			// 操作說明
			DrawText("================ CONTROLS ================", currentWidth / 2 - 170, currentHeight / 2 + 40, 14, GRAY);
			DrawText("[WASD / Arrows]  Move & Push Window", currentWidth / 2 - 140, currentHeight / 2 + 70, 14, LIGHTGRAY);
			DrawText("[Mouse]  Aim Direction", currentWidth / 2 - 140, currentHeight / 2 + 90, 14, LIGHTGRAY);
			DrawText("[SPACE]  Attack", currentWidth / 2 - 140, currentHeight / 2 + 110, 14, LIGHTGRAY);
			DrawText("[1] Shoot Mode    [2] Melee Mode", currentWidth / 2 - 140, currentHeight / 2 + 130, 14, LIGHTGRAY);
		}
		else if (isPaused) {
			// 暫停黑幕
			DrawRectangle(0, 0, currentWidth, currentHeight, Fade(BLACK, 0.6f));
			DrawText("SYSTEM PAUSED", currentWidth / 2 - 75, currentHeight / 2 - 20, 20, LIGHTGRAY);
			DrawText("Press [ESC] or [P] to Resume", currentWidth / 2 - 110, currentHeight / 2 + 20, 16, RAYWHITE);
		}

		// ==========================================
		EndDrawing();
	}

	AssetManager::UnloadAllAssets();
	CloseWindow();
	return 0;
}
