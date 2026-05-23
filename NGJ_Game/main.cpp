#include "raylib.h"
#include "PlatformUtils.h"
#include "AssetManager.h"
#include "Map.h"
#include "Player.h"

int main() {
	// 使用 Player 的視窗初始值以保持先前行為
	int currentWidth = 400;
	int currentHeight = 400;

	// 先讓視窗出現在螢幕中間偏左上的位置，方便測試擴張
	InitWindow(currentWidth, currentHeight, "NGJ2026 - Integration: Map + Player");
	AssetManager::LoadAllAssets();
	SetWindowPosition(500, 300);
	SetTargetFPS(60);

	// 追蹤前一個所在的顯示器索引
	int prevMonitor = GetCurrentMonitor();

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

	while (!WindowShouldClose()) {
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
		} else {
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
		int downAvailable  = maxHeight - relWinY - currentHeight;
		int leftAvailable  = relWinX;
		int upAvailable    = relWinY - 40;
		if (upAvailable < 0) upAvailable = 0;

		// 關卡切換重置機制
		static DungeonLayer lastLayer = (DungeonLayer)-1;
		if (dungeonMap.GetCurrentLayer() != lastLayer) {
			if (dungeonMap.GetCurrentLayer() != DungeonLayer::VICTORY) {
				Vector2 startMapPos = dungeonMap.GetPlayerStartPos();
				float cX = currentWidth * 0.5f;
				float cY = currentHeight * 0.5f;
				int targetWinX = (int)(monLeft + startMapPos.x - cX);
				int targetWinY = (int)(monTop + startMapPos.y - cY);
				SetWindowPosition(targetWinX, targetWinY);
				playerPos = { cX, cY };
				winPos = GetWindowPosition();
			}
			lastLayer = dungeonMap.GetCurrentLayer();
		}

		float centerX = currentWidth * 0.5f;
		float centerY = currentHeight * 0.5f;
		float margin = 5.0f;

		// 保留地圖牆壁碰撞的移動邏輯（以鍵盤控制）
		if (dungeonMap.GetCurrentLayer() != DungeonLayer::VICTORY) {
			if (IsKeyDown(KEY_RIGHT)) {
				float targetPlayerX = playerPos.x;
				int targetWinX = (int)winPos.x;

				if (playerPos.x < centerX) {
					targetPlayerX += playerSpeed;
					if (targetPlayerX > centerX) targetPlayerX = centerX;
				} else {
					int winRightDesk = (int)winPos.x + currentWidth;
					int screenRight = monLeft + monW;
					int availableRight = screenRight - winRightDesk;
					if (availableRight > 0) {
						int delta = (availableRight >= (int)playerSpeed) ? (int)playerSpeed : availableRight;
						targetWinX += delta;
						targetPlayerX = centerX;
					} else {
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

			if (IsKeyDown(KEY_LEFT)) {
				float targetPlayerX = playerPos.x;
				int targetWinX = (int)winPos.x;

				if (playerPos.x > centerX) {
					targetPlayerX -= playerSpeed;
					if (targetPlayerX < centerX) targetPlayerX = centerX;
				} else {
					int winLeftDesk = (int)winPos.x;
					int screenLeft = monLeft;
					int availableLeft = winLeftDesk - screenLeft;
					if (availableLeft > 0) {
						int delta = (availableLeft >= (int)playerSpeed) ? (int)playerSpeed : availableLeft;
						targetWinX -= delta;
						targetPlayerX = centerX;
					} else {
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

			if (IsKeyDown(KEY_DOWN)) {
				float targetPlayerY = playerPos.y;
				int targetWinY = (int)winPos.y;

				if (playerPos.y < centerY) {
					targetPlayerY += playerSpeed;
					if (targetPlayerY > centerY) targetPlayerY = centerY;
				} else {
					int winBottomDesk = (int)winPos.y + currentHeight;
					int screenBottom = monTop + monH;
					int availableDown = screenBottom - winBottomDesk;
					if (availableDown > 0) {
						int delta = (availableDown >= (int)playerSpeed) ? (int)playerSpeed : availableDown;
						targetWinY += delta;
						targetPlayerY = centerY;
					} else {
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

			if (IsKeyDown(KEY_UP)) {
				float targetPlayerY = playerPos.y;
				int targetWinY = (int)winPos.y;

				if (playerPos.y > centerY) {
					targetPlayerY -= playerSpeed;
					if (targetPlayerY < centerY) targetPlayerY = centerY;
				} else {
					int winTopDesk = (int)winPos.y;
					int screenTop = monTop;
					int availableUp = winTopDesk - screenTop;
					if (availableUp > 0) {
						int delta = (availableUp >= (int)playerSpeed) ? (int)playerSpeed : availableUp;
						targetWinY -= delta;
						targetPlayerY = centerY;
					} else {
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

		Vector2 playerMapPos = { (winPos.x + playerPos.x) - monLeft, (winPos.y + playerPos.y) - monTop };
		dungeonMap.Update(playerMapPos);

		camera.target = playerMapPos;
		camera.offset = playerPos;

		BeginDrawing();
		ClearBackground(BLACK);

		if (dungeonMap.GetCurrentLayer() == DungeonLayer::VICTORY) {
			DrawText("VICTORY!", currentWidth / 2 - 60, currentHeight / 2 - 10, 24, GOLD);
			DrawText("Dungeon Defeated!", currentWidth / 2 - 80, currentHeight / 2 + 20, 16, WHITE);
		} else {
			BeginMode2D(camera);
			dungeonMap.DrawBaseMap();
			dungeonMap.DrawObjects();
			EndMode2D();

			// 同步 player 內部狀態至 main 的 playerPos 以供繪製（player 仍管理攻擊/子彈資料）
			player.playerPos = playerPos;
			// Draw player using AssetManager at window-local position
			AssetManager::DrawPlayerAnimated(playerPos, WHITE);

			DrawRectangle(8, 8, 220, 50, Fade(BLACK, 0.7f));
			const char* gNames[] = { "Layer 1 (Maze)", "Layer 2 (Spiral)", "Layer 3 (Rooms)", "FINAL BOSS" };
			DrawText(gNames[(int)dungeonMap.GetCurrentLayer()], 14, 12, 14, ORANGE);
			DrawText(TextFormat("Key Status: %s", dungeonMap.PlayerHasKey() ? "READY" : "SEARCHING..."), 14, 32, 14, dungeonMap.PlayerHasKey() ? GREEN : RED);
		}

		EndDrawing();
	}

	AssetManager::UnloadAllAssets();
	CloseWindow();
	return 0;
}
