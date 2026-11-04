#pragma once

#include <array>
#include <cassert>

// Every storage reserves room for this many entities, so raising it costs memory for every component type.
const int MAX_ENTITIES = 100;

// Lets the EntityManager keep storages of different component types in the same list.
// The price is the void*: the type is only known again in the typed storage below.
class IComponentStorage
{
public:
    virtual ~IComponentStorage() = default;

    virtual void add(int entity, void* value) = 0;
    virtual void remove(int entity) = 0;
    virtual bool has(int entity) = 0;
    virtual void* get(int entity) = 0;
};

// All the components of one type. An entity is just an int: the index in these arrays.
// owned says which slots are in use.
template<typename T>
class ComponentStorage : public IComponentStorage
{
public:

    void add(int entity, void* value) override
    {
        assert(entity >= 0 && entity < MAX_ENTITIES);
        owned[entity] = true;
        data[entity] = *static_cast<T*>(value);
    }

    void remove(int entity) override
    {
        assert(entity >= 0 && entity < MAX_ENTITIES);
        owned[entity] = false;
    }

    bool has(int entity) override
    {
        assert(entity >= 0 && entity < MAX_ENTITIES);
        return owned[entity];
    }

    void* get(int entity) override
    {
        assert(entity >= 0 && entity < MAX_ENTITIES);
        return &data[entity];
    }

private:

    std::array<T, MAX_ENTITIES> data{};
    std::array<bool, MAX_ENTITIES> owned{};
};
