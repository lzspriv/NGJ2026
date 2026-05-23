#include "Enemy.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>

// -------------------------------------------
// Enemy.cpp - Maze game enemy behaviors
// 實作 Enemy 與 Vector2 的必要方法以符合 Enemy.h 定義
// -------------------------------------------

float NGJ::Vec2::Distance(const NGJ::Vec2& a, const NGJ::Vec2& b) {
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	return std::sqrt(dx * dx + dy * dy);
}

NGJ::Vec2 NGJ::Vec2::Normalized() const {
	float len = std::sqrt(x * x + y * y);
	if (len <= 1e-6f) return NGJ::Vec2(0.0f, 0.0f);
	return NGJ::Vec2(x / len, y / len);
}

namespace {
	inline void SeedRandOnce() {
		static bool seeded = false;
		if (!seeded) {
			std::srand(static_cast<unsigned int>(std::time(nullptr)));
			seeded = true;
		}
	}
}

// -------------------------------------------
// Enemy methods
// -------------------------------------------
NGJ::Enemy::Enemy(const std::string& name_,
	int maxHP_,
	int attackPower_,
	int defense_,
	float moveSpeed_,
	float detectionRange_,
	float attackRange_,
	float attackCooldown_,
	const NGJ::Vec2& startPos)
	: name(name_),
	maxHP(maxHP_),
	currentHP(maxHP_),
	attackPower(attackPower_),
	defense(defense_),
	moveSpeed(moveSpeed_),
	detectionRange(detectionRange_),
	attackRange(attackRange_),
	attackCooldown(attackCooldown_),
	isDead(false),
	position(startPos),
	patrolTarget(startPos),
	state(NGJ::EnemyState::Idle),
	attackTimer(0.0f),
	patrolRadius(80.0f)
{
	SeedRandOnce();
}

NGJ::Enemy::~Enemy() {}

void NGJ::Enemy::Update(float deltaTime, const NGJ::Vec2& playerPosition, Map* map) {
	if (isDead) {
		state = EnemyState::Dead;
		return;
	}

	if (attackTimer > 0.0f) {
		attackTimer -= deltaTime;
		if (attackTimer < 0.0f) attackTimer = 0.0f;
	}

	UpdateState(playerPosition, map);

	switch (state) {
	case NGJ::EnemyState::Idle:
		UpdateIdle(deltaTime, map);
		break;
	case NGJ::EnemyState::Patrol:
		UpdatePatrol(deltaTime, map);
		break;
	case NGJ::EnemyState::Chase:
		UpdateChase(deltaTime, playerPosition, map);
		break;
	case NGJ::EnemyState::Attack:
		UpdateAttack(deltaTime, playerPosition);
		break;
	case NGJ::EnemyState::Dead:
		break;
	}
}

void NGJ::Enemy::TakeDamage(int damage) {
	if (isDead) return;
	int net = damage - defense;
	if (net < 1) net = 1;
	currentHP -= net;
	if (currentHP <= 0) {
		currentHP = 0;
		Die();
	}
}

int NGJ::Enemy::Attack() {
	if (!CanAttack() || isDead) return 0;
	attackTimer = attackCooldown;
	return attackPower;
}

void NGJ::Enemy::Die() {
	isDead = true;
	state = NGJ::EnemyState::Dead;
}

bool NGJ::Enemy::CanAttack() const { return (attackTimer <= 0.0f) && (!isDead); }
bool NGJ::Enemy::GetIsDead() const { return isDead; }
NGJ::EnemyState NGJ::Enemy::GetState() const { return state; }
NGJ::Vec2 NGJ::Enemy::GetPosition() const { return position; }
void NGJ::Enemy::SetPosition(const NGJ::Vec2& newPosition) { position = newPosition; }
int NGJ::Enemy::GetCurrentHP() const { return currentHP; }
int NGJ::Enemy::GetMaxHP() const { return maxHP; }

void NGJ::Enemy::UpdateState(const NGJ::Vec2& playerPosition, Map* map) {
	if (isDead) { state = NGJ::EnemyState::Dead; return; }
	float distToPlayer = NGJ::Vec2::Distance(position, playerPosition);

	if (distToPlayer <= attackRange) { state = NGJ::EnemyState::Attack; return; }

	// 【智商升級 1：仇恨記憶機制】
	// 如果怪物已經處於「追擊 (Chase)」狀態，牠的視野（偵測距離）會自動放大 3 倍！
	// 這樣玩家一旦被盯上，就很難輕易甩掉牠，除非跑得非常非常遠。
	float currentDetectRange = (state == NGJ::EnemyState::Chase) ? (detectionRange * 3.0f) : detectionRange;

	if (distToPlayer <= currentDetectRange) { state = NGJ::EnemyState::Chase; return; }

	float distToPatrol = NGJ::Vec2::Distance(position, patrolTarget);
	if (distToPatrol > 5.0f) state = NGJ::EnemyState::Patrol; else state = NGJ::EnemyState::Idle;
}

void NGJ::Enemy::UpdateIdle(float /*deltaTime*/, Map* map) {
	// 提高發呆時尋找新巡邏點的機率，讓怪物更頻繁亂晃 (120 幀大約 2 秒)
	if (std::rand() % 120 == 0) ChooseNewPatrolTarget(map);
}

