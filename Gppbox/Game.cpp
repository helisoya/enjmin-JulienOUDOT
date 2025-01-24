
#include <imgui.h>
#include <array>
#include <vector>

#include "C.hpp"
#include "Game.hpp"

#include "HotReloadShader.hpp"
#include "Entity.h"
#include "iostream"

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
	entities.push_back(player);

	entities.push_back(new Entity(this, ELK,10,5));
	entities.push_back(new Entity(this, ELK,13,8));


	deathRayLines = sf::VertexArray(sf::LinesStrip, 2);
	deathRayLines[0].position = sf::Vector2f(0, 0);
	deathRayLines[1].position = sf::Vector2f(0, 0);
	drawDeathRay = false;
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

	float lateralSpeed = 8.0;
	float maxSpeed = 40.0;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
		player->AddForce(-lateralSpeed,0);
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
		player->AddForce(lateralSpeed, 0);
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
		upPressed = true;
	}
	else {
		upPressed = false;
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::T)) {
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
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
		if (!wasPressed) {
			onSpacePressed();
			wasPressed = true;
		}
	}
	else {
		wasPressed = false;
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
	for (Entity* entity : entities) {
		entity->Update(dt);
		if (entity->GetType() != PLAYER && entity->CollidesWith(player->GetSprite())) {
			std::cout << "Player should die" << std::endl;
		}
	}
	updateDeathLaser(dt);	
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
	float potentialX = playerX + (flip ? -raySize : raySize);
	float potentialY = playerY + (upPressed ? - 15 : 0);

	sf::Vector2f finalPosition = deathRayIsOnWall ? deathRayWallPosition : bresenham(playerX, potentialX, playerY, potentialY);

	float pX = player->GetX() + 0.5f * C::GRID_SIZE;
	float pY = player->GetY() + 0.5f * C::GRID_SIZE;
	float mX = (finalPosition.x + 0.5f) * C::GRID_SIZE;
	float mY = (finalPosition.y + 0.5f) * C::GRID_SIZE;
 
	deathRayLines[flip ? 1 : 0].position = sf::Vector2f(pX, pY);
	deathRayLines[flip ? 0 : 1].position = sf::Vector2f(pX + (mX - pX) * ratioDeathRay, pY + (mY - pY) * ratioDeathRay);


	// Behaviour when string is at max
	if (ratioDeathRay >= 1.0f) {
		if (!deathRayIsOnWall) {
			drawDeathRay = false;
		}
		else {
			float dist = std::sqrt((mX - pX) * (mX - pX) + (mY - pY) * (mY - pY));
			if (dist > C::GRID_SIZE * 3) {
				player->AddForce((mX - pX) / 0.5f, (mY - pY) * 0.5f);
			}
		}
	}
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
		for (Entity* entity : entities) {
			if (entity->GetType() != PLAYER && entity->GetCX() == x0 && entity->GetCY() == y0) {
				std::cout << "Kill the Elk" << std::endl;
			}
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

	/*
	int dx = x1 - x0;
	int dy = y1 - y0;
	int d = 2 * dy - dx;
	int y = y0;
	int x;

	bool flipX = x1 < x0;
	bool flipY = y1 < y0;

	for (x = x0; flipX ? x >= x1 : x <= x1; flipX ? x-- : x++) {
		if (isWall(x, y)) {
			deathRayIsOnWall = true;
			deathRayWallPosition = sf::Vector2f(x, y);
			break;
		}
		for (Entity* entity : entities) {
			if (entity->GetType() != PLAYER && entity->GetCX() == x && entity->GetCY() == y) {
				std::cout << "Kill the Elk" << std::endl;
			}
		}

		if (d > 0 && y != y1) {
			y = y + (flipY ? -1 : 1);
			d = d - 2 * dx;
		}
		d = d + 2 * dy;
	}*/

	return sf::Vector2f(x0,y0);
}

 void Game::draw(sf::RenderWindow & win) {
	if (closing) return;

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

	if(drawDeathRay) win.draw(deathRayLines);

	afterParts.draw(win);
}

void Game::onSpacePressed() {
	player->Jump(7);
}


bool Game::isWall(int cx, int cy)
{
	for (Vector2i & w : walls) {
		if (w.x == cx && w.y == cy)
			return true;
	}
	return false;
}

void Game::im()
{
	using namespace ImGui;
	int hre = 0;

	int mapSize = 50;

	if (Button("Save")) {

	}

	if (Button("Load")) {

	}

	if (TreeNode("Walls")) {
		Columns(mapSize, "mycolumns");
		Separator();

		static int selected = -1;
		for (int y = 0; y < mapSize; y++)
		{
			for (int x = 0; x < mapSize; x++) {
				

				PushID(mapSize * x + y);
				if (Button(isWall(y,x) ? "#" : " ")) {
					if (isWall(y, x)) {
						walls.erase(find(walls.begin(), walls.end(), Vector2i(y, x)));
					}
					else {
						walls.push_back(Vector2i(y, x));
					}
					cacheWalls();
				}
				PopID();
			}
			ImGui::NextColumn();
		}
		Columns(1);
		Separator();
		TreePop();
	}
	if (TreeNode("Entities")) {

	}
}

