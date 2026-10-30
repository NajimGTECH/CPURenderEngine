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

private:

	std::vector<std::unique_ptr<IComponentStorage>> m_components;
};