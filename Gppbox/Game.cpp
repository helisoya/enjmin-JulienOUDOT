
#include <imgui.h>
#include <array>
#include <vector>

#include "C.hpp"
#include "Game.hpp"

#include "HotReloadShader.hpp"
#include "Entity.h"
#include "iostream"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "math.h"

static int cols = 1280 / C::GRID_SIZE;
static int lastLine = 720 / C::GRID_SIZE - 1;

Game::Game(sf::RenderWindow * win) {
	this->win = win;
	bg = sf::RectangleShape(Vector2f((float)win->getSize().x, (float)win->getSize().y));

	bool isOk = tex.loadFromFile("res/bg_stars.png");
	if (!isOk) {
		printf("ERR : LOAD FAILED\n");
	}
	bg.setTexture(&tex);
	bg.setSize(sf::Vector2f(1280, 720));

	bgShader = new HotReloadShader("res/bg.vert", "res/bg.frag");
	
	for (int i = 0; i < 1280 / C::GRID_SIZE; ++i) 
		walls.push_back( Vector2i(i, lastLine) );

	walls.push_back(Vector2i(0, lastLine-1));
	walls.push_back(Vector2i(0, lastLine-2));
	walls.push_back(Vector2i(0, lastLine-3));

	walls.push_back(Vector2i(cols-1, lastLine - 1));
	walls.push_back(Vector2i(cols-1, lastLine - 2));
	walls.push_back(Vector2i(cols-1, lastLine - 3));

	walls.push_back(Vector2i(cols >>2, lastLine - 2));
	walls.push_back(Vector2i(cols >>2, lastLine - 3));
	walls.push_back(Vector2i(cols >>2, lastLine - 4));
	walls.push_back(Vector2i((cols >> 2) + 1, lastLine - 4));
	cacheWalls();

	player = new Entity(this, PLAYER, 0, 0);
	drone = new Entity(this, DRONE, 0, 0);
	entities.push_back(player);
	entities.push_back(drone);

	elkSpawns.push_back(Vector2i(10, 5));
	elkSpawns.push_back(Vector2i(13, 8));
	entities.push_back(new Entity(this, ELK,10,5));
	entities.push_back(new Entity(this, ELK,13,8));


	deathRayLines = sf::VertexArray(sf::LinesStrip, 2);
	deathRayLines[0].position = sf::Vector2f(0, 0);
	deathRayLines[1].position = sf::Vector2f(0, 0);
	drawDeathRay = false;

	viewSpeed = 150;
	view.setCenter(0, 0);
	view.zoom(0.5f);
	win->setView(view);

	shakesToDo = 0;
	shakeStrength = 20;

	inEditor = false;
	editorType = WALL;
}

void Game::cacheWalls()
{
	wallSprites.clear();
	for (Vector2i & w : walls) {
		sf::RectangleShape rect(Vector2f(16,16));
		rect.setPosition((float)w.x * C::GRID_SIZE, (float)w.y * C::GRID_SIZE);
		rect.setFillColor(sf::Color(0x07ff07ff));
		wallSprites.push_back(rect);
	}
}

void Game::processInput(sf::Event ev) {
	if (ev.type == sf::Event::Closed) {
		win->close();
		closing = true;
		return;
	}
	if (ev.type == sf::Event::KeyReleased) {
		
	
	}
}


static double g_time = 0.0;
static double g_tickTimer = 0.0;


