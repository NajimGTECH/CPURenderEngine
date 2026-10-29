#pragma once

#include <array>
#include <cassert>
#include <string>
#include <any>

const int MAX_ENTITIES = 100;

class IComponentStorage
{
public:
    virtual ~IComponentStorage() = 0;

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
        m_ownedComponents[entity] = true;
        m_components[entity] = std::any_cast<ComponentType>(component);
    }

    void remove_component(int entity) override
    {
        m_ownedComponents[entity] = false;
        m_components[entity] = nullptr;
    }

    bool has_component(int entity) override
    {
        return m_ownedComponents[entity];
    }

    void* get_component(int entity) override
    {
        //static_assert(m_ownedComponents[entity] == false, "Cannot get the component " + std::to_string(typdeid(ComponentType).name()) + " on entity: " + std::to_string(entity));
        //static_assert(m_ownedComponents[entity] == false, "Cannot get the component on entity");
        if (m_ownedComponents[entity] == false) { static_assert(true, "Cannot get the component\n"); }
        return (void*)&m_components[entity];
    }

private:

    std::array<ComponentType, MAX_ENTITIES> m_components;
    std::array<bool, MAX_ENTITIES> m_ownedComponents;
};
