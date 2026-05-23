#include "raylib.h"
#include "PlatformUtils.h"
#include "AssetManager.h"
#include "Map.h"

int main() {
	int currentWidth = 400;
	int currentHeight = 400;

	InitWindow(currentWidth, currentHeight, "NGJ2026 - 4D Window Expand Dungeon!");
	AssetManager::LoadAllAssets();
	SetWindowPosition(500, 300);
	SetTargetFPS(60);

	int prevMonitor = GetCurrentMonitor();

	// 初始化自動產圖地圖系統
	Map dungeonMap(currentWidth, currentHeight);
	// 開啟地圖 debug overlay，顯示取樣顏色與提示數值
	dungeonMap.SetDebugMode(true);

	// 玩家在視窗內的相對位置
	Vector2 playerPos = { 200.0f, 200.0f };
	float playerSpeed = 5.0f;
	float playerRadius = 16.0f; // 用於地圖碰撞偵測

	// 設定 2D 攝影機實現地圖與視窗桌面的完美映射
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

		// 【關卡切換重置機制】當層級切換時，自動把視窗和玩家精準校位至新地圖的綠色出生點
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

		// --- 融合原版視窗追隨邏輯與全新地圖牆壁碰撞偵測 ---
		if (dungeonMap.GetCurrentLayer() != DungeonLayer::VICTORY) {

			// 水平移動（右）
			if (IsKeyDown(KEY_RIGHT)) {
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

			// 水平移動（左）
			if (IsKeyDown(KEY_LEFT)) {
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

			// 垂直移動（下）
			if (IsKeyDown(KEY_DOWN)) {
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

			// 垂直移動（上）
			if (IsKeyDown(KEY_UP)) {
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

		// 核心校準：計算玩家目前在桌面工作區（地圖世界）的絕對座標
		Vector2 playerMapPos = { (winPos.x + playerPos.x) - monLeft, (winPos.y + playerPos.y) - monTop };

		// 更新地圖狀態（撿鑰匙、過關判斷）
		dungeonMap.Update(playerMapPos);

		// 設定 2D 攝影機：將玩家的網格世界座標映射到視窗畫面上
		camera.target = playerMapPos;
		camera.offset = playerPos;

		// 繪製畫面
		BeginDrawing();
		ClearBackground(BLACK);

		if (dungeonMap.GetCurrentLayer() == DungeonLayer::VICTORY) {
			DrawText("VICTORY!", currentWidth / 2 - 60, currentHeight / 2 - 10, 24, GOLD);
			DrawText("Dungeon Defeated!", currentWidth / 2 - 80, currentHeight / 2 + 20, 16, WHITE);
		}
		else {
			// 進入 2D 視界渲染地圖與物件
			BeginMode2D(camera);
			dungeonMap.DrawBaseMap();
			dungeonMap.DrawObjects();
			EndMode2D();

			// 畫出主角動畫（保持在相對視窗座標）
			AssetManager::DrawPlayerAnimated(playerPos, WHITE);

			// 左上角固定 UI 狀態面板
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