void Game::pollInput(double dt) {

	// Get Gamepad Controls
	bool joystickConnected = sf::Joystick::isConnected(0);
	float joystickAxisX = joystickConnected ? sf::Joystick::getAxisPosition(0, sf::Joystick::X) : 0;
	bool joystickUp = joystickConnected ? sf::Joystick::getAxisPosition(0, sf::Joystick::Y) < 0 : false;
	bool joystickFireDeathLaser = joystickConnected ? sf::Joystick::isButtonPressed(0, 2) : false;
	bool joystickJump = joystickConnected ? sf::Joystick::isButtonPressed(0, 0) : false;
	bool joystickFireMissile = joystickConnected ? sf::Joystick::isButtonPressed(0, 1) : false;
	bool joystickFireSpray = joystickConnected ? sf::Joystick::isButtonPressed(0, 3) : false;

	float lateralSpeed = 8.0;
	float maxSpeed = 40.0;

	if (inEditor) {
		pollInputEditor(dt);
		return;
	}

	if (!player->IsAlive()) return;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || joystickAxisX < -15) {
		player->AddForce(-lateralSpeed,0);
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || joystickAxisX > 15) {
		player->AddForce(lateralSpeed, 0);
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) || joystickUp) {
		upPressed = true;
	}
	else {
		upPressed = false;
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::T) || joystickFireDeathLaser) {
		if (!deathRayPressed) {
			deathRayPressed = true;
			drawDeathRay = true;
			ratioDeathRay = 0.0f;
			deathRayIsOnWall = false;
		}
	}
	else {
		deathRayPressed = false;
		drawDeathRay = false;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) || joystickJump) {
		if (!wasPressed) {
			onSpacePressed();
			wasPressed = true;
		}
	}
	else {
		wasPressed = false;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Y) || joystickFireMissile) {
		if (!missilePressed) {
			entities.push_back(new Entity(this,MISSILE,player->GetCX(), player->GetCY()));
			missilePressed = true;
		}
	}
	else {
		missilePressed = false;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::U) || joystickFireSpray) {
		if (!sprayPressed) {
			player->AddForce(0, 50,true);
			addShakes(2);

			Entity* bullet;
			float angle;
			int amount = 10;
			for (int i = 0; i < amount; i++) {
				angle = i * (-180.0 / amount) * (M_PI / 180.0);
				bullet = new Entity(this, BULLET, player->GetCX(), player->GetCY());
				bullet->SetForce(
					0.5 * cos(angle) * 20 ,
					0.5 * sin(angle) * 20
				);
				entities.push_back(bullet);
			}



			sprayPressed = true;
		}
	}
	else {
		sprayPressed = false;
	}
}

void Game::pollInputEditor(double dt)
{
	Vector2i mousePos = sf::Mouse::getPosition(*win);
	
	ImVec2 imPos = ImGui::GetWindowPos();

	if (mousePos.x >= imPos.x && mousePos.x <= imPos.x + ImGui::GetWindowWidth() &&
		mousePos.y >= imPos.y && mousePos.y <= imPos.y + ImGui::GetWindowHeight()) return;

	int x = mousePos.x / C::GRID_SIZE;
	int y = mousePos.y / C::GRID_SIZE;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) editorType = WALL;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) editorType = ENNEMY;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)) editorType = PLAYERSPAWN;

	if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
		if (editorType == WALL && !isWall(x, y)) {
			std::cout << "Pushing at " << x << " " << y << std::endl;
			walls.push_back(Vector2i(x,y));
			cacheWalls();
		}else if (editorType == ELK && !isElk(x,y)) {
			elkSpawns.push_back(Vector2i(x, y));
			entities.push_back(new Entity(this, ELK, x, y));
		}
		else if (editorType == PLAYERSPAWN) {
			playerSpawn = Vector2i(x, y);
			player->SetPosition(x, y);
			drone->SetPosition(x, y);
		}
	}
	else if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
		if (editorType == WALL && isWall(x, y)) {
			std::cout << "Erasing at " << x << " " << y << std::endl;
			walls.erase(find(walls.begin(), walls.end(), Vector2i(x, y)));
			cacheWalls();
		}
		else if (editorType == ELK && isElk(x, y)) {
			elkSpawns.erase(find(elkSpawns.begin(), elkSpawns.end(), Vector2i(x, y)));
		}
	}
}

static sf::VertexArray va;
static RenderStates vaRs;
static std::vector<sf::RectangleShape> rects;

int blendModeIndex(sf::BlendMode bm) {
	if (bm == sf::BlendAlpha) return 0;
	if (bm == sf::BlendAdd) return 1;
	if (bm == sf::BlendNone) return 2;
	if (bm == sf::BlendMultiply) return 3;
	return 4;
};

void Game::update(double dt) {
	pollInput(dt);

	g_time += dt;
	if (bgShader) bgShader->update(dt);

	beforeParts.update(dt);
	afterParts.update(dt);

	std::vector<int> idxToDestroy;
	Entity* entity;


	if (!inEditor) {
		for (int i = 0; i < entities.size(); i++) {
			entity = entities[i];
			if (!entity->IsAlive()) {
				std::cout << i << " dead" << std::endl;
				idxToDestroy.push_back(i);
				continue;
			}

			entity->Update(dt);
			if (entity->GetType() == ELK && entity->CollidesWith(player->GetSprite())) {
				player->Kill();
			}
		}

		for (int i = idxToDestroy.size() - 1; i >= 0; i--) {
			entity = entities[i];
			entities.erase(entities.begin() + idxToDestroy.at(i));
			if (entity != player && entity != drone) delete entity;
		}
		updateDeathLaser(dt);
	}


	updateCameraPosition(dt);

	


	for (Entity* entity : entitiesToAddAfterUpdate) {
		entities.push_back(entity);
	}
	entitiesToAddAfterUpdate.clear();

}

