#pragma once

#include <functional>
#include <vector>

#include "EntityManager.h"

// Ties the data (the EntityManager) to the behaviour (the systems).
class ECS
{
public:

    using System = std::function<void(EntityManager&)>;

    EntityManager& GetEntityManager()
    {
        return em;
    }

    void AddSystem(System s)
    {
        systems.push_back(s);
    }

    // Runs every system once, in the order they were added.
    void ExecuteSystems()
    {
        for (System& system : systems)
        {
            system(em);
        }
    }

private:

    EntityManager em;
    std::vector<System> systems;
};
