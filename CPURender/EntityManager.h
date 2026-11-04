#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <vector>

#include "ComponentStorage.h"

// A componentID is both the index of a storage in componentStorages and a bit number in the bitsets.
const int MAX_COMPONENTS = 64;

// Creates the entities, owns the storages, and remembers which components each entity has.
// See ARCHITECTURE.md at the root of the repository for how it fits with the rest of the engine.
//
//	EntityManager em;
//	const int transformID = em.register_component<TransformComponent>();  // once per type, at startup
//	const int cow = em.create_entity();
//	TransformComponent transform;
//	em.add_component(cow, transformID, &transform);
//	for (int entity : em.fetch_all_entities(transformID)) { /* ... */ }
class EntityManager
{
public:

	EntityManager() = default;

	~EntityManager()
	{
		for (IComponentStorage* storage : componentStorages) delete storage;
	}

	// The manager owns its storages: a copy would delete them a second time.
	EntityManager(const EntityManager&) = delete;
	EntityManager& operator=(const EntityManager&) = delete;

	// Creates the storage of a component type and returns its componentID.
	template<typename T>
	int register_component()
	{
		assert(static_cast<int>(componentStorages.size()) < MAX_COMPONENTS && "Too many component types, raise MAX_COMPONENTS");

		componentStorages.push_back(new ComponentStorage<T>());
		return static_cast<int>(componentStorages.size()) - 1;
	}

	// Reuses the id of a destroyed entity if there is one.
	int create_entity()
	{
		if (!freeEntityIDs.empty())
		{
			const int entity = freeEntityIDs.back();
			freeEntityIDs.pop_back();
			return entity;
		}

		assert(nextEntityID < MAX_ENTITIES && "Too many entities, raise MAX_ENTITIES");
		return nextEntityID++;
	}

	// Removes every component of the entity, and frees its id.
	void destroy_entity(int entity)
	{
		assert(entity >= 0 && entity < nextEntityID);
		assert(std::find(freeEntityIDs.begin(), freeEntityIDs.end(), entity) == freeEntityIDs.end() && "Entity already destroyed");

		for (int componentID = 0; componentID < static_cast<int>(componentStorages.size()); componentID++)
		{
			if (entityComponents[entity].test(componentID)) remove_component(entity, componentID);
		}

		freeEntityIDs.push_back(entity);
	}

	// The value is copied into the storage.
	void add_component(int entity, int componentID, void* value)
	{
		assert(entity >= 0 && entity < nextEntityID);
		assert(componentID >= 0 && componentID < static_cast<int>(componentStorages.size()));

		componentStorages[componentID]->add(entity, value);
		entityComponents[entity].set(componentID);
	}

	void remove_component(int entity, int componentID)
	{
		assert(entity >= 0 && entity < nextEntityID);
		assert(componentID >= 0 && componentID < static_cast<int>(componentStorages.size()));

		componentStorages[componentID]->remove(entity);
		entityComponents[entity].reset(componentID);
	}

	// The systems read and write the components through this: cast the result to the component type.
	void* get_component(int entity, int componentID)
	{
		assert(entity >= 0 && entity < nextEntityID);
		assert(componentID >= 0 && componentID < static_cast<int>(componentStorages.size()));

		return componentStorages[componentID]->get(entity);
	}

	// Every living entity that has ALL the given components, in increasing entity id.
	// A destroyed entity has an empty bitset, so it can't match: that's why the mask must not be empty.
	template<typename... IDs>
	std::vector<int> fetch_all_entities(IDs... componentIDs) const
	{
		static_assert(sizeof...(componentIDs) > 0, "fetch_all_entities needs at least one componentID");

		std::bitset<MAX_COMPONENTS> mask;
		(mask.set(static_cast<std::size_t>(componentIDs)), ...);

		std::vector<int> entities;
		for (int entity = 0; entity < nextEntityID; entity++)
		{
			if ((mask & entityComponents[entity]) == mask) entities.push_back(entity);
		}

		return entities;
	}

private:

	int nextEntityID = 0;
	std::vector<int> freeEntityIDs;
	std::array<std::bitset<MAX_COMPONENTS>, MAX_ENTITIES> entityComponents;
	std::vector<IComponentStorage*> componentStorages;
};