void NGJ::Enemy::UpdatePatrol(float deltaTime, Map* map) {
	float dist = NGJ::Vec2::Distance(position, patrolTarget);
	if (dist <= 4.0f) ChooseNewPatrolTarget(map); else MoveTowards(patrolTarget, deltaTime, map);
}

void NGJ::Enemy::UpdateChase(float deltaTime, const NGJ::Vec2& playerPosition, Map* map) {
	MoveTowards(playerPosition, deltaTime, map);
}

void NGJ::Enemy::UpdateAttack(float /*deltaTime*/, const NGJ::Vec2& /*playerPosition*/) {
}

void NGJ::Enemy::MoveTowards(const NGJ::Vec2& target, float deltaTime, Map* map) {
	NGJ::Vec2 dir = NGJ::Vec2(target.x - position.x, target.y - position.y);
	float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);
	if (dist <= 1e-4f) return;

	NGJ::Vec2 norm = NGJ::Vec2(dir.x / dist, dir.y / dist);
	float moveDist = moveSpeed * deltaTime;
	if (moveDist >= dist) moveDist = dist;

	float r = 12.0f;           // 怪物的物理碰撞半徑
	float sideR = r * 0.7f;    // 稍微縮小側邊判定，讓怪物更容易滑進狹窄的走廊

	// 獨立檢查 X 與 Y 軸前方是否有牆壁
	bool canMoveX = true;
	if (map) {
		float checkX = position.x + norm.x * moveDist + (norm.x > 0 ? r : -r);
		if (map->IsWall(checkX, position.y - sideR) || map->IsWall(checkX, position.y + sideR)) canMoveX = false;
	}

	bool canMoveY = true;
	if (map) {
		float checkY = position.y + norm.y * moveDist + (norm.y > 0 ? r : -r);
		if (map->IsWall(position.x - sideR, checkY) || map->IsWall(position.x + sideR, checkY)) canMoveY = false;
	}

	bool movedX = false;
	bool movedY = false;

	// 基礎直線移動
	if (canMoveX) { position.x += norm.x * moveDist; movedX = true; }
	if (canMoveY) { position.y += norm.y * moveDist; movedY = true; }

	// 【智商升級 2：人工貼牆繞行演算法 (Wall Sliding)】
	if (state == NGJ::EnemyState::Chase) {
		if (!movedX && movedY) {
			// X 軸撞牆了，但 Y 軸沒撞：將原本要走 X 的動能，全部轉換到 Y 軸，實作全速貼牆滑行
			float extraY = (dir.y >= 0) ? std::abs(norm.x * moveDist) : -std::abs(norm.x * moveDist);
			// 如果完全水平撞牆，隨機決定往上繞還是往下繞
			if (dir.y == 0) extraY = (std::rand() % 2 == 0) ? std::abs(norm.x * moveDist) : -std::abs(norm.x * moveDist);

			float checkY = position.y + extraY + (extraY > 0 ? r : -r);
			if (map && !map->IsWall(position.x - sideR, checkY) && !map->IsWall(position.x + sideR, checkY)) {
				position.y += extraY;
			}
		}
		else if (movedX && !movedY) {
			// Y 軸撞牆了，但 X 軸沒撞：轉移速度繞路
			float extraX = (dir.x >= 0) ? std::abs(norm.y * moveDist) : -std::abs(norm.y * moveDist);
			if (dir.x == 0) extraX = (std::rand() % 2 == 0) ? std::abs(norm.y * moveDist) : -std::abs(norm.y * moveDist);

			float checkX = position.x + extraX + (extraX > 0 ? r : -r);
			if (map && !map->IsWall(checkX, position.y - sideR) && !map->IsWall(checkX, position.y + sideR)) {
				position.x += extraX;
			}
		}
	}

	// 如果是在巡邏狀態且四處碰壁，立刻重新選一個方向
	if ((!movedX || !movedY) && state == NGJ::EnemyState::Patrol) {
		ChooseNewPatrolTarget(map);
	}
}

void NGJ::Enemy::ChooseNewPatrolTarget(Map* map) {
	// 嘗試找尋一個不是牆壁的目標點 (最多嘗試 10 次)
	for (int i = 0; i < 10; i++) {
		double fracA = (double)(std::rand() % 3600) / 3600.0;
		double ang_d = std::fma(fracA, 2.0 * 3.14159265358979323846, 0.0);
		double fracR = (double)(std::rand() % 1000) / 1000.0;
		double r_d = std::fma(fracR, (double)patrolRadius, 0.0);
		float ang = (float)ang_d;
		float r = (float)r_d;
		double cosv = std::cos(ang);
		double sinv = std::sin(ang);
		float nx = (float)std::fma(cosv, r, (double)position.x);
		float ny = (float)std::fma(sinv, r, (double)position.y);

		// 確保選出的新巡邏點不在牆壁內
		if (map && !map->IsWall(nx, ny)) {
			patrolTarget = NGJ::Vec2(nx, ny);
			return;
		}
	}
	// 如果嘗試 10 次都失敗(例如被關在死胡同)，原地放棄
	patrolTarget = position;
}