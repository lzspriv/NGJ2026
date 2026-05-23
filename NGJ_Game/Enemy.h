#pragma once
#include <string>

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
    void Update(float deltaTime, const Vec2& playerPosition);

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

protected:
    void UpdateState(const Vec2& playerPosition);
    void UpdateIdle(float deltaTime);
    void UpdatePatrol(float deltaTime);
    void UpdateChase(float deltaTime, const Vec2& playerPosition);
    void UpdateAttack(float deltaTime, const Vec2& playerPosition);

    void MoveTowards(const Vec2& target, float deltaTime);
    virtual void ChooseNewPatrolTarget();

    EnemyState state;
    float attackTimer;
    float patrolRadius;
};

} // namespace NGJ
