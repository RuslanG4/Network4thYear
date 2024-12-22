#ifndef GAME_HPP
#define GAME_HPP
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <SFML/Graphics.hpp>
#include<SFML/Audio.hpp>
#include <iostream>
#include <mutex>
#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include <chrono>
#include <queue>
#include <thread>
#include"Player.h"
#include"array"
#include"Constants.h"
#include"string"
#include"InvisibilityPickUp.h"

enum class GameState {
	Playing,
	GameOver
};

struct InvisibilityPickUpCollected {
	int playerID;
	std::string itemType;
	bool reset;
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

	void startHost();

private:
	void processEvents();
	void update(sf::Time t_deltaTime);
	void render();

	int assignID(); // assigns id to joining players
	void releaseID(int id); //releases in use id and puts it back in queue

	void sendReleasedPlayerId(int id); //sends id of gone player to remove from clients

	void acceptClients(SOCKET listenerSocket); //accepts incoming clients
	void handleClient(SOCKET client, int playerIndex); //handles an individual client

	void handleMovement(const std::shared_ptr<Player>& _player); //handles movement of players
	void handleBoundary(std::shared_ptr<Player>& _player); //handles player boundary

	void sendPlayerData(const std::shared_ptr<Player>& _player); //send over player data for movement
	void sendGameOverToPeers(float _survivalTime); //sends over end game
	void sendRestartToPeer(const std::shared_ptr<Player>& _player); //send over restart message to players

	void collisionCheck(); //checks collision amoung players

	void handleGameOver(); //handles game over
	void resetGame(); //resets game back to start

	//pickups
	void handlePickUp(); //spawns pickup 
	void handlePickUpCollision(); //pickup collision
	void handlePickUpEffect(); // activates the effect

	void sendPickUpData(const std::shared_ptr<Player>& _player, bool _reset); //sends over effect data to clients
	void sendPickUpPosition(sf::Vector2f _pos); //send over position of pickup

	sf::RenderWindow m_window; // main SFML window

	std::unique_ptr<InvisibilityPickUp> pickUp; //pickup

	sf::Text gameOverText;
	sf::Font font;

	std::shared_ptr<Player> currentPlayer; //ref to current local player 
	std::vector<std::shared_ptr<Player>> activePlayers; //player data
	std::mutex dataMutex; //mutex for safe transfers

	/// start positions for players
	std::array<sf::Vector2f, 3> startingPositions = { {
		sf::Vector2f(200, 400),
		sf::Vector2f(600, 400),
		sf::Vector2f(500, 200)
	} };

	WSADATA wsaData; //win socket

	std::vector<SOCKET> clients; //clients to send to 

	int localID = 0; //local player id

	sf::Clock timer;
	float redSurvivalTime = 0.0f; //end game timer

	std::queue<int> availableIDs; //available ids

	sf::Clock invisibilityTimer;
	bool isInvisibile = false;

	sf::Clock pickUpTimer;

	GameState currentState = GameState::Playing;

};

#endif // !GAME_HPP