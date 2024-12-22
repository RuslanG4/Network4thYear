#include "Game.h"

/// <summary>
/// default constructor
/// setup the window properties
/// load and setup the text 
/// load and setup thne image
/// </summary>
Game::Game() :
	m_window{ sf::VideoMode{ SCREEN_WIDTH, SCREEN_HEIGHT, 32U }, "SFML Game" }
{
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		std::cerr << "WSAStartup failed with error: " << result << "\n";
		return;
	}

	for (int i = 0; i < 3; ++i) {
		availableIDs.push(i); // adding available ids
	}

	//game initialise
	if(!font.loadFromFile("ASSETS\\FONTS\\ComicNeueSansID.ttf"))
	{
		std::cout<<"error loading font" << "\n";
	}

	gameOverText.setFont(font);
	gameOverText.setCharacterSize(48U);
	gameOverText.setFillColor(sf::Color::White);
	gameOverText.setPosition(160, 60);

	pickUpTimer.restart();

}

/// <summary>
/// default destructor we didn't dynamically allocate anything
/// so we don't need to free it, but mthod needs to be here
/// </summary>
Game::~Game()
{
	WSACleanup();  // Cleanup Winsock
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
void Game::startHost()
{
	SOCKET listenerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //tcp socket
	if (listenerSocket == INVALID_SOCKET) {
		std::cerr << "Error creating socket: " << WSAGetLastError() << "\n";
		return;
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(53000); //sets port number and address

	if (bind(listenerSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Bind failed: " << WSAGetLastError() << "\n";
		return;
	}

	if (listen(listenerSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "Listen failed: " << WSAGetLastError() << "\n";
		return;
	}

	std::cout << "Server listening on port " << 53000 << "\n";  // successful bind and listen

	{
		std::lock_guard<std::mutex> lock(dataMutex); // Mutex locked

		//add host player to the active players list
		activePlayers.emplace_back(std::make_shared<Player>(assignID(),true)); //make him the seeker
		currentPlayer = activePlayers[0]; //set the local player
		currentPlayer->updatePlayerPosition(startingPositions[0]); //set his position
	}

	std::thread acceptThread(&Game::acceptClients, this, listenerSocket); //for accepting others
	acceptThread.detach();

	run(); //run game
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

		redSurvivalTime += timer.restart().asSeconds(); //time for endgame message

		handleMovement(currentPlayer); //local movement
		handleBoundary(currentPlayer); //local boundary

		collisionCheck(); //collision between players

		handlePickUp();
		if (pickUp) {
			handlePickUpCollision();
		}
		if(isInvisibile)
		{
			handlePickUpEffect();
		}
	}else
	{
		handleGameOver();
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
	if(pickUp)
	{
		pickUp->render(m_window);
	}
	if (currentState == GameState::GameOver) {
		m_window.draw(gameOverText);
	}
	m_window.draw(currentPlayer->indicator);
	m_window.display();
}


/// <summary>
/// checks if there are any availabale id's for joining players and assigns them out
/// </summary>
/// <returns></returns>
int Game::assignID()
{
	if (!availableIDs.empty()) {
		int assignedID = availableIDs.front();
		availableIDs.pop();
		std::cout << "Assigned ID: " << assignedID <<"\n";
		return assignedID;
	}
	else {
		std::cout << "No available IDs!" << "\n";
		return -1;
	}
}

/// <summary>
/// Adds back an ID if a player is leaving
/// </summary>
/// <param name="id"></param>
void Game::releaseID(int id)
{
	availableIDs.push(id); 
}

/// <summary>
/// removes a client based off a id of the left player
/// </summary>
void Game::sendReleasedPlayerId(int id)
{
	std::string message = "Remove : " + std::to_string(id);

	for (SOCKET client : clients) {
		send(client, message.c_str(), message.length(), 0);
	}
}


/// <summary>
/// Accepts any incoming connections if there are available ids
/// </summary>
/// <param name="listenerSocket"></param>
void Game::acceptClients(SOCKET listenerSocket)
{
	while (true) {

		if (availableIDs.empty()) {
			std::cout << "No available IDs. Waiting for an ID to be released..."<<"\n";
			std::this_thread::sleep_for(std::chrono::milliseconds(100)); //short wait before checking again
			continue; //skip accepting new connections if no IDs are available
		}

		sockaddr_in clientAddr;
		int clientAddrSize = sizeof(clientAddr);
		SOCKET clientSocket = accept(listenerSocket, (sockaddr*)&clientAddr, &clientAddrSize);

		if (clientSocket != INVALID_SOCKET) {
			std::cout << "Client connected!" << "\n"; //client joined
			{
				std::lock_guard<std::mutex> lock(dataMutex); //lock mutex

				activePlayers.emplace_back(std::make_shared<Player>(assignID(), false)); //track players
				clients.push_back(clientSocket); //add the new client

				activePlayers.back()->updatePlayerPosition(startingPositions[activePlayers.back()->localID]); //set his spawn

				std::thread clientThread(&Game::handleClient, this, clientSocket, activePlayers.size() - 1); //give thread to update
				clientThread.detach();

				send(clients.back(), reinterpret_cast<char*>(&activePlayers.back()->localID), sizeof(activePlayers.back()->localID), NULL); //send new players id to client so he can set his
				std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Short delay
				if (pickUp) {
					sendPickUpPosition(pickUp->position); //if anypickups are on the screen, send them
				}
			}
			for(auto& player : activePlayers)
			{
				sendPlayerData(player); //send any other player data to client
				std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Short delay
			}
			redSurvivalTime = 0; //start game time
		}
		else {
			std::cerr << "Accept failed: " << WSAGetLastError() << "\n";
		}
		
	}
}

/// <summary>
///  updates individual clients
/// </summary>
/// <param name="clientSocket"></param>
/// <param name="playerIndex"></param>
void Game::handleClient(SOCKET clientSocket, int playerIndex)
{
	while (true) {
		char data[256];
		int received = recv(clientSocket, data, sizeof(data), 0);

		if (received > 0) {
			if (received == sizeof(PacketData)) {
				PacketData* position = reinterpret_cast<PacketData*>(data);

				if (playerIndex < activePlayers.size() && activePlayers[playerIndex]) { //only update if there is an active player with an id
					activePlayers[playerIndex]->move(sf::Vector2f(position->xVel, position->yVel)); // Move players
					handleBoundary(activePlayers[playerIndex]); // Handle boundary
					sendPlayerData(activePlayers[playerIndex]);
				}
			}
		}
		else if (received == 0) {
			std::cout << "Client disconnected." << "\n";
			break;
		}
		else {
			std::cerr << "Recv failed: " << WSAGetLastError() << "\n";
			releaseID(activePlayers[playerIndex]->localID); //player is gone so re-add his id
			break;
		}
	}

	if (playerIndex >= 0 && playerIndex < activePlayers.size()) {
		std::cout << "Removing player ID: " << activePlayers[playerIndex]->localID << "\n";
		activePlayers.erase(activePlayers.begin() + playerIndex); //erase from local storage
	}

	closesocket(clientSocket);  //close the client socket when done

	clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());

	sendReleasedPlayerId(playerIndex);

}

/// <summary>
/// moves player based on keyboard input
/// </summary>
/// <param name="_player"></param>
void Game::handleMovement(const std::shared_ptr<Player>& _player)
{
	if (_player->localID == localID) //only move local player
	{
		float dx = 0, dy = 0;
		if (m_window.hasFocus()) {
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) dy -= 1;
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) dy += 1;
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) dx -= 1;
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) dx += 1;
		}
		_player->move(sf::Vector2f(dx, dy));
		sendPlayerData(_player); //send out new data
	}
}

