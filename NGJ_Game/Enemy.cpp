#include "Enemy.h"
#include "raylib.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>

// Enemy.cpp
// 本檔案實作 EnemyManager 的所有方法
// 註解使用繁體中文，說明 AI 行為、生成邏輯與 Boss 縮窗機制

// 檔案等級的縮窗計時器 (假設專案僅有一個 EnemyManager 實例使用)
static float s_bossShrinkAccumulator = 0.0f;

EnemyManager::EnemyManager() {
	// 初始化 Boss 預設屬性
	bossPos = { 200.0f, 200.0f };
	bossSpeed = 40.0f;
	bossHp = 100;
	bossMaxHp = 100;
	isBossSpawned = false;

	// 初始化隨機種子（只需一次）
	std::srand((unsigned)std::time(nullptr));
}

EnemyManager::~EnemyManager() {
	// 向量會自動管理記憶，這裡僅清空內容
	activeEnemies.clear();
	bossCores.clear();
}

// 在目前視窗邊界外隨機生成一隻怪物
// level: 用於調整怪物強度
// playerPos: 可作為生成參考（此處沒直接使用）
void EnemyManager::SpawnEnemy(int level, Vector2 playerPos) {
	// 取得目前視窗大小，確保怪物出現在畫面外
	int w = GetScreenWidth();
	int h = GetScreenHeight();

	Enemy e;
	e.active = true;
	e.hp = 1 + (level / 2);            // 隨關卡提升生命
	e.speed = 40.0f + level * 5.0f;     // 隨關卡提升速度

	// 根據機率指定怪物類型
	int r = std::rand() % 100;
	if (r < 60) e.type = 1;      // 60%：直線追擊玩家
	else if (r < 90) e.type = 2; // 30%：包抄型（吃豆人式）
	else e.type = 3;             // 10%：特殊/遠程型

	// 隨機選邊並放到視窗外的座標
	int side = std::rand() % 4; // 0:left,1:right,2:top,3:bottom
	switch (side) {
	case 0: // 左側
		e.pos.x = -30.0f - (std::rand() % 80);
		e.pos.y = (float)(std::rand() % h);
		break;
	case 1: // 右側
		e.pos.x = (float)w + 30.0f + (std::rand() % 80);
		e.pos.y = (float)(std::rand() % h);
		break;
	case 2: // 上方
		e.pos.x = (float)(std::rand() % w);
		e.pos.y = -30.0f - (std::rand() % 80);
		break;
	case 3: // 下方
		e.pos.x = (float)(std::rand() % w);
		e.pos.y = (float)h + 30.0f + (std::rand() % 80);
		break;
	}

	activeEnemies.push_back(e);
}

