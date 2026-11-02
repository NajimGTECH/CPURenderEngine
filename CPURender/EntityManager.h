#pragma once

#include <array>
#include <cassert>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "Components.h"
#include "ComponentStorage.h"

class EntityManager
{
public:

	// Returns the lowest free entity id: the id of a destroyed entity gets reused.
	int create_entity()
	{
		for (int entity = 0; entity < MAX_ENTITIES; entity++)
		{
			if (!m_aliveEntities[entity])
			{
				m_aliveEntities[entity] = true;
				return entity;
			}
		}

		assert(false && "Too many entities, raise MAX_ENTITIES");
		return -1;
	}

	void destroy_entity(int entity)
	{
		for (const std::unique_ptr<IComponentStorage>& storage : m_components)
		{
			if (storage->has_component(entity)) storage->remove_component(entity);
		}

		m_aliveEntities.at(entity) = false;
	}

	// Add a component of new specified type and returns the component id as int.
	template<typename ComponentType>
	int add_component_type()
	{
		m_components.push_back(std::make_unique<ComponentStorage<ComponentType>>());

		const int componentId = static_cast<int>(m_components.size()) - 1; // represents the component id (index in m_components)
		m_componentIds[std::type_index(typeid(ComponentType))] = componentId;

		return componentId;
	}

	void add_component(int entity, int componentId, const void* component)
	{
		m_components.at(componentId)->add_component(entity, component);
	}

	void* get_component(int entity, int componentId)
	{
		return m_components.at(componentId)->get_component(entity);
	}

	void remove_component(int entity, int componentId)
	{
		m_components.at(componentId)->remove_component(entity);
	}

	bool has_component(int entity, int componentId)
	{
		return m_components.at(componentId)->has_component(entity);
	}

	// Same functions, found from the component type: no id to carry around and no void* to cast.
	// The type must have been registered with add_component_type first.

	// The component is copied into its storage: returns the stored copy.
	template<typename ComponentType>
	ComponentType& add_component(int entity, const ComponentType& component)
	{
		add_component(entity, component_id<ComponentType>(), &component);
		return get_component<ComponentType>(entity);
	}

	template<typename ComponentType>
	ComponentType& get_component(int entity)
	{
		return *static_cast<ComponentType*>(get_component(entity, component_id<ComponentType>()));
	}

	template<typename ComponentType>
	void remove_component(int entity)
	{
		remove_component(entity, component_id<ComponentType>());
	}

	template<typename ComponentType>
	bool has_component(int entity)
	{
		return has_component(entity, component_id<ComponentType>());
	}

	// What the systems are built on: calls function(entity, components...) for every living entity
	// that has ALL the given components, in increasing entity id.
	template<typename... ComponentTypes, typename Function>
	void for_each(Function function)
	{
		for (int entity = 0; entity < MAX_ENTITIES; entity++)
		{
			if (m_aliveEntities[entity] && (has_component<ComponentTypes>(entity) && ...))
			{
				function(entity, get_component<ComponentTypes>(entity)...);
			}
		}
	}

private:

	template<typename ComponentType>
	int component_id() const
	{
		return m_componentIds.at(std::type_index(typeid(ComponentType)));
	}

	std::vector<std::unique_ptr<IComponentStorage>> m_components;
	std::unordered_map<std::type_index, int> m_componentIds;
	std::array<bool, MAX_ENTITIES> m_aliveEntities{};
};