/// <summary>
/// sends out players data and game data
/// </summary>
/// <param name="_player"></param>
void Game::sendPlayerData(const std::shared_ptr<Player>& _player)
{
	std::lock_guard<std::mutex> lock(dataMutex);

	PacketData packet;
	packet.restart = false; 
	packet.gameOver = false; 
	packet.isIt = _player->isIt; //if player is on
	packet.playerID = _player->localID;
	packet.xVel = static_cast<int>(_player->getPosition().x);
	packet.yVel = static_cast<int>(_player->getPosition().y);

	for (SOCKET client : clients) {
		send(client, reinterpret_cast<char*>(&packet), sizeof(packet), NULL);
	}
}

/// <summary>
/// Sends game over prompt to players to toggle screen
/// </summary>
/// <param name="_survivalTime"></param>
void Game::sendGameOverToPeers(float _survivalTime)
{
	PacketData packet;
	packet.restart = false;
	packet.timeLasted = std::format("{:.2f}", _survivalTime);
	packet.gameOver = true;

	gameOverText.setString("Game Over! Red lasted " + packet.timeLasted + " seconds"); //local string

	for (SOCKET client : clients) {
		send(client, reinterpret_cast<char*>(&packet), sizeof(packet), NULL);
	}
}

/// <summary>
/// restart all base player values 
/// </summary>
/// <param name="_player"></param>
void Game::sendRestartToPeer(const std::shared_ptr<Player>& _player)
{
	PacketData packet;
	packet.restart = true;
	packet.gameOver = false;
	packet.isIt = _player->isIt;
	packet.playerID = _player->localID;
	packet.xVel = static_cast<int>(_player->getPosition().x);
	packet.yVel = static_cast<int>(_player->getPosition().y);

	for (SOCKET client : clients) {
		send(client, reinterpret_cast<char*>(&packet), sizeof(packet), NULL);
	}
}

