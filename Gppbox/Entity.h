#pragma once

#include "SFML/Graphics.hpp"

class Game;

enum EntityType {
	PLAYER,
	ELK
};

class Entity
{
public:

private:
	
	Game* game;

	EntityType type;

	sf::Sprite sprite;
	sf::Texture texture;

	sf::Vector2u collisionSize;
	bool canJump;

	// Base coordinates
	int cx;
	int cy;
	float xr;
	float yr;

	// Resulting coordinates
	float xx;
	float yy;

	// Movements
	float dx;
	float dy;

	float direction;

	float maxDx;
	float maxDy;

public:
	Entity(Game* game, EntityType type, int x, int y);


	float GetX();
	float GetY();
	float GetCX();
	float GetCY();
	float GetDirection();
	EntityType GetType();
	sf::Sprite& GetSprite();

	void AddForce(int x,int y);
	void Jump(int force);
	void SetForce(int x, int y);

	void Update(float dt);

	void Draw(sf::RenderWindow& window);

	bool CollidesWith(sf::Sprite other);
};

