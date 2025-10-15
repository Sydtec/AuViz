#pragma once
#include <glad/glad.h>
#include <SFML/Graphics.hpp>
#include <complex>
#include <iostream>

namespace avRender {
	extern 	bool close;
	inline std::unique_ptr<sf::RenderWindow> settingswin;
	sf::RenderWindow* init(uint32_t bands);
	void render(float fovslope, std::int32_t etm, std::vector<float>& in);
	void exit();
}