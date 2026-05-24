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

int main() {
	// 使用 Player 的視窗初始值以保持先前行為
	int currentWidth = 400;
	int currentHeight = 400;

	// 先讓視窗出現在螢幕中間偏左上的位置，方便測試擴張
	InitWindow(currentWidth, currentHeight, "NGJ2026 - Integration: Map + Player");
	AssetManager::LoadAllAssets();
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

	// 初始化地圖系統
	Map dungeonMap(currentWidth, currentHeight);

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
		for (int i = 0; i < 3; i++) {
			int typeCount = ((int)dungeonMap.GetCurrentLayer() >= (int)DungeonLayer::LAYER_2) ? 5 : 4;
			int t = std::rand() % typeCount;
			float px = 80.0f + (float)(std::rand() % (currentWidth - 160));
			int upperMinY = 60;
			int upperMaxY = (currentHeight / 2) - 60;
			if (upperMaxY < upperMinY) upperMaxY = upperMinY;
			float py = (float)upperMinY + (float)(std::rand() % (upperMaxY - upperMinY + 1));
			NGJ::Vec2 pos(px, py);

			switch (t) {
			case 0:
				chestRoomEnemies.emplace_back("Goblin", 8, 5, 0, 40.0f, 300.0f, 24.0f, 1.0f, pos);
				break;
			case 1:
				chestRoomEnemies.emplace_back("Wolf", 12, 5, 1, 80.0f, 400.0f, 20.0f, 1.2f, pos);
				chestRoomEnemies.back().SetRangedAttack(true, 260.0f, 0.7f);
				break;
			case 2:
				chestRoomEnemies.emplace_back("Slime", 6, 5, 0, 30.0f, 250.0f, 18.0f, 0.8f, pos);
				break;
			case 3:
				chestRoomEnemies.emplace_back("Bat", 5, 5, 0, 120.0f, 350.0f, 14.0f, 0.6f, pos);
				chestRoomEnemies.back().SetRangedAttack(true, 330.0f, 0.45f);
				break;
			default:
				chestRoomEnemies.emplace_back("Assassin", 7, 6, 0, 20.0f, 420.0f, 18.0f, 0.9f, pos);
				break;
			}
		}
	};

	// 呼叫 spawnRandom() 讓怪物誕生在隨機角落
	enemies.emplace_back("Goblin", 8, 5, 0, 40.0f, 300.0f, 24.0f, 1.0f, spawnRandom());
	enemies.emplace_back("Wolf", 12, 5, 1, 80.0f, 400.0f, 20.0f, 1.2f, spawnRandom());
	enemies.emplace_back("Slime", 6, 3, 0, 30.0f, 250.0f, 18.0f, 0.8f, spawnRandom());
	enemies.emplace_back("Bat", 5, 3, 0, 120.0f, 350.0f, 14.0f, 0.6f, spawnRandom());
	if ((int)dungeonMap.GetCurrentLayer() >= (int)DungeonLayer::LAYER_2) {
		enemies.emplace_back("Assassin", 5, 6, 0, 20.0f, 420.0f, 18.0f, 0.9f, spawnRandom());
	}

	// 指定部分敵人為遠程攻擊型
	if (enemies.size() > 1) enemies[1].SetRangedAttack(true, 260.0f, 0.7f); // Wolf
	if (enemies.size() > 3) enemies[3].SetRangedAttack(true, 330.0f, 0.45f); // Bat


	// 初始化關卡層級追蹤，避免第一幀時觸發關卡重置
	DungeonLayer lastLayer = dungeonMap.GetCurrentLayer();

	// 近戰命中追蹤：每次揮劍每個敵人只會吃到一次傷害
	std::vector<bool> swordHitThisSwing(enemies.size(), false);
	bool wasSwordActive = false;

	// 用於檢測視窗拖曳作弊的變數
	Vector2 lastWinPos = GetWindowPosition();
	int lastMonitor = GetCurrentMonitor();

	while (!WindowShouldClose()) {
		float dt = GetFrameTime();
		bool isUiPause = showRewardUI || showChestEntryPrompt;
		if (!isGameOver && !isUiPause && playerInvincibleTimer > 0.0f) {
			playerInvincibleTimer -= dt;
			if (playerInvincibleTimer < 0.0f) playerInvincibleTimer = 0.0f;
		}
		if (player.currentHp <= 0) {
			isGameOver = true;
		}
		if (isGameOver) {
			BeginDrawing();
			ClearBackground(BLACK);
			DrawText("GAME OVER", currentWidth / 2 - 90, currentHeight / 2 - 20, 36, RED);
			DrawText("Press ESC to quit", currentWidth / 2 - 90, currentHeight / 2 + 20, 18, WHITE);
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
		if (dungeonMap.GetCurrentLayer() != lastLayer) {
			if (dungeonMap.GetCurrentLayer() != DungeonLayer::VICTORY) {
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
				enemies.emplace_back("Goblin", 8, 5, 0, 40.0f, 300.0f, 24.0f, 1.0f, spawnRandom());
				enemies.emplace_back("Wolf", 8, 5, 1, 80.0f, 400.0f, 20.0f, 1.2f, spawnRandom());
				enemies.emplace_back("Slime", 6, 5, 0, 30.0f, 250.0f, 18.0f, 0.8f, spawnRandom());
				enemies.emplace_back("Bat", 5, 5, 0, 120.0f, 350.0f, 14.0f, 0.6f, spawnRandom());
				if ((int)dungeonMap.GetCurrentLayer() >= (int)DungeonLayer::LAYER_2) {
					enemies.emplace_back("Assassin", 7, 6, 0, 20.0f, 420.0f, 18.0f, 0.9f, spawnRandom());
				}

				if (enemies.size() > 1) enemies[1].SetRangedAttack(true, 260.0f, 0.7f);
				if (enemies.size() > 3) enemies[3].SetRangedAttack(true, 330.0f, 0.45f);

				enemySpawnTimer = 0.0f;
				swordHitThisSwing.assign(enemies.size(), false);
				wasSwordActive = false;
			}
			lastLayer = dungeonMap.GetCurrentLayer();
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
		// 注意：主移動邏輯在 main 處理，移動完成後會把位置同步到 player 以處理攻擊輸入
		if (!inChestRoom && dungeonMap.GetCurrentLayer() != DungeonLayer::VICTORY && !isUiPause) {
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
						float maxPlayerX = (float)(currentWidth - 25);
						targetPlayerX += playerSpeed;
						if (targetPlayerX > maxPlayerX) targetPlayerX = maxPlayerX;
					}
				}

				float potentialMapX = (targetWinX + targetPlayerX) - monLeft;
				float currentMapY = (winPos.y + playerPos.y) - monTop;

				if (!dungeonMap.IsWall(potentialMapX + playerRadius, currentMapY) &&
					!dungeonMap.IsWall(potentialMapX - playerRadius, currentMapY)) {
					playerPos.x = targetPlayerX;
					if ((int)winPos.x != targetWinX) {
						SetWindowPosition(targetWinX, (int)winPos.y);
						winPos.x = (float)targetWinX;
					}
				}
			}

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

				if (!dungeonMap.IsWall(potentialMapX - playerRadius, currentMapY) &&
					!dungeonMap.IsWall(potentialMapX + playerRadius, currentMapY)) {
					playerPos.x = targetPlayerX;
					if ((int)winPos.x != targetWinX) {
						SetWindowPosition(targetWinX, (int)winPos.y);
						winPos.x = (float)targetWinX;
					}
				}
			}

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
						float maxPlayerY = (float)(currentHeight - 25);
						targetPlayerY += playerSpeed;
						if (targetPlayerY > maxPlayerY) targetPlayerY = maxPlayerY;
					}
				}

				float currentMapX = (winPos.x + playerPos.x) - monLeft;
				float potentialMapY = (targetWinY + targetPlayerY) - monTop;

				if (!dungeonMap.IsWall(currentMapX, potentialMapY + playerRadius) &&
					!dungeonMap.IsWall(currentMapX, potentialMapY - playerRadius)) {
					playerPos.y = targetPlayerY;
					if ((int)winPos.y != targetWinY) {
						SetWindowPosition((int)winPos.x, targetWinY);
						winPos.y = (float)targetWinY;
					}
				}
			}

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
						targetPlayerY = centerY;
					}
					else {
						targetPlayerY -= playerSpeed;
						if (targetPlayerY < margin) targetPlayerY = margin;
					}
				}

				float currentMapX = (winPos.x + playerPos.x) - monLeft;
				float potentialMapY = (targetWinY + targetPlayerY) - monTop;

				if (!dungeonMap.IsWall(currentMapX, potentialMapY - playerRadius) &&
					!dungeonMap.IsWall(currentMapX, potentialMapY + playerRadius)) {
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
			for (int i = 0; i < (int)chestPositions.size(); i++) {
				if (!chestOpened[i]) {
					if (i == declinedChestIndex) continue;
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
			}
		}

		// 獎勵 UI 交互
		if (showRewardUI && currentChestIndex >= 0) {
			rewardUITimer += dt;

			// 用數字鍵 1/2/3 選擇獎勵，或用方向鍵 + Enter
			if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) {
				selectedReward = 0;  // 血量上限 +20
			} else if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) {
				selectedReward = 1;  // 移動速度 +1
			} else if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) {
				selectedReward = 2;  // 攻擊距離 +10
			}

			// 如果選擇了獎勵，套用並關閉 UI
			if (selectedReward >= 0) {
					switch (selectedReward) {
					case 0:  // 增加血量上限
						player.maxHp += 20;
						player.currentHp = player.maxHp;
						break;
					case 1:  // 增加移動速度
						playerSpeed += 1.0f;
						break;
					case 2:  // 增加攻擊距離
						player.attackRange += 10.0f;
						break;
				}

				// 標記寶箱已打開
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

			// 如果超過 10 秒沒選擇，自動關閉
			if (rewardUITimer > 10.0) {
				showRewardUI = false;
				currentChestIndex = -1;
				selectedReward = -1;
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
		}

		// 更新敵人狀態（使用世界座標的 player 位置）
		NGJ::Vec2 playerWorldPos(playerMapPos.x, playerMapPos.y);
		NGJ::Vec2 viewMin((float)(winPos.x - monLeft), (float)(winPos.y - monTop));
		NGJ::Vec2 viewMax(viewMin.x + (float)player.currentWinWidth, viewMin.y + (float)player.currentWinHeight);
		auto& activeEnemies = inChestRoom ? chestRoomEnemies : enemies;
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

		// 每 7 秒新增一隻隨機怪物（僅一般地圖）
		if (!inChestRoom && !isUiPause && dungeonMap.GetCurrentLayer() != DungeonLayer::VICTORY) {
			enemySpawnTimer += dt;
			if (enemySpawnTimer >= enemySpawnInterval) {
				enemySpawnTimer -= enemySpawnInterval;
				int typeCount = ((int)dungeonMap.GetCurrentLayer() >= (int)DungeonLayer::LAYER_2) ? 5 : 4;
				int t = std::rand() % typeCount;
				switch (t) {
				case 0:
					enemies.emplace_back("Goblin", 8, 5, 0, 40.0f, 300.0f, 24.0f, 1.0f, spawnRandom());
					break;
				case 1:
					enemies.emplace_back("Wolf", 12, 5, 1, 80.0f, 400.0f, 20.0f, 1.2f, spawnRandom());
					enemies.back().SetRangedAttack(true, 260.0f, 0.7f);
					break;
				case 2:
					enemies.emplace_back("Slime", 6, 5, 0, 30.0f, 250.0f, 18.0f, 0.8f, spawnRandom());
					break;
				case 3:
					enemies.emplace_back("Bat", 5, 5, 0, 120.0f, 350.0f, 14.0f, 0.6f, spawnRandom());
					enemies.back().SetRangedAttack(true, 330.0f, 0.45f);
					break;
				default:
					enemies.emplace_back("Assassin", 7, 6, 0, 20.0f, 420.0f, 18.0f, 0.9f, spawnRandom());
					break;
				}
				swordHitThisSwing.push_back(false);
			}
		}

		// 玩家攻擊命中判定：子彈與近戰揮砍命中敵人時扣血
		const int bulletDamage = 2;
		const int swordDamage = 4;
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

			if (playerInvincibleTimer <= 0.0f) {
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
					e.TakeDamage(bulletDamage);
					b.active = false;
					break;
				}
			}

			if (e.GetIsDead()) continue;

			if (playerInvincibleTimer <= 0.0f) {
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
					e.TakeDamage(swordDamage);
					swordHitThisSwing[ei] = true;
				}
			}
		}

		camera.target = playerMapPos;
		camera.offset = playerPos;

		BeginDrawing();
		ClearBackground(BLACK);

		if (dungeonMap.GetCurrentLayer() == DungeonLayer::VICTORY) {
			DrawText("VICTORY!", currentWidth / 2 - 60, currentHeight / 2 - 10, 24, GOLD);
			DrawText("Dungeon Defeated!", currentWidth / 2 - 80, currentHeight / 2 + 20, 16, WHITE);
		}
		else {
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

			// Draw enemies in world-space
			for (const auto& e : drawEnemies) {
				if (e.GetIsDead()) continue;
				Vector2 wp = { e.GetPosition().x, e.GetPosition().y };
				Color col = GRAY;
				switch (e.GetState()) {
				case NGJ::EnemyState::Idle: col = LIGHTGRAY; break;
				case NGJ::EnemyState::Patrol: col = GREEN; break;
				case NGJ::EnemyState::Chase: col = ORANGE; break;
				case NGJ::EnemyState::Attack: col = RED; break;
				case NGJ::EnemyState::Dead: col = DARKGRAY; break;
				}
				DrawCircle((int)wp.x, (int)wp.y, 12, col);

				// Goblin 盾牌線條（朝向玩家）
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

				int barW = 30; int hpW = (int)((float)e.GetCurrentHP() / (float)e.GetMaxHP() * barW);
				DrawRectangle((int)wp.x - barW / 2, (int)wp.y - 20, barW, 5, DARKGRAY);
				DrawRectangle((int)wp.x - barW / 2, (int)wp.y - 20, hpW, 5, RED);
				DrawText(e.name.c_str(), (int)wp.x - 16, (int)wp.y + 16, 10, BLACK);
			}

			EndMode2D();

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

			DrawRectangle(8, 8, 260, 68, Fade(BLACK, 0.7f));
			const char* gNames[] = { "Layer 1 (Maze)", "Layer 2 (Spiral)", "Layer 3 (Rooms)", "FINAL BOSS" };
			DrawText(gNames[(int)dungeonMap.GetCurrentLayer()], 14, 12, 14, ORANGE);
			DrawText(TextFormat("Key Status: %d/%d", dungeonMap.GetKeysCollected(), dungeonMap.GetTotalKeys()), 14, 32, 14, dungeonMap.IsDoorUnlocked() ? GREEN : RED);
			DrawText(TextFormat("HP: %d/%d", player.currentHp, player.maxHp), 14, 50, 14, player.currentHp <= 1 ? RED : WHITE);

			// 調試信息：顯示地圖大小和鑰匙數量
			DrawText(TextFormat("Map Size: %dx%d tiles", dungeonMap.GetTotalWidth()/50, dungeonMap.GetTotalHeight()/50), 10, 100, 12, YELLOW);
			DrawText(TextFormat("Keys Found: %d", dungeonMap.GetKeysCollected()), 10, 115, 12, YELLOW);

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

				// 三個獎勵選項
				const char* rewardNames[] = { 
					"[1] Max HP +20", 
					"[2] Speed +1", 
					"[3] Attack Range +10" 
				};
				Color rewardColors[] = { RED, BLUE, ORANGE };

				for (int i = 0; i < 3; i++) {
					int optionY = uiY + 50 + i * 40;
					Color optColor = (selectedReward == i) ? YELLOW : rewardColors[i];
					DrawRectangle(uiX + 20, optionY, uiWidth - 40, 35, Fade(BLACK, 0.3f));
					DrawRectangleLines(uiX + 20, optionY, uiWidth - 40, 35, optColor);
					DrawText(rewardNames[i], uiX + 35, optionY + 8, 14, optColor);
				}

				DrawText("Selecting in...", uiX + 20, uiY + 180, 10, GRAY);
			}
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
		EndDrawing();
	}

	AssetManager::UnloadAllAssets();
	CloseWindow();
	return 0;
}
