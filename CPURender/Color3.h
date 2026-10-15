#pragma once

struct Color3
{
	Color3() :
		r(0), g(0), b(0), a(255) 
	{};

	Color3(uint8_t _r, uint8_t _g, uint8_t _b) :
		r(_r), g(_g), b(_b), a(255)
	{};

	Color3(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) :
		r(_r), g(_g), b(_b), a(_a)
	{};

	uint8_t r, g, b, a;

	uint32_t toRGB() 
	{ 
		return (static_cast<uint32_t>(a) << 24)
			| (static_cast<uint32_t>(r) << 16)
			| (static_cast<uint32_t>(g) << 8)
			| static_cast<uint32_t>(b);
	};
};
