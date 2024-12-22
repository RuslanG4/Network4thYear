#pragma once
#include<SFML/Graphics.hpp>

class Player
{
	public:
		Player(int _id, bool _isIt) : localID(_id), isIt(_isIt){ initShape(); }
		
		void move(sf::Vector2f _vel);
		void render(sf::RenderWindow& _window);

		void updatePlayerPosition(sf::Vector2f _playerPos);

		void invisiblePowerUp(bool _currentPlayer);

		sf::Vector2f getPosition() const { return playerShape.getPosition(); }

		void setColor();

		int localID = 0;
		sf::CircleShape playerShape;
		sf::RectangleShape indicator;
		sf::Color currentColor;
		bool isIt = false;
	private:
		void initShape();
		
		
};

