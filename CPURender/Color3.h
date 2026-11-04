#pragma once

// An 8-bit per channel colour, the format of the textures and the framebuffer.
// (Materials, lights and particles use Vector3 colours in [0, 1] instead, to do math on them.)
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

	// The framebuffer format: ARGB8888 (0xAARRGGBB).
	uint32_t to_rgb()
	{ 
		return (static_cast<uint32_t>(a) << 24)
			| (static_cast<uint32_t>(r) << 16)
			| (static_cast<uint32_t>(g) << 8)
			| static_cast<uint32_t>(b);
	};
};