/// <summary>
/// keeps players inside the screen
/// </summary>
/// <param name="player"></param>
void Game::handleBoundary(std::shared_ptr<Player>& player)
{
	if(player->getPosition().x < -60)
	{
		player->updatePlayerPosition(sf::Vector2f(SCREEN_WIDTH + 60, player->getPosition().y));
		sendPlayerData(player);
	}
	else if(player->getPosition().x > SCREEN_WIDTH + 60)
	{
		player->updatePlayerPosition(sf::Vector2f(0 - 60, player->getPosition().y));
		sendPlayerData(player);
	}
	else if (player->getPosition().y < -60)
	{
		player->updatePlayerPosition(sf::Vector2f(player->getPosition().x, SCREEN_HEIGHT + 60));
		sendPlayerData(player);
	}
	else if(player->getPosition().y > SCREEN_HEIGHT + 60)
	{
		player->updatePlayerPosition(sf::Vector2f(player->getPosition().x, - 60));
		sendPlayerData(player);
	}
}

/// <summary>
/// checks collisions between any active players
/// </summary>
void Game::collisionCheck()
{
	for(auto& checkingPlayer : activePlayers) //pick out start player
	{
		for(auto& otherPlayer : activePlayers)
		{
			if(checkingPlayer != otherPlayer && checkingPlayer->isIt) //check all other players
			{
				if(checkingPlayer->playerShape.getGlobalBounds().intersects(otherPlayer->playerShape.getGlobalBounds()))
				{
					std::cout << "Collision" << "\n";
					currentState = GameState::GameOver;//end game
					for(auto& player : activePlayers)
					{
						sendPickUpData(player, false); //reset pickups
						player->setColor(); //make player visibile if he wasnt
					}
					sendGameOverToPeers(redSurvivalTime); //update peers
				}
			}
		}
	}
}

