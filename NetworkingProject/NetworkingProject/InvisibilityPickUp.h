#pragma once
#include<SFML/Graphics.hpp>

class InvisibilityPickUp
{
public:
	InvisibilityPickUp(sf::Vector2f _pos) : position(_pos) { initShape(); }

	void render(sf::RenderWindow& _window);

	sf::CircleShape shape;

	sf::Vector2f position;
private:
	void initShape();
};

