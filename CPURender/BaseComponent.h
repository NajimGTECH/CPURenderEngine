#pragma once

#include <iostream>

struct TransformComponent
{
	float x, y, z = 0.0f;
};

struct PrintComponent
{
	std::string message = "Hello World!\n";

	void print_message()
	{
		std::cout << message;
	}
};