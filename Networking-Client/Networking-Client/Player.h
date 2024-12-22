#pragma once
#include<SFML/Graphics.hpp>

class Player
{
public:
	Player(int _id, bool _isIt) : localID(_id), isIt(_isIt) { initShape(); }

	void render(sf::RenderWindow& _window);
	void move(sf::Vector2f _vel);
	void updatePlayerPosition(sf::Vector2f _playerPos) { playerShape.setPosition(_playerPos); }

	void invisiblePowerUp(bool _currentPlayer);

	sf::Vector2f getPosition() const { return playerShape.getPosition(); }

	void setColor();

	int localID = 0;
	bool isIt = false;
	sf::RectangleShape indicator;
	sf::Color currentColor;
private:
	void initShape();
	sf::CircleShape playerShape;
	
};

