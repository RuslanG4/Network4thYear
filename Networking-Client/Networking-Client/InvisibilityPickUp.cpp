#include "InvisibilityPickUp.h"

void InvisibilityPickUp::render(sf::RenderWindow& _window)
{
	_window.draw(shape);
}

void InvisibilityPickUp::initShape()
{
	shape.setRadius(5);
	shape.setOrigin(5, 5);
	shape.setFillColor(sf::Color::Yellow);

	shape.setPosition(position);
}
