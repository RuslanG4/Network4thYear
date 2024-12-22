#include "Player.h"

void Player::move(sf::Vector2f _vel)
{
	sf::Vector2f newPos = playerShape.getPosition() + _vel * 3.f;
	playerShape.setPosition(newPos);

	indicator.setPosition(sf::Vector2f(playerShape.getPosition().x, playerShape.getPosition().y - 40));
}

void Player::render(sf::RenderWindow& _window)
{
	_window.draw(playerShape);
}

void Player::updatePlayerPosition(sf::Vector2f _playerPos)
{
	playerShape.setPosition(_playerPos);
}

void Player::invisiblePowerUp(bool _currentPlayer)
{
	if(_currentPlayer)
	{
		if(currentColor == sf::Color::Green)
		{
			playerShape.setFillColor(sf::Color(0, 255, 0, 100)); //alpha lower to indicate invis
		} else
		{
			playerShape.setFillColor(sf::Color(155, 0, 0, 100)); //alpha lower to indicate invis
		}
	}else
	{
		playerShape.setFillColor(sf::Color::Transparent); //if not current player make invisible
	}
}

void Player::setColor()
{
	if (!isIt) {
		currentColor = sf::Color::Green;
	}
	else
	{
		currentColor = sf::Color::Red;
	}
	playerShape.setFillColor(currentColor);
	indicator.setFillColor(currentColor);
}

void Player::initShape()
{
	playerShape.setRadius(15);
	playerShape.setOrigin(15, 15);

	indicator.setSize(sf::Vector2f(5, 20));
	indicator.setOrigin(2, 10);

	setColor();
}
