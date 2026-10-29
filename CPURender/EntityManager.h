#pragma once

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
		ComponentStorage<ComponentType>* componentStorage = new ComponentStorage<ComponentType>();
		m_components.push_back(componentStorage);

		return static_cast<int>(m_components.size()); // represents the component id
	}

	void add_component(int entity, int componentId, const void* component)
	{
		m_components[componentId]->add_component(entity, component);
	}

private:

	std::vector<IComponentStorage*> m_components;
};