// 更新怪物與 Boss 的邏輯
// - playerPos: 玩家當前位置
// - winWidth/winHeight: 傳入目前視窗寬高 (可被 Boss 戰時強制縮小)
// - dt: delta time
void EnemyManager::UpdateEnemiesAndBoss(Vector2 playerPos, int& winWidth, int& winHeight, float dt) {
	// 檔案內部自動生怪與自動召喚 Boss 的計時器（把責任從 main 移入此處）
	static float s_spawnAccumulator = 0.0f;
	static float s_playTime = 0.0f;
	const float s_spawnInterval = 2.0f;    // 每 2 秒產生一隻怪
	const float s_bossSpawnTime = 30.0f;    // 30 秒後自動產生 Boss
	const size_t s_maxEnemies = 32;        // 場上最多怪物數量上限

	// 累積時間並處理自動生怪
	s_playTime += dt;
	s_spawnAccumulator += dt;
	if (s_spawnAccumulator >= s_spawnInterval) {
		// 如果尚未到達上限則生成
		if (activeEnemies.size() < s_maxEnemies) {
			int level = 1 + (int)(s_playTime / 10.0f);
			SpawnEnemy(level, playerPos);
		}
		s_spawnAccumulator -= s_spawnInterval;
	}

	// 自動召喚 Boss（若尚未出現且遊戲時間到）
	if (!isBossSpawned && s_playTime >= s_bossSpawnTime) {
		isBossSpawned = true;
		bossPos = { (float)winWidth / 2.0f, (float)winHeight / 2.0f };
		bossHp = bossMaxHp;
		// 初始化四個保護核心
		bossCores.clear();
		BossCore c1; c1.pos = { 60.0f, 60.0f }; c1.hp = 3; c1.active = true; bossCores.push_back(c1);
		BossCore c2; c2.pos = { (float)winWidth - 80.0f, 60.0f }; c2.hp = 3; c2.active = true; bossCores.push_back(c2);
		BossCore c3; c3.pos = { 60.0f, (float)winHeight - 80.0f }; c3.hp = 3; c3.active = true; bossCores.push_back(c3);
		BossCore c4; c4.pos = { (float)winWidth - 80.0f, (float)winHeight - 80.0f }; c4.hp = 3; c4.active = true; bossCores.push_back(c4);
	}

	// 保存上一次玩家位置以計算移動方向
	static Vector2 lastPlayerPos = playerPos;

	// 計算玩家移動方向向量
	Vector2 playerMoveDir = { playerPos.x - lastPlayerPos.x, playerPos.y - lastPlayerPos.y };
	float moveLen = sqrtf(playerMoveDir.x * playerMoveDir.x + playerMoveDir.y * playerMoveDir.y);
	if (moveLen > 0.0001f) {
		playerMoveDir.x /= moveLen;
		playerMoveDir.y /= moveLen;
	} else {
		// 玩家靜止時視為無方向
		playerMoveDir = { 0.0f, 0.0f };
	}

	// 遍歷每隻怪物並根據 type 更新位置
	for (auto &e : activeEnemies) {
		if (!e.active) continue;

		if (e.type == 1) {
			// type 1：直接朝向玩家前進（簡單直追）
			Vector2 dir = { playerPos.x - e.pos.x, playerPos.y - e.pos.y };
			float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
			if (len > 0.001f) {
				dir.x /= len; dir.y /= len;
				e.pos.x += dir.x * e.speed * dt;
				e.pos.y += dir.y * e.speed * dt;
			}
		}
		else if (e.type == 2) {
			// type 2：吃豆人式包抄怪
			// 計算玩家目前移動方向，目標設為玩家前方 3 格的位置
			// 這裡假設一格為 20 像素，三格即 60 像素
			const float tileSize = 20.0f;
			const float aheadDist = tileSize * 3.0f;

			Vector2 interceptTarget;
			if (playerMoveDir.x == 0.0f && playerMoveDir.y == 0.0f) {
				// 玩家靜止時退回直追行為，目標為玩家本身
				interceptTarget = playerPos;
			} else {
				// 目標為玩家前方若干距離的預測位置
				interceptTarget = { playerPos.x + playerMoveDir.x * aheadDist,
									playerPos.y + playerMoveDir.y * aheadDist };
			}

			Vector2 dir = { interceptTarget.x - e.pos.x, interceptTarget.y - e.pos.y };
			float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
			if (len > 0.001f) {
				dir.x /= len; dir.y /= len;
				// 包抄怪速度略高以便形成包夾威脅
				float moveSpeed = e.speed * 1.05f;
				e.pos.x += dir.x * moveSpeed * dt;
				e.pos.y += dir.y * moveSpeed * dt;
			}
		}
		else {
			// type 3：遠程或特殊怪
			// 實作為緩慢靠近 + 微幅擺動
			Vector2 dir = { playerPos.x - e.pos.x, playerPos.y - e.pos.y };
			float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
			if (len > 0.001f) {
				dir.x /= len; dir.y /= len;
				float sway = sinf((float)GetTime() * 3.0f + (e.pos.x + e.pos.y) * 0.01f) * 10.0f;
				e.pos.x += (dir.x * e.speed + sway * 0.1f) * dt;
				e.pos.y += (dir.y * e.speed + sway * 0.1f) * dt;
			}
		}
	}

	// 如果 Boss 出現，更新 Boss 行為並處理視窗每秒自動縮小的機制
	if (isBossSpawned) {
		// 簡單追蹤玩家的 Boss 移動
		Vector2 bdir = { playerPos.x - bossPos.x, playerPos.y - bossPos.y };
		float blen = sqrtf(bdir.x * bdir.x + bdir.y * bdir.y);
		if (blen > 0.001f) {
			bdir.x /= blen; bdir.y /= blen;
			bossPos.x += bdir.x * bossSpeed * dt;
			bossPos.y += bdir.y * bossSpeed * dt;
		}

		// 每秒強制縮小視窗 5 像素的邏輯
		s_bossShrinkAccumulator += dt;
		if (s_bossShrinkAccumulator >= 1.0f) {
			int times = (int)floorf(s_bossShrinkAccumulator); // 已累積的完整秒數
			int shrinkAmount = times * 5; // 每秒縮 5 像素

			// 保護最小尺寸，避免縮到看不見
			const int MIN_W = 200;
			const int MIN_H = 200;

			// 計算新的尺寸
			int oldW = winWidth;
			int oldH = winHeight;
			int newW = std::max(MIN_W, winWidth - shrinkAmount);
			int newH = std::max(MIN_H, winHeight - shrinkAmount);

			// 以視窗中心為基準做縮放：先計算中心點，改變尺寸後把視窗位置設回使中心不變
			Vector2 winPos = GetWindowPosition();
			float centerX = winPos.x + oldW * 0.5f;
			float centerY = winPos.y + oldH * 0.5f;

			int newPosX = (int)roundf(centerX - newW * 0.5f);
			int newPosY = (int)roundf(centerY - newH * 0.5f);

			// 夾到螢幕範圍內，避免被移出螢幕
			int monitor = GetCurrentMonitor();
			int monitorW = GetMonitorWidth(monitor);
			int monitorH = GetMonitorHeight(monitor);
			if (newPosX < 0) newPosX = 0;
			if (newPosY < 0) newPosY = 0;
			if (newPosX + newW > monitorW) newPosX = monitorW - newW;
			if (newPosY + newH > monitorH) newPosY = monitorH - newH;

			// 套用新位置與新尺寸，確保從四個方向同時縮回
			SetWindowPosition(newPosX, newPosY);
			winWidth = newW;
			winHeight = newH;
			SetWindowSize(winWidth, winHeight);

			// 扣除已處理的秒數
			s_bossShrinkAccumulator -= (float)times;
		}
	}

	// 更新 lastPlayerPos，供下一次計算使用
	// 注意：必須在所有邏輯處理完後更新
	// 這樣包抄怪能得到當前格的移動方向預測
	lastPlayerPos = playerPos;
}

