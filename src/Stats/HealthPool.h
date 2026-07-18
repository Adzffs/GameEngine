#pragma once

class HealthPool
{
public:
    explicit HealthPool(int maximumHealth);

    int GetCurrentHealth() const;
    int GetMaximumHealth() const;
    bool IsAlive() const;

    void SetMaximumHealth(int newMaximumHealth);
    int ApplyDamage(int amount);
    void Heal(int amount);
    void RestoreToFull();

private:
    int currentHealth;
    int maximumHealth;
};