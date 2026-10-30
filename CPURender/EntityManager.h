#pragma once

#include <memory>
#include <vector>

#include "Components.h"
#include "ComponentStorage.h"

class EntityManager
{
public:

	// Add a component of new specified type and returns the component id as int.
	template<typename ComponentType>
	int add_component_type()
	{
		m_components.push_back(std::make_unique<ComponentStorage<ComponentType>>());

		return static_cast<int>(m_components.size()) - 1; // represents the component id (index in m_components)
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

private:

	std::vector<std::unique_ptr<IComponentStorage>> m_components;
};