#include "HealthPool.h"

namespace
{
    int ClampMaximumHealth(int maximumHealth)
    {
        if (maximumHealth < 1)
        {
            return 1;
        }

        return maximumHealth;
    }
}

HealthPool::HealthPool(int maximumHealth)
    : currentHealth(0),
      maximumHealth(ClampMaximumHealth(maximumHealth))
{
    RestoreToFull();
}

int HealthPool::GetCurrentHealth() const
{
    return currentHealth;
}

int HealthPool::GetMaximumHealth() const
{
    return maximumHealth;
}

bool HealthPool::IsAlive() const
{
    return currentHealth > 0;
}

void HealthPool::SetMaximumHealth(int newMaximumHealth)
{
    maximumHealth = ClampMaximumHealth(newMaximumHealth);

    if (currentHealth > maximumHealth)
    {
        currentHealth = maximumHealth;
    }
}

void HealthPool::ApplyDamage(int amount)
{
    if (amount <= 0)
    {
        return;
    }

    currentHealth -= amount;

    if (currentHealth < 0)
    {
        currentHealth = 0;
    }
}

void HealthPool::Heal(int amount)
{
    if (amount <= 0)
    {
        return;
    }

    currentHealth += amount;

    if (currentHealth > maximumHealth)
    {
        currentHealth = maximumHealth;
    }
}

void HealthPool::RestoreToFull()
{
    currentHealth = maximumHealth;
}