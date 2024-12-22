#ifndef GAME_HPP
#define GAME_HPP
#include <SFML/Graphics.hpp>
#include<SFML/Audio.hpp>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#include <iostream>
#include <mutex>
#include"InvisibilityPickUp.h"
#include"Constants.h"
#include"Player.h"

enum class GameState {
	Wait,
	Playing,
	GameOver
};

#pragma pack(push, 1)
struct PacketData {
	std::string timeLasted;
	bool gameOver;
	bool restart;
	bool isIt;
	int playerID;
	int xVel;
	int yVel;
};
#pragma pack(pop)

class Game
{
public:
	Game();
	~Game();
	/// <summary>
	/// main method for game
	/// </summary>
	void run();
	bool connectToHost(const std::string& host, unsigned short port);

private:

	void processEvents();
	void update(sf::Time t_deltaTime);
	void render();

	void handleMovement();

	void sendPlayerData(sf::Vector2f vel);

	void removeLocalPeer(std::string _message); //removes a player  that has left from local

	void networkLoop();
	void receivePositions();

	void handleInvisState(std::string _message);
	void handleInvisLocation(std::string _message);

	GameState currentState = GameState::Wait;

	sf::Text gameOverText;
	sf::Font font;

	std::thread networkThread;
	std::mutex dataMutex;

	SOCKET clientSocket; //tcp socket local

	int localID = 2;

	std::atomic<bool> isRunning = false;

	std::shared_ptr<Player> currentPlayer;

	std::unique_ptr<InvisibilityPickUp> pickup;

	std::vector<std::shared_ptr<Player>> activePlayers;

	sf::RenderWindow m_window; // main SFML window
};

#endif // !GAME_HPP