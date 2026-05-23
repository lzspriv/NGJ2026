#include "raylib.h"
#include "PlatformUtils.h"
#include "AssetManager.h"

int main() {
	int currentWidth = 400;
	int currentHeight = 400;

	// 先讓視窗出現在螢幕中間偏左上的位置，方便測試擴張
	InitWindow(currentWidth, currentHeight, "NGJ2026 - 4D Window Expand!");
	AssetManager::LoadAllAssets();
	SetWindowPosition(500, 300);
	SetTargetFPS(60);

	// 追蹤前一個所在的顯示器索引，若使用者把視窗拖到另一個螢幕，會自動置中
	int prevMonitor = GetCurrentMonitor();

	Vector2 playerPos = { 200.0f, 200.0f };
	float playerSpeed = 5.0f;

	while (!WindowShouldClose()) {
		// 每一幀取得目前視窗在桌面座標上的絕對位置
		Vector2 winPos = GetWindowPosition();

		// 取得目前 Raylib 所判定的顯示器索引與尺寸
		int monitor = GetCurrentMonitor();
		int maxWidth = GetMonitorWidth(monitor);
		int maxHeight = GetMonitorHeight(monitor);

		// 當偵測到顯示器改變（使用者把視窗拖到另一個螢幕）時，自動把視窗置中到該顯示器
		if (monitor != prevMonitor) {
			// 嘗試使用平台工具取得該顯示器在桌面座標系的矩形
			int monLeft = 0, monTop = 0, monW = GetMonitorWidth(monitor), monH = GetMonitorHeight(monitor);
			// 嘗試呼叫本地平臺 helper（Windows 會回傳正確的 monLeft/monTop）
			if (GetMonitorRectForWindow(GetWindowHandle(), monLeft, monTop, monW, monH)) {
				// 成功取得
			}

			// 如果當前視窗尺寸超過目標顯示器，先 clamp 大小，避免被放置到螢幕外
			if (currentWidth > monW) currentWidth = monW;
			if (currentHeight > monH) currentHeight = monH;

			int newX = monLeft + (monW - currentWidth) / 2;
			int newY = monTop + (monH - currentHeight) / 2;
			// 先設定大小再設定位置，確保置中計算正確
			SetWindowSize(currentWidth, currentHeight);
			SetWindowPosition(newX, newY);
			// 立刻更新 winPos，確保後續基於當前視窗位置的計算正確
			winPos = GetWindowPosition();
			prevMonitor = monitor;
		}

		// 嘗試使用 PlatformUtils 取得此視窗所在顯示器的工作區矩形（桌面座標，不含任務列）以正確計算相對位置
		int monLeft = 0;
		int monTop = 0;
		int monW = maxWidth;
		int monH = maxHeight;
		// 優先嘗試取得工作區，失敗時退回普通顯示器矩形
		if (!GetMonitorWorkAreaForWindow(GetWindowHandle(), monLeft, monTop, monW, monH)) {
			GetMonitorRectForWindow(GetWindowHandle(), monLeft, monTop, monW, monH);
		}
		// 如果成功取得，將作為有效的顯示器範圍；否則保留 Raylib 的 monitor 大小，且 monLeft/monTop 為 0
		bool gotWork = (monW != maxWidth || monH != maxHeight || monLeft != 0 || monTop != 0);
		if (gotWork) {
			maxWidth = monW;
			maxHeight = monH;
		} else {
			monLeft = 0;
			monTop = 0;
		}

		// 計算視窗相對於該顯示器原點的座標，用於擴張邏輯
		int relWinX = (int)winPos.x - monLeft;
		int relWinY = (int)winPos.y - monTop;
		// 邊界保護
		if (relWinX < 0) relWinX = 0;
		if (relWinY < 0) relWinY = 0;
		if (relWinX > maxWidth) relWinX = maxWidth;
		if (relWinY > maxHeight) relWinY = maxHeight;

		// 最後做邊界保護（相對座標）
		if (relWinX < 0) relWinX = 0;
		if (relWinY < 0) relWinY = 0;
		if (relWinX > maxWidth) relWinX = maxWidth;
		if (relWinY > maxHeight) relWinY = maxHeight;

		// 計算可用擴張空間（避免超出當前顯示器）
		int rightAvailable = maxWidth - relWinX - currentWidth; // 還能往右擴張的像素數
		int downAvailable  = maxHeight - relWinY - currentHeight; // 還能往下擴張的像素數
		int leftAvailable  = relWinX; // 還能往左擴張的像素數（視窗左邊到顯示器左邊）
		int upAvailable    = relWinY - 40; // 還能往上擴張的像素數，保留標題列高度緩衝
		if (upAvailable < 0) upAvailable = 0;

		// 新的視窗追隨邏輯：
		// - 當按方向鍵時，角色會先在視窗內移動到視窗中心；
		// - 若角色已在視窗中心，視窗開始沿著該方向在顯示器上移動（角色保持在視窗中心）；
		// - 當視窗無法再移動（到達顯示器邊界）時，角色才在視窗內繼續移動直到撞到視窗邊界。
		float centerX = currentWidth * 0.5f;
		float centerY = currentHeight * 0.5f;
		float margin = 5.0f; // 視窗內角色邊界緩衝

		// 水平移動（右）
		if (IsKeyDown(KEY_RIGHT)) {
			if (playerPos.x < centerX) {
				// 先把角色移到視窗中心
				playerPos.x += playerSpeed;
				if (playerPos.x > centerX) playerPos.x = centerX;
			} else {
				// 角色已在中心，嘗試移動視窗到右邊（以顯示器為界）
				int winRightDesk = (int)winPos.x + currentWidth;
				int screenRight = monLeft + monW;
				int availableRight = screenRight - winRightDesk;
				if (availableRight > 0) {
					int delta = (availableRight >= (int)playerSpeed) ? (int)playerSpeed : availableRight;
					SetWindowPosition((int)winPos.x + delta, (int)winPos.y);
					// 角色保持在中心
					playerPos.x = centerX;
					winPos = GetWindowPosition();
				} else {
					// 視窗已到螢幕右邊，角色在視窗內繼續向右移動
					float maxPlayerX = (float)(currentWidth - 25);
					playerPos.x += playerSpeed;
					if (playerPos.x > maxPlayerX) playerPos.x = maxPlayerX;
				}
			}
		}

		// 水平移動（左）
		if (IsKeyDown(KEY_LEFT)) {
			if (playerPos.x > centerX) {
				// 先把角色回到視窗中心
				playerPos.x -= playerSpeed;
				if (playerPos.x < centerX) playerPos.x = centerX;
			} else {
				// 角色已在中心或左側，嘗試移動視窗到左邊
				int winLeftDesk = (int)winPos.x;
				int screenLeft = monLeft;
				int availableLeft = winLeftDesk - screenLeft;
				if (availableLeft > 0) {
					int delta = (availableLeft >= (int)playerSpeed) ? (int)playerSpeed : availableLeft;
					SetWindowPosition((int)winPos.x - delta, (int)winPos.y);
					// 角色保持在中心
					playerPos.x = centerX;
					winPos = GetWindowPosition();
				} else {
					// 視窗已到螢幕左邊，角色在視窗內繼續向左移動
					playerPos.x -= playerSpeed;
					if (playerPos.x < margin) playerPos.x = margin;
				}
			}
		}

		// 垂直移動（下）
		if (IsKeyDown(KEY_DOWN)) {
			if (playerPos.y < centerY) {
				playerPos.y += playerSpeed;
				if (playerPos.y > centerY) playerPos.y = centerY;
			} else {
				int winBottomDesk = (int)winPos.y + currentHeight;
				int screenBottom = monTop + monH;
				int availableDown = screenBottom - winBottomDesk;
				if (availableDown > 0) {
					int delta = (availableDown >= (int)playerSpeed) ? (int)playerSpeed : availableDown;
					SetWindowPosition((int)winPos.x, (int)winPos.y + delta);
					playerPos.y = centerY;
					winPos = GetWindowPosition();
				} else {
					float maxPlayerY = (float)(currentHeight - 25);
					playerPos.y += playerSpeed;
					if (playerPos.y > maxPlayerY) playerPos.y = maxPlayerY;
				}
			}
		}

		// 垂直移動（上）
		if (IsKeyDown(KEY_UP)) {
			if (playerPos.y > centerY) {
				playerPos.y -= playerSpeed;
				if (playerPos.y < centerY) playerPos.y = centerY;
			} else {
				int winTopDesk = (int)winPos.y;
				int screenTop = monTop;
				int availableUp = winTopDesk - screenTop;
				if (availableUp > 0) {
					int delta = (availableUp >= (int)playerSpeed) ? (int)playerSpeed : availableUp;
					SetWindowPosition((int)winPos.x, (int)winPos.y - delta);
					playerPos.y = centerY;
					winPos = GetWindowPosition();
				} else {
					playerPos.y -= playerSpeed;
					if (playerPos.y < margin) playerPos.y = margin;
				}
			}
		}

		// 繪製畫面
		BeginDrawing();
		ClearBackground(BLACK);

		DrawText("4-Directional Expansion!", 20, 20, 18, GREEN);

		// 畫出主角小藍方塊
		AssetManager::DrawPlayerAnimated(playerPos, WHITE);

		EndDrawing();
	}

	AssetManager::UnloadAllAssets();
	CloseWindow();
	return 0;
}
