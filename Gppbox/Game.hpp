#pragma once

#include <vector>

#include "SFML/Graphics.hpp"
#include "SFML/System.hpp"
#include "SFML/Window.hpp"

#include "sys.hpp"

#include "Particle.hpp"
#include "ParticleMan.hpp"

using namespace sf;

class HotReloadShader;
class Entity;
class Game {
public:
	sf::RenderWindow*				win = nullptr;

	sf::RectangleShape				bg;
	HotReloadShader *				bgShader = nullptr;

	sf::Texture						tex;

	bool							closing = false;
	
	std::vector<sf::Vector2i>		walls;
	std::vector<sf::RectangleShape> wallSprites;

	ParticleMan beforeParts;
	ParticleMan afterParts;

	Entity* player;
	Entity* drone;
	Vector2i playerSpawn;
	std::vector<Entity*> entities;
	std::vector<Vector2i> elkSpawns;

	sf::View view;
	sf::Vector2f viewPosition;
	float viewSpeed;
	int shakesToDo;
	double shakeCooldown;
	int shakeStrength;

	sf::VertexArray deathRayLines;
	bool drawDeathRay;
	float ratioDeathRay;
	bool deathRayIsOnWall;
	sf::Vector2f deathRayWallPosition;

	Entity* cachedBulletToCreate;

	Game(sf::RenderWindow * win);

	void cacheWalls();

	void processInput(sf::Event ev);
	bool wasPressed = false;
	bool deathRayPressed = false;
	bool missilePressed = false;
	bool upPressed = false;
	void pollInput(double dt);
	void onSpacePressed();

	void update(double dt);
	void updateDeathLaser(double dt);
	void updateCameraPosition(double dt);
	void addShakes(int amount);

	sf::Vector2f bresenham(int x0, int x1, int y0, int y1);

	void draw(sf::RenderWindow& win);

	void ResetMap();

	bool isEnnemySpawner(int cx, int cy);
	bool isWall(int cx, int cy);
	void im();

	bool debugSeeAll;

	std::vector<Entity*>& getEntities();
	Entity* getPlayer();
};