/// <summary>
/// Freezes game at game oveer to give a break
/// </summary>
void Game::handleGameOver()
{
	sf::Clock endGameWait;

	if (currentState == GameState::GameOver && endGameWait.getElapsedTime().asSeconds() == 0.0f) {
		endGameWait.restart();
	}

	while(endGameWait.getElapsedTime().asSeconds() <= 3.0f) {
	}
	resetGame();
}

/// <summary>
/// restarts the game from the beginning
/// </summary>
void Game::resetGame()
{
	int randomIt = rand() % activePlayers.size(); //pick a random person to be 'IT'
	for(int i =0; i< activePlayers.size(); i++)
	{
		activePlayers[i]->updatePlayerPosition(startingPositions[i]);
		if(activePlayers[i]->localID == randomIt)
		{
			activePlayers[i]->isIt = true;
		}else
		{
			activePlayers[i]->isIt = false;
		}
		activePlayers[i]->setColor();
		sendRestartToPeer(activePlayers[i]);
	}
	redSurvivalTime = 0;
	timer.restart();

	isInvisibile = false;
	pickUpTimer.restart();
	pickUp.reset();

	currentState = GameState::Playing;
}
/// <summary>
/// spawns pick up based on timer and if ther is no other pckup
/// </summary>
void Game::handlePickUp()
{
	if(pickUp == nullptr && !isInvisibile)
	{
		if(pickUpTimer.getElapsedTime().asSeconds() > 3.f)
		{
			int xPox = rand() % (SCREEN_WIDTH - 200) + 100; //keep within screen
			int yPox = rand() % (SCREEN_HEIGHT - 200) + 100;
			pickUp = std::make_unique<InvisibilityPickUp>(sf::Vector2f(xPox, yPox));
			sendPickUpPosition(sf::Vector2f(xPox, yPox)); //send out pickup
		}

	}
}

/// <summary>
/// checks if any active players have colided with the pick up and handles their effects
/// </summary>
void Game::handlePickUpCollision()
{
	for(auto& player : activePlayers)
	{
		if (pickUp)
		{
			if (pickUp->shape.getGlobalBounds().intersects(player->playerShape.getGlobalBounds()))
			{
				isInvisibile = true;
				invisibilityTimer.restart();
				if(player == currentPlayer) //if local player collides, update his condition
				{
					player->invisiblePowerUp(true);
				}else
				{
					player->invisiblePowerUp(false);
				}
				sendPickUpData(player, false); //send out data
				pickUp.reset(); //delete pick up
			}
		}
		
	}
}


/// <summary>
/// reset invisibility after 1.5 seconds to reappera on screen
/// </summary>
void Game::handlePickUpEffect()
{
	if(invisibilityTimer.getElapsedTime().asSeconds() > 1.5f)
	{
		for(auto& player : activePlayers)
		{
			player->setColor();
			sendPickUpData(player, true);
		}
		isInvisibile = false;
		pickUpTimer.restart();
	}
	
}

/// <summary>
/// sends out invisibility data for certain player 
/// </summary>
/// <param name="_player">specific player</param>
/// <param name="_reset">to see if i cahnge back to default color</param>
void Game::sendPickUpData(const std::shared_ptr<Player>& _player, bool _reset)
{
	InvisibilityPickUpCollected packet;
	packet.itemType = "Invis : ";
	packet.playerID = _player->localID;
	_reset ? packet.reset = true : packet.reset = false;

	std::string message = packet.itemType + std::to_string(packet.playerID) + "," + std::to_string(packet.reset);

	for (SOCKET client : clients) {
		send(client, message.c_str(), message.length(), 0);
	}
}

/// <summary>
/// send out position of pickup
/// </summary>
/// <param name="_pos"></param>
void Game::sendPickUpPosition(sf::Vector2f _pos)
{
	int x = static_cast<int>(_pos.x);
	int y = static_cast<int>(_pos.y);

	std::string message = "PickUp : " + std::to_string(x) + "," + std::to_string(y);

	for (SOCKET client : clients) {
		send(client, message.c_str(), message.length(), 0);
	}
}


