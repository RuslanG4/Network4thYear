#include "Game.h"

#include <chrono>
#include <thread>

/// <summary>
/// default constructor
/// setup the window properties
/// load and setup the text 
/// load and setup thne image
/// </summary>
Game::Game() :
	m_window{ sf::VideoMode{ SCREEN_WIDTH, SCREEN_HEIGHT, 32U }, "SFML Game" }
{
	if (!font.loadFromFile("ASSETS\\FONTS\\ComicNeueSansID.ttf"))
	{
		std::cout << "error loading font";
	}

	gameOverText.setFont(font);
	gameOverText.setCharacterSize(48U);
	gameOverText.setFillColor(sf::Color::White);
	gameOverText.setPosition(160, 60);
}

/// <summary>
/// default destructor we didn't dynamically allocate anything
/// so we don't need to free it, but mthod needs to be here
/// </summary>
Game::~Game()
{
	isRunning = false;
	if (networkThread.joinable()) {
		networkThread.join();
	}

	closesocket(clientSocket); 
	WSACleanup();
}


/// <summary>
/// main game loop
/// update 60 times per second,
/// process update as often as possible and at least 60 times per second
/// draw as often as possible but only updates are on time
/// if updates run slow then don't render frames
/// </summary>
void Game::run()
{
	isRunning = true;

	//start a thread for receiving data
	networkThread = std::thread(&Game::networkLoop, this);

	sf::Clock clock;
	sf::Time timeSinceLastUpdate = sf::Time::Zero;
	const float fps{ 60.0f };
	sf::Time timePerFrame = sf::seconds(1.0f / fps); // 60 fps

	while (m_window.isOpen())
	{
		processEvents(); // as many as possible
		timeSinceLastUpdate += clock.restart();
		while (timeSinceLastUpdate > timePerFrame)
		{
			timeSinceLastUpdate -= timePerFrame;
			processEvents(); // at least 60 fps
			update(timePerFrame); //60 fps
		}
		render(); // as many as possible
	}
}
/// <summary>
/// handle user and system events/ input
/// get key presses/ mouse moves etc. from OS
/// and user :: Don't do game update here
/// </summary>
void Game::processEvents()
{
	sf::Event newEvent;
	while (m_window.pollEvent(newEvent))
	{
		if (sf::Event::KeyPressed == newEvent.type) //user pressed a key
		{
			//processKeys(newEvent);
		}
	}
}

/// <summary>
/// Update the game world
/// </summary>
/// <param name="t_deltaTime">time interval per frame</param>
void Game::update(sf::Time t_deltaTime)
{
	if (currentState == GameState::Playing) {
		handleMovement();
	}
}

/// <summary>
/// draw the frame and then switch buffers
/// </summary>
void Game::render()
{
	m_window.clear(sf::Color::Black);
	for (auto& player : activePlayers) {
		player->render(m_window);
	}
	if(currentState == GameState::GameOver)
	{
		m_window.draw(gameOverText);
	}
	if (currentPlayer) {
		m_window.draw(currentPlayer->indicator);
	}
	if(pickup)
	{
		pickup->render(m_window);
	}
	m_window.display();
}


void Game::handleMovement()
{
	float dx = 0, dy = 0;
	if (m_window.hasFocus()) {
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) dy -=1;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) dy+=1;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) dx -=1;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) dx+=1;
	}
	if (currentPlayer) {
		currentPlayer->move(sf::Vector2f(dx, dy));
	}
	//sends data back to serer
	sendPlayerData(sf::Vector2f(dx,dy));
}


void Game::sendPlayerData(sf::Vector2f vel)
{
	std::lock_guard<std::mutex> lock(dataMutex);  //locking to prevent race condition when accessing shared resources
	PacketData packet;
	packet.xVel = static_cast<int>(vel.x);
	packet.yVel = static_cast<int>(vel.y);


	if (send(clientSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0) == SOCKET_ERROR) {
		std::cerr << "Error sending data: " << WSAGetLastError();
	}
}

/// <summary>
/// removes a local player from the vector if they have left
/// </summary>
/// <param name="_message"></param>
void Game::removeLocalPeer(std::string _message)
{
	size_t colonPos = _message.find(": ");

	int idToRemove = std::stoi(_message.substr(colonPos + 2));

	auto it = std::remove_if(activePlayers.begin(), activePlayers.end(),
		[idToRemove](const std::shared_ptr<Player>& player) {
			return player->localID == idToRemove;
		});

	if (it != activePlayers.end()) {
		activePlayers.erase(it, activePlayers.end()); // Remove the player(s)
		std::cout << "Player with ID " << idToRemove << " removed." << "\n";
	}
	else {
		std::cout << "Player with ID " << idToRemove << " not found." << "\n";
	}
}