// 繪製所有小怪、Boss 與 Boss 的核心節點
void EnemyManager::DrawEnemies() {
	// 小怪繪製：不同 type 使用不同顏色與大小
	for (const auto &e : activeEnemies) {
		if (!e.active) continue;

		Color c = RED;
		Vector2 size = { 16.0f, 16.0f };
		if (e.type == 1) { c = RED; }
		else if (e.type == 2) { c = ORANGE; }
		else { c = PURPLE; size = { 20.0f, 20.0f }; }

		DrawRectangleV(e.pos, size, c);
	}

	// Boss 與其血條、保護核心
	if (isBossSpawned) {
		Vector2 bsize = { 120.0f, 80.0f };
		Vector2 drawPos = { bossPos.x - bsize.x / 2.0f, bossPos.y - bsize.y / 2.0f };
		DrawRectangleV(drawPos, bsize, DARKGRAY);

		// 繪製血條
		float hpRatio = (bossMaxHp > 0) ? (float)bossHp / (float)bossMaxHp : 0.0f;
		int barW = 100;
		int barH = 10;
		Vector2 barPos = { bossPos.x - barW / 2.0f, bossPos.y - bsize.y / 2.0f - 16.0f };
		DrawRectangle((int)barPos.x - 1, (int)barPos.y - 1, barW + 2, barH + 2, BLACK);
		DrawRectangle((int)barPos.x, (int)barPos.y, (int)(barW * hpRatio), barH, RED);
		DrawRectangleLines((int)barPos.x, (int)barPos.y, barW, barH, WHITE);

		// 繪製 Boss 的保護核心 (bossCores)
		for (const auto &core : bossCores) {
			if (!core.active) continue;
			Color cc = (core.hp > 0) ? GOLD : GRAY;
			DrawRectangleV(core.pos, { 12.0f, 12.0f }, cc);
		}
	}
}
