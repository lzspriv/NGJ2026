#include "raylib.h"


int main() {
	int currentWidth = 400;
	int currentHeight = 400;

	// 先讓視窗出現在螢幕中間偏左上的位置，方便測試擴張
	InitWindow(currentWidth, currentHeight, "NGJ2026 - 4D Window Expand!");
	SetWindowPosition(500, 300);
	SetTargetFPS(60);

	Vector2 playerPos = { 200.0f, 200.0f };
	float playerSpeed = 5.0f;

	while (!WindowShouldClose()) {
		// 每一幀取得目前視窗在桌面座標上的絕對位置
		Vector2 winPos = GetWindowPosition();

		// 預設使用 Raylib 提供的當前 monitor 資訊作為備援
		int monitor = GetCurrentMonitor();
		int maxWidth = GetMonitorWidth(monitor);
		int maxHeight = GetMonitorHeight(monitor);

		// 監視器原點（桌面座標），預設為 0,0（單螢幕或無法取得時）
		int monLeft = 0;
		int monTop = 0;
		int monRight = maxWidth;
		int monBottom = maxHeight;

		int relWinX = (int)winPos.x;
		int relWinY = (int)winPos.y;

		// 在 Windows 平台下，使用 Win32 API 判定視窗實際所屬的顯示器，並取得該顯示器的原點及尺寸。為避免與 WinUser.h 的函式命名衝突
		// 我們不直接包含 <windows.h>，而改用 GetCurrentMonitor + GetMonitorWidth/GetMonitorHeight 作為較安全的跨平台作法。
		// 這樣能避免在不同 Windows SDK 與 Raylib 之間的命名衝突。
		// 若你之後確認要使用 Win32 精確 API，可以把對應邏輯移到獨立平台檔案並加上適當的宏保護。
		// （此處保留 relWinX/relWinY 為 winPos 的簡單裁切）
		relWinX = (int)winPos.x;
		relWinY = (int)winPos.y;
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
			currentWidth += grow; // 視窗變寬
			// 把整個視窗往左移 grow 像素，但確保不會移到顯示器左側以外
			int newPosX = (int)winPos.x - grow;
			if (newPosX < 0) newPosX = 0;
			SetWindowPosition(newPosX, (int)winPos.y);
			// 強迫將視窗大小同步更新
			SetWindowSize(currentWidth, currentHeight);
			// 因為視窗往左長了 grow 像素，主角在視窗內的相對座標必須右移 grow 像素
			playerPos.x += (float)grow;
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
		DrawRectangleV(playerPos, { 20, 20 }, BLUE);

		EndDrawing();
	}

	CloseWindow();
	return 0;
}
