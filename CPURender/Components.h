#pragma once

#include <iostream>
#include <string>

struct TransformComponent
{
	float x = 0.0f, y = 0.0f, z = 0.0f;
};

struct PrintComponent
{
	std::string message = "Hello World!\n";

	void print_message()
	{
		std::cout << message;
	}
};