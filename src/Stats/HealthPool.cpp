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

int HealthPool::ApplyDamage(int amount)
{
    if (amount <= 0)
    {
        return 0;
    }

    int actualDamage = amount;

    if (actualDamage > currentHealth)
    {
        actualDamage = currentHealth;
    }

    currentHealth -= actualDamage;

    if (currentHealth < 0)
    {
        currentHealth = 0;
    }

    return actualDamage;
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