void Game::updateDeathLaser(double dt)
{
	// The death laser is now some sort of web slinger
	// If it touches a wall, you can be propulsed towards it

	if (!drawDeathRay) return;

	if (ratioDeathRay < 1) ratioDeathRay += dt * 4;
	if (ratioDeathRay > 1) ratioDeathRay = 1;

	int raySize = 15;

	bool flip = player->GetDirection() < 0;

	float playerX = player->GetCX();
	float playerY = player->GetCY();
	float potentialX = playerX + (flip ? -raySize : raySize) * (upPressed ? 0.5f : 1.0f);
	float potentialY = playerY + (upPressed ? - 15 : 0);

	sf::Vector2f finalPosition = deathRayIsOnWall ? deathRayWallPosition : bresenham(playerX, potentialX, playerY, potentialY);

	float pX = player->GetX() + 0.5f * C::GRID_SIZE;
	float pY = player->GetY() + 0.5f * C::GRID_SIZE;
	float mX = (finalPosition.x + 0.5f) * C::GRID_SIZE;
	float mY = (finalPosition.y + 0.5f) * C::GRID_SIZE;
 
	deathRayLines[flip ? 1 : 0].position = sf::Vector2f(pX, pY);
	deathRayLines[flip ? 0 : 1].position = sf::Vector2f(pX + (mX - pX) * ratioDeathRay, pY + (mY - pY) * ratioDeathRay);

	// Check for collision with entities (Only the tip of the web)
	int ratioX = playerX + (finalPosition.x - playerX) * ratioDeathRay;
	int ratioY = playerY + (finalPosition.y - playerY) * ratioDeathRay;
	for (Entity* entity : entities) {
		if (entity->GetType() == ELK && entity->GetCX() == ratioX && entity->GetCY() == ratioY) {
			entity->Kill();
		}
	}

	// Behaviour when string is at max
	if (ratioDeathRay >= 1.0f) {
		if (!deathRayIsOnWall) {
			drawDeathRay = false;
		}
		else {
			float dist = std::sqrt((mX - pX) * (mX - pX) + (mY - pY) * (mY - pY));
			if (dist > C::GRID_SIZE * 6) {
				player->AddForce((mX - pX) / 0.5f, (mY - pY) * 0.1f);
			}
		}
	}
}

void Game::updateCameraPosition(double dt)
{
	double px = player->GetX();
	double py = player->GetY();
	double dx = viewPosition.x < px ? 1 : -1;
	double dy = viewPosition.y < py ? 1 : -1;

	double moveAmount = dt * viewSpeed;


	if (abs(px - viewPosition.x) <= moveAmount) viewPosition.x = px;
	else viewPosition.x += moveAmount * dx;

	if (abs(py - viewPosition.y) <= moveAmount) viewPosition.y = py;
	else viewPosition.y += moveAmount * dy;

	if (shakesToDo > 0 && shakeCooldown <= 0) {
		shakesToDo--;
		shakeCooldown = 0.05;

		viewPosition.x += (rand() % shakeStrength) - shakeStrength/2;
		viewPosition.y += (rand() % shakeStrength) - shakeStrength/2;
	}
	else {
		shakeCooldown -= dt;
	}

	view.setCenter(viewPosition);
}

void Game::addShakes(int amount)
{
	shakesToDo += amount;
}



sf::Vector2f Game::bresenham(int x0, int x1, int y0, int y1)
{

	int dx = std::abs(x1 - x0);
	int sx = x0 < x1 ? 1 : -1;
	int dy = -std::abs(y1 - y0);
	int sy = y0 < y1 ? 1 : -1;
	int error = dx + dy;
	int e2;

	for (int i = 0; i < 150; i++) {
		if (isWall(x0, y0)) {
			deathRayIsOnWall = true;
			deathRayWallPosition = sf::Vector2f(x0, y0);
			break;
		}

		e2 = 2 * error;
		if (e2 >= dy) {
			if (x0 == x1) break;
			error = error + dy;
			x0 = x0 + sx;
		}
		if (e2 <= dx) {
			if (y0 == y1) break;
			error = error + dx;
			y0 = y0 + sy;
		}
	}

	return sf::Vector2f(x0,y0);
}

 void Game::draw(sf::RenderWindow & win) {
	if (closing) return;
	
	if(!debugSeeAll) win.setView(view);

	sf::RenderStates states = sf::RenderStates::Default;
	sf::Shader * sh = &bgShader->sh;
	states.blendMode = sf::BlendAdd;
	states.shader = sh;
	states.texture = &tex;
	sh->setUniform("texture", tex);
	//sh->setUniform("time", g_time);
	win.draw(bg, states);

	beforeParts.draw(win);

	
	for (sf::RectangleShape & r : wallSprites)
		win.draw(r);

	for (sf::RectangleShape& r : rects) 
		win.draw(r);

	for (Entity* entity : entities) {
		entity->Draw(win);
	}

	if (inEditor) {
		for (Vector2i& spawner : elkSpawns) {
			sf::RectangleShape spawnRects(Vector2f(16, 16));
			spawnRects.setPosition((float)spawner.x * C::GRID_SIZE, (float)spawner.y * C::GRID_SIZE);
			spawnRects.setFillColor(sf::Color(255,255,255,255));
			win.draw(spawnRects);
		}
	}

	if(drawDeathRay) win.draw(deathRayLines);

	afterParts.draw(win);

	win.setView(win.getDefaultView());
}

 void Game::ResetMap()
 {
	 player->Reset();
	 player->SetPosition(playerSpawn.x,playerSpawn.y);
	 drone->SetPosition(playerSpawn.x, playerSpawn.y);
	 viewPosition = sf::Vector2f(playerSpawn.x * C::GRID_SIZE,playerSpawn.y * C::GRID_SIZE);

	 for (int i = entities.size() - 1; i >= 0; i--) {
		 Entity* entity = entities[i];
		 entities.pop_back();
		 if (entity != player && entity != drone) delete entity;
	 }

	 entities.clear();
	 entities.push_back(player);
	 entities.push_back(drone);
	 for (Vector2i spawner : elkSpawns) {
		 entities.push_back(new Entity(this, ELK, spawner.x, spawner.y));
	 }
 }

void Game::onSpacePressed() {
	player->Jump(7);
}


bool Game::isEnnemySpawner(int cx, int cy)
{
	for (Vector2i& w : elkSpawns) {
		if (w.x == cx && w.y == cy)
			return true;
	}
	return false;
}

bool Game::isWall(int cx, int cy)
{
	for (Vector2i & w : walls) {
		if (w.x == cx && w.y == cy)
			return true;
	}
	return false;
}

bool Game::isElk(int cx, int cy)
{
	for (Vector2i& e : elkSpawns) {
		if (e.x == cx && e.y == cy)
			return true;
	}
	return false;
}

void Game::im()
{
	using namespace ImGui;
	int hre = 0;

	if (Button("Save")) {
		ofstream saveFile;
		saveFile.open("save.txt");
		for (Vector2i wall : walls) {
			saveFile << wall.x << ";" << wall.y << ";" << "W" << "\n";
		}
		for (Vector2i elk : elkSpawns) {
			saveFile << elk.x << ";" << elk.y << ";" << "E" << "\n";
		}
		saveFile << playerSpawn.x << ";" << playerSpawn.y << ";" << "P" << "\n";
		saveFile.close();
	}

	if (Button("Load")) {
		walls.clear();
		elkSpawns.clear();


		for (int i = entities.size() - 1; i >= 0; i--) {
			Entity* entity = entities[i];
			entities.pop_back();
			if (entity != player && entity != drone) delete entity;
		}

		entities.push_back(player);
		entities.push_back(drone);
		player->Reset();

		std::string line;

		ifstream saveFile;
		saveFile.open("save.txt");
		while (getline(saveFile, line))
		{
			stringstream ss(line);
			string t;
			char del = ',';

			getline(ss, t, ';');
			int x = atoi(t.c_str());
			getline(ss, t, ';');
			int y = atoi(t.c_str());
			getline(ss, t, ';');
			char type = t.c_str()[0];

			if (type == 'W') {
				walls.push_back(Vector2i(x,y));
			}
			else if (type == 'E') {
				elkSpawns.push_back(Vector2i(x, y));
				entities.push_back(new Entity(this, ELK, x, y));
			}
			else if (type == 'P') {
				playerSpawn = Vector2i(x, y);
				player->SetPosition(x, y);
				drone->SetPosition(x, y);
				viewPosition = sf::Vector2f(playerSpawn.x * C::GRID_SIZE, playerSpawn.y * C::GRID_SIZE);
			}
		}
		saveFile.close();

		cacheWalls();
	}

	if (Button("Reset Map")) {
		ResetMap();
	}

	if (Button("Debug (See All)")) {
		debugSeeAll = !debugSeeAll;
	}

	if (Button("Editor")) {
		inEditor = !inEditor;
		debugSeeAll = inEditor;
	}
}

std::vector<Entity*>& Game::getEntities()
{
	return entities;
}

Entity* Game::getPlayer()
{
	return player;
}

