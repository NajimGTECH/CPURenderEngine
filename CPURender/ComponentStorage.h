#pragma once

#include <array>
#include <cassert>

// An entity is only an id: all its data lives in the component storages, at the index of that id.
using Entity = int;

// Returned when no entity could be created or found.
constexpr Entity NO_ENTITY = -1;

// Every storage reserves room for this many entities, so raising it costs memory for every component type.
const int MAX_ENTITIES = 100;

// Lets the EntityManager keep storages of different component types in the same list.
// The price is the void*: the type is only known again in the typed storage below.
class IComponentStorage
{
public:
    virtual ~IComponentStorage() = default;

    virtual void add_component(Entity entity, const void* component) = 0;
    virtual void remove_component(Entity entity) = 0;
    virtual bool has_component(Entity entity) = 0;
    virtual void* get_component(Entity entity) = 0;
};

// All the components of one type, in an array indexed by entity id.
// m_ownedComponents says which slots are in use.
template<typename ComponentType>
class ComponentStorage : public IComponentStorage
{
public:

    void add_component(Entity entity, const void* component) override
    {
        assert(entity >= 0 && entity < MAX_ENTITIES);
        m_ownedComponents[entity] = true;
        m_components[entity] = *static_cast<const ComponentType*>(component);
    }

    void remove_component(Entity entity) override
    {
        assert(entity >= 0 && entity < MAX_ENTITIES);
        m_ownedComponents[entity] = false;
        m_components[entity] = ComponentType{};
    }

    bool has_component(Entity entity) override
    {
        return m_ownedComponents[entity];
    }

    void* get_component(Entity entity) override
    {
        // Runtime check: static_assert only works on compile-time constants.
        assert(m_ownedComponents[entity] && "Cannot get a component the entity does not own");
        return &m_components[entity];
    }

private:

    std::array<ComponentType, MAX_ENTITIES> m_components{};
    std::array<bool, MAX_ENTITIES> m_ownedComponents{};
};
