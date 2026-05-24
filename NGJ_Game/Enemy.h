#pragma once
#include <string>
#include <vector>
#include "Map.h"

namespace NGJ {

// 簡單 2D 向量實作（獨立於任何引擎）
struct Vec2 {
    float x;
    float y;
    Vec2() : x(0.0f), y(0.0f) {}
    Vec2(float _x, float _y) : x(_x), y(_y) {}

    static float Distance(const Vec2& a, const Vec2& b);
    Vec2 Normalized() const;

    // 明確的運算函式：避免與其他庫型別造成運算子衝突
    Vec2 Add(const Vec2& rhs) const { return Vec2(x + rhs.x, y + rhs.y); }
    Vec2 Sub(const Vec2& rhs) const { return Vec2(x - rhs.x, y - rhs.y); }
    Vec2 Mul(float scalar) const { return Vec2(x * scalar, y * scalar); }
};

// 敵人狀態列舉
enum class EnemyState {
    Idle,
    Patrol,
    Chase,
    Attack,
    Dead
};

struct EnemyBullet {
    Vec2 position;
    Vec2 velocity;
    bool active;
};

// 可擴充的 Enemy 類別宣告
// 可擴充的 Enemy 類別宣告
class Enemy {
public:
    Enemy(const std::string& name_,
          int maxHP_ = 10,
          int attackPower_ = 2,
          int defense_ = 0,
          float moveSpeed_ = 60.0f,
          float detectionRange_ = 150.0f,
          float attackRange_ = 24.0f,
          float attackCooldown_ = 1.0f,
          const Vec2& startPos = Vec2(0.0f, 0.0f));

    virtual ~Enemy();

    // 每幀呼叫以更新狀態與行為
    void Update(float deltaTime, const Vec2& playerPosition, Map* map,
                const Vec2* viewMin = nullptr, const Vec2* viewMax = nullptr);

    // 受傷、攻擊、死亡等介面
    virtual void TakeDamage(int damage);
    virtual int Attack();
    virtual void Die();

    bool CanAttack() const;
    bool GetIsDead() const;
    EnemyState GetState() const;

    Vec2 GetPosition() const;
    void SetPosition(const Vec2& newPosition);

    int GetCurrentHP() const;
    int GetMaxHP() const;

    void SetRangedAttack(bool enabled, float bulletSpeed = 220.0f, float bulletCooldown = 1.2f);
    bool IsRangedAttackEnabled() const;
    const std::vector<EnemyBullet>& GetBullets() const;
    std::vector<EnemyBullet>& GetBulletsMutable();

    // Boss 第一階段控制
    void ConfigureBossPhaseOne();
    bool IsBoss() const { return isBoss; }
    bool IsBossBulletBarrageActive() const { return bossBulletBarrageActive; }
    bool IsBossSummonTriggered() const { return bossSummonTriggered; }
    bool IsBossIdleLockActive() const { return bossIdleLockTimer > 0.0f; }
    void TriggerBossBarrage();
    void TriggerBossSummon();
    void TriggerBossIdleLock(float seconds = 5.0f);
    void UpdateBoss(float deltaTime, const Vec2& playerPosition, Map* map, const Vec2* viewMin = nullptr, const Vec2* viewMax = nullptr);
    void SpawnBossBullets360();
    void ClearBossBullets();

    // 可供外部調整的屬性（繼承或管理器可直接存取）
    std::string name;
    int maxHP;
    int currentHP;
    int attackPower;
    int defense;
    float moveSpeed;
    float detectionRange;
    float attackRange;
    float attackCooldown;
    bool isDead;
    Vec2 position;
    Vec2 patrolTarget;
    bool isBoss;
    bool bossSummonTriggered;
    float bossPhaseTimer;
    float bossAttackCycleTimer;
    float bossBulletBarrageTimer;
    float bossIdleLockTimer;
    int bossBulletDamage;
    int bossMeleeDamage;
    int bossBarrageWaveCount;
    int bossBarrageWaveIndex;
    float bossBarrageWaveTimer;
    bool bossBulletBarrageActive;
    bool bossSummonPending;
    bool bossAtCenter;


protected:
    void UpdateState(const Vec2& playerPosition, Map* map);
    void UpdateIdle(float deltaTime, Map* map);
    void UpdatePatrol(float deltaTime, Map* map);
    void UpdateChase(float deltaTime, const Vec2& playerPosition, Map* map,
                     const Vec2* viewMin = nullptr, const Vec2* viewMax = nullptr);
    void UpdateAttack(float deltaTime, const Vec2& playerPosition);
    void UpdateBullets(float deltaTime, Map* map);

    void MoveTowards(const Vec2& target, float deltaTime, Map* map);
    virtual void ChooseNewPatrolTarget(Map* map);

    EnemyState state;
    float attackTimer;
    float patrolRadius;
    bool canShoot;
    float bulletSpeed;
    float bulletCooldown;
    float bulletTimer;
    std::vector<EnemyBullet> bullets;

    // Wolf 衝刺機制
    float dashCooldownTimer;
    float dashStopTimer;
    bool isDashing;
    float dashSpeed;
    Vec2 dashTarget;

    // Boss 彈幕子彈
    float bossBulletSpeed;
    float bossBulletCooldown;
    float bossBulletAngleOffset;

    // Assassin 特性：每 1~2 秒瞬移到玩家背後
    float assassinTeleportTimer;
};

} // namespace NGJ