void Game::networkLoop()
{
	receivePositions();
}

/// <summary>
/// hadnles all receives of new data
/// </summary>
void Game::receivePositions()
{
	while (isRunning) {
		char data[256]; //buffer size
		int received = recv(clientSocket, data, sizeof(data), 0);

		if (received > 0) {
			std::string message(data, received);
			if(message.starts_with("Remove"))
			{
				removeLocalPeer(message);
			}
			if(message.starts_with("Invis")) //invisibility color changes
			{
				handleInvisState(message);
			}
			if (message.starts_with("PickUp")) //pickup locations
			{
				handleInvisLocation(message);
			}
			if (received == sizeof(int)) { //assign local id
				localID = *reinterpret_cast<int*>(data);
				std::cout << "Assigned local ID: " << localID << "\n";
			}
			if (received == sizeof(PacketData)) { //updating player and game 
				PacketData* packet = reinterpret_cast<PacketData*>(data);

				std::lock_guard<std::mutex> lock(dataMutex); //lock

				if (packet->gameOver) {
					std::string temp = "Game Over! Red lasted " + packet->timeLasted + " seconds"; //update end game
					gameOverText.setString(temp);
					currentState = GameState::GameOver;
				}
				if (packet->restart) { //restart game
					currentState = GameState::Playing;
					for(auto& player : activePlayers)
					{
						if (packet->playerID == player->localID) {
							player->isIt = packet->isIt;
							player->setColor();
						}
					}
				}

				if (activePlayers.size() < 3) { //3 player limit
					auto it = std::find_if(activePlayers.begin(), activePlayers.end(),
						[packet](const std::shared_ptr<Player>& player) {
							return player->localID == packet->playerID;
						});

					if (it == activePlayers.end()) { //doesnt add play if already in local storage based on id
						activePlayers.push_back(std::make_shared<Player>(packet->playerID, packet->isIt));

						//if the added player is the local player, set it as currentPlayer
						if (packet->playerID == localID) {
							currentPlayer = activePlayers.back();
							currentState = GameState::Playing;
						}
					}
				}
				for(auto& player : activePlayers)
				{
					if(player->localID == packet->playerID)
					{
						player->updatePlayerPosition(sf::Vector2f(packet->xVel, packet->yVel));
					}
				}
			}
		}
		else if (received == 0) {
			std::cout << "Connection closed by server." << "\n";
			isRunning = false;
			break;
		}
		else {
			std::cerr << "Error receiving data: " << WSAGetLastError() << "\n";
			isRunning = false;
			break;
		}
	}
}

void Game::handleInvisState(std::string _message)
{
	size_t commaPos = _message.find(','); //find split in data

	// extract
	std::string playerIDStr = _message.substr(_message.find(": ") + 2, commaPos - (_message.find(": ") + 2));
	std::string resetColor = _message.substr(commaPos + 1); 

	int playerID = std::stoi(playerIDStr); //convert to integer
	bool reset = std::stoi(resetColor); //convert to integer

	for (auto& player : activePlayers)
	{
		if (player->localID == playerID && !reset)
		{
			if (player->localID == currentPlayer->localID)
			{
				player->invisiblePowerUp(true);
				pickup.reset();
			}
			else
			{
				player->invisiblePowerUp(false);
				pickup.reset();
			}
		}
		else
		{
			player->setColor();
			if(!reset)
			{
				pickup.reset();
			}
		}
	}
}

void Game::handleInvisLocation(std::string _message)
{
	size_t commaPos = _message.find(','); //splitter

	//extract
	std::string xStr = _message.substr(_message.find(": ") + 2, commaPos - (_message.find(": ") + 2)); //x value
	std::string yStr = _message.substr(commaPos + 1); //y value after the comma
	int xPos = std::stoi(xStr); //convert to integer
	int yPos = std::stoi(yStr); //convert to integer

	pickup = std::make_unique<InvisibilityPickUp>(sf::Vector2f(xPos, yPos)); //make pickup
}

/// <summary>
/// connect with host server
/// </summary>
/// <param name="host">local ip</param>
/// <param name="port">port number</param>
/// <returns></returns>
bool Game::connectToHost(const std::string& host, unsigned short port)
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed: " << WSAGetLastError() << "\n";
		return false;
	}

	clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET) {
		std::cerr << "Error creating socket: " << "\n";
		WSACleanup();
		return false;
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);
	serverAddr.sin_port = htons(port);

	if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Failed to connect to host: " << WSAGetLastError() << "\n";
		closesocket(clientSocket);
		WSACleanup();
		return false;
	}

	std::cout << "Connected to host: " << host << ":" << port << "\n";
	return true;
}

