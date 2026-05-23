#include "raylib.h"
#include "PlatformUtils.h"
#include "AssetManager.h"
#include "Map.h"
#include "Player.h"
#include "Enemy.h"
#include <vector>
#include <ctime>
#include <cstdlib>

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

	// 怪物管理（簡單示例）
	std::vector<NGJ::Enemy> enemies;
	// 若要更容易看見怪物，從地圖的玩家起始位置附近刷怪
	// seed 隨機
	std::srand((unsigned int)std::time(nullptr));
	NGJ::Vec2 start = NGJ::Vec2(dungeonMap.GetPlayerStartPos().x, dungeonMap.GetPlayerStartPos().y);
	auto spawnNear = [&](float dx, float dy) {
		return NGJ::Vec2(start.x + dx, start.y + dy);
		};
	enemies.emplace_back("Goblin", 8, 2, 0, 40.0f, 180.0f, 24.0f, 1.0f, spawnNear(0.0f, 0.0f));
	enemies.emplace_back("Wolf", 12, 3, 1, 80.0f, 220.0f, 20.0f, 1.2f, spawnNear(120.0f, 0.0f));
	enemies.emplace_back("Slime", 6, 1, 0, 30.0f, 120.0f, 18.0f, 0.8f, spawnNear(-100.0f, 40.0f));
	enemies.emplace_back("Bat", 5, 1, 0, 120.0f, 160.0f, 14.0f, 0.6f, spawnNear(40.0f, -80.0f));

	// 初始化關卡層級追蹤，避免第一幀時觸發關卡重置
	DungeonLayer lastLayer = dungeonMap.GetCurrentLayer();

	// 用於檢測視窗拖曳作弊的變數
	Vector2 lastWinPos = GetWindowPosition();
	int lastMonitor = GetCurrentMonitor();

	while (!WindowShouldClose()) {
		float dt = GetFrameTime();
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
			}
			lastLayer = dungeonMap.GetCurrentLayer();
		}

		float centerX = currentWidth * 0.5f;
		float centerY = currentHeight * 0.5f;
		float margin = 5.0f;

		// 保留地圖牆壁碰撞的移動邏輯（以鍵盤控制）
		// 注意：主移動邏輯在 main 處理，移動完成後會把位置同步到 player 以處理攻擊輸入
		if (dungeonMap.GetCurrentLayer() != DungeonLayer::VICTORY) {
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

		Vector2 playerMapPos = { (winPos.x + playerPos.x) - monLeft, (winPos.y + playerPos.y) - monTop };
		dungeonMap.Update(playerMapPos);

		// 同步位置給 Player，再處理攻擊輸入與更新（僅處理攻擊，不再次處理移動）
		player.playerPos = playerPos;
		player.currentWinPos = winPos;
		player.currentWinWidth = currentWidth;
		player.currentWinHeight = currentHeight;
		player.ProcessCombatInput();
		player.UpdateCombat(dt);

		// 更新敵人狀態（使用世界座標的 player 位置）
		NGJ::Vec2 playerWorldPos(playerMapPos.x, playerMapPos.y);
		for (auto& e : enemies) {
			e.Update(dt, playerWorldPos);
			// 若要讓敵人實際傷害玩家，可在此處處理 e.Attack() 的回傳值
			// if (e.GetState() == NGJ::EnemyState::Attack && e.CanAttack() && !e.GetIsDead()) { playerHp -= e.Attack(); }
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
			dungeonMap.DrawBaseMap();
			dungeonMap.DrawObjects();
			// 在 world-space 畫出子彈與近戰特效（將 window-local 轉為 world：world = winPos + local - monLeft/top）
			for (const auto& b : player.bullets) {
				if (b.active) {
					Vector2 worldB = { (winPos.x + b.pos.x) - monLeft, (winPos.y + b.pos.y) - monTop };
					DrawCircleV(worldB, 4.0f, YELLOW);
				}
			}
			if (player.sword.active) {
				Vector2 worldSwordCenter = { (winPos.x + player.sword.center.x) - monLeft, (winPos.y + player.sword.center.y) - monTop };
				Rectangle swordRect = { worldSwordCenter.x, worldSwordCenter.y, 55.0f, 5.0f };
				Vector2 origin = { 0.0f, 2.5f };
				DrawRectanglePro(swordRect, origin, player.sword.currentAngle, RAYWHITE);
			}

			// Draw enemies in world-space
			for (const auto& e : enemies) {
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
				int barW = 30; int hpW = (int)((float)e.GetCurrentHP() / (float)e.GetMaxHP() * barW);
				DrawRectangle((int)wp.x - barW / 2, (int)wp.y - 20, barW, 5, DARKGRAY);
				DrawRectangle((int)wp.x - barW / 2, (int)wp.y - 20, hpW, 5, RED);
				DrawText(e.name.c_str(), (int)wp.x - 16, (int)wp.y + 16, 10, BLACK);
			}

			EndMode2D();

			// Debug: 顯示怪物/玩家世界座標與轉換到螢幕位置，幫助定位為何看不到怪物
			if (!enemies.empty()) {
				const auto& e0 = enemies[0];
				Vector2 eWorld = { e0.GetPosition().x, e0.GetPosition().y };
				Vector2 eScreen = GetWorldToScreen2D(eWorld, camera);
				DrawCircleV(eScreen, 6.0f, RED);
				DrawText(TextFormat("Enemies: %d", (int)enemies.size()), 10, 10, 14, WHITE);
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

			DrawRectangle(8, 8, 220, 50, Fade(BLACK, 0.7f));
			const char* gNames[] = { "Layer 1 (Maze)", "Layer 2 (Spiral)", "Layer 3 (Rooms)", "FINAL BOSS" };
			DrawText(gNames[(int)dungeonMap.GetCurrentLayer()], 14, 12, 14, ORANGE);
			DrawText(TextFormat("Key Status: %d/%d", dungeonMap.GetKeysCollected(), dungeonMap.GetTotalKeys()), 14, 32, 14, dungeonMap.IsDoorUnlocked() ? GREEN : RED);

			// 調試信息：顯示地圖大小和鑰匙數量
			DrawText(TextFormat("Map Size: %dx%d tiles", dungeonMap.GetTotalWidth()/50, dungeonMap.GetTotalHeight()/50), 10, 100, 12, YELLOW);
			DrawText(TextFormat("Keys Found: %d", dungeonMap.GetKeysCollected()), 10, 115, 12, YELLOW);
		}

		EndDrawing();
	}

	AssetManager::UnloadAllAssets();
	CloseWindow();
	return 0;
}
