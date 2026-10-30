#pragma once

#include <array>
#include <cassert>

const int MAX_ENTITIES = 100;

class IComponentStorage
{
public:
    virtual ~IComponentStorage() = default;

    virtual void add_component(int entity, const void* component) = 0;
    virtual void remove_component(int entity) = 0;
    virtual bool has_component(int entity) = 0;
    virtual void* get_component(int entity) = 0;
};

template<typename ComponentType>
class ComponentStorage : public IComponentStorage
{
public:

    void add_component(int entity, const void* component) override
    {
        assert(entity >= 0 && entity < MAX_ENTITIES);
        m_ownedComponents[entity] = true;
        m_components[entity] = *static_cast<const ComponentType*>(component);
    }

    void remove_component(int entity) override
    {
        assert(entity >= 0 && entity < MAX_ENTITIES);
        m_ownedComponents[entity] = false;
        m_components[entity] = ComponentType{};
    }

    bool has_component(int entity) override
    {
        return m_ownedComponents[entity];
    }

    void* get_component(int entity) override
    {
        // Runtime check: static_assert only works on compile-time constants.
        assert(m_ownedComponents[entity] && "Cannot get a component the entity does not own");
        return &m_components[entity];
    }

private:

    std::array<ComponentType, MAX_ENTITIES> m_components{};
    std::array<bool, MAX_ENTITIES> m_ownedComponents{};
};
