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

		// 基礎移動控制（限制在視窗內）
		if (IsKeyDown(KEY_RIGHT) && playerPos.x <= currentWidth - 25) playerPos.x += playerSpeed;
		if (IsKeyDown(KEY_LEFT)  && playerPos.x >= 5)  playerPos.x -= playerSpeed;
		if (IsKeyDown(KEY_DOWN)  && playerPos.y <= currentHeight - 25)  playerPos.y += playerSpeed;
		if (IsKeyDown(KEY_UP)    && playerPos.y >= 5)    playerPos.y -= playerSpeed;

		// 1. ➡️ 往右擴張（不超出當前顯示器）
		if (playerPos.x >= currentWidth - 20 && rightAvailable > 0) {
			int grow = (rightAvailable >= 10) ? 10 : rightAvailable;
			currentWidth += grow;
			SetWindowSize(currentWidth, currentHeight);
		}

		// 2. ⬇️ 往下擴張（不超出當前顯示器）
		if (playerPos.y >= currentHeight - 20 && downAvailable > 0) {
			int grow = (downAvailable >= 10) ? 10 : downAvailable;
			currentHeight += grow;
			SetWindowSize(currentWidth, currentHeight);
		}

		// 3. ⬅️ 往左擴張（只在視窗左邊還有空間時）
		if (playerPos.x <= 0 && leftAvailable > 0) {
			int grow = (leftAvailable >= 10) ? 10 : leftAvailable;
			// 計算相對於該顯示器的新位置（使用 relWinX 避免桌面座標混淆）
			int newRelX = relWinX - grow;
			if (newRelX < 0) newRelX = 0; // 不超出顯示器左邊
			int newPosX = monLeft + newRelX;
			// 先設定位置再設定大小，避免某些系統在改大小時重定位視窗到主螢幕
			SetWindowPosition(newPosX, monTop + relWinY);
			SetWindowSize(currentWidth + grow, currentHeight);
			currentWidth += grow; // 更新內部尺寸
			// 因為視窗往左長了 grow 像素，主角在視窗內的相對座標必須右移 grow 像素
			playerPos.x += (float)grow;
			// 立刻更新 winPos，避免下一次計算使用舊值
			winPos = GetWindowPosition();
		}

		// 4. ⬆️ 往上擴張（只在視窗上邊還有空間時）
		if (playerPos.y <= 0 && upAvailable > 0) {
			int grow = (upAvailable >= 10) ? 10 : upAvailable;
			currentHeight += grow; // 視窗變高
			// 把整個視窗往上移 grow 像素，但確保不會移到顯示器上方以外
			int newPosY = (int)winPos.y - grow;
			if (newPosY < 0) newPosY = 0;
			SetWindowPosition((int)winPos.x, newPosY);
			SetWindowSize(currentWidth, currentHeight);
			// 主角在視窗內的相對座標下移 grow 像素
			playerPos.y += (float)grow;
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
