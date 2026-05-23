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

// 如果 header 宣告了這些運算子，通常它會在 header 中實作。
// 不使用 non-member 運算子以避免與專案其他向量型別衝突

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

void NGJ::Enemy::Update(float deltaTime, const NGJ::Vec2& playerPosition) {
	if (isDead) {
		state = EnemyState::Dead;
		return;
	}

	if (attackTimer > 0.0f) {
		attackTimer -= deltaTime;
		if (attackTimer < 0.0f) attackTimer = 0.0f;
	}

	UpdateState(playerPosition);

	switch (state) {
	case NGJ::EnemyState::Idle:
		UpdateIdle(deltaTime);
		break;
	case NGJ::EnemyState::Patrol:
		UpdatePatrol(deltaTime);
		break;
	case NGJ::EnemyState::Chase:
		UpdateChase(deltaTime, playerPosition);
		break;
	case NGJ::EnemyState::Attack:
		UpdateAttack(deltaTime, playerPosition);
		break;
	case NGJ::EnemyState::Dead:
		break;
	}
}

void NGJ::Enemy::TakeDamage(int damage) {
	// Apply damage after accounting for defense; ensure at least 1 damage is applied
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

void NGJ::Enemy::UpdateState(const NGJ::Vec2& playerPosition) {
	if (isDead) { state = NGJ::EnemyState::Dead; return; }
	float distToPlayer = NGJ::Vec2::Distance(position, playerPosition);
	if (distToPlayer <= attackRange) { state = NGJ::EnemyState::Attack; return; }
	if (distToPlayer <= detectionRange) { state = NGJ::EnemyState::Chase; return; }
	float distToPatrol = NGJ::Vec2::Distance(position, patrolTarget);
	if (distToPatrol > 5.0f) state = NGJ::EnemyState::Patrol; else state = NGJ::EnemyState::Idle;
}

void NGJ::Enemy::UpdateIdle(float /*deltaTime*/) {
	if (std::rand() % 2000 == 0) ChooseNewPatrolTarget();
}

void NGJ::Enemy::UpdatePatrol(float deltaTime) {
	float dist = NGJ::Vec2::Distance(position, patrolTarget);
	if (dist <= 4.0f) ChooseNewPatrolTarget(); else MoveTowards(patrolTarget, deltaTime);
}

void NGJ::Enemy::UpdateChase(float deltaTime, const NGJ::Vec2& playerPosition) { MoveTowards(playerPosition, deltaTime); }

void NGJ::Enemy::UpdateAttack(float /*deltaTime*/, const NGJ::Vec2& /*playerPosition*/) {
	// Attack action is triggered externally via Attack()
}

void NGJ::Enemy::MoveTowards(const NGJ::Vec2& target, float deltaTime) {
	NGJ::Vec2 dir = NGJ::Vec2(target.x - position.x, target.y - position.y);
	float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);
	if (dist <= 1e-4f) return;
	NGJ::Vec2 norm = NGJ::Vec2(dir.x / dist, dir.y / dist);
	double moveDist_d = std::fma((double)moveSpeed, (double)deltaTime, 0.0);
	float moveDist = (float)moveDist_d;
	if (moveDist >= dist) {
		position = target;
	} else {
		float newX = (float)std::fma((double)norm.x, (double)moveDist, (double)position.x);
		float newY = (float)std::fma((double)norm.y, (double)moveDist, (double)position.y);
		position = NGJ::Vec2(newX, newY);
	}
}

void NGJ::Enemy::ChooseNewPatrolTarget() {
	// 計算一個 0..1 的隨機比例再轉成角度
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
	patrolTarget = NGJ::Vec2(nx, ny);
}
