#include "Entity.h"
#include "C.hpp"
#include "Game.hpp"
#include "iostream"

Entity::Entity(Game* game, EntityType type, int x, int y)
{
	this->game = game;

	cx = x;
	cy = y;
	
	xr = 0;
	yr = 0;
	
	xx = 0;
	yy = 0;
	
	dx = 0;
	dy = 0;

	direction = 1;

	maxDx = 10;
	maxDy = 20;

	this->type = type;
	if(type == PLAYER) texture.loadFromFile("res/Player/Idle.png");
	else if (type == ELK) texture.loadFromFile("res/Elk/Idle.png");

	collisionSize = texture.getSize();
	sprite.setTexture(texture);
	sprite.setPosition(GetX(), GetY());

	canJump = false;
}

float Entity::GetX()
{
	return (cx+xr) * C::GRID_SIZE;
}

float Entity::GetY()
{
	return (cy + yr) * C::GRID_SIZE;
}

float Entity::GetCX()
{
	return cx;
}

float Entity::GetCY()
{
	return cy;
}

float Entity::GetDirection()
{
	return direction;
}

EntityType Entity::GetType()
{
	return type;
}

sf::Sprite& Entity::GetSprite()
{
	return sprite;
}

void Entity::AddForce(int x, int y)
{
	dx += x;
	dy += y;
}

void Entity::Jump(int force)
{
	if (canJump) {
		dy -= force;
	}
}

void Entity::SetForce(int x, int y)
{
	dx = x;
	dy = y;
}

void Entity::Update(float dt)
{
	// AI
	if (type == ELK) {
		if (dx == 0) dx = 4;
	}


	//  Compute direction
	dx = std::clamp(dx, -maxDx, maxDx);
	dy = std::clamp(dy, -maxDy, maxDy);

	if (dx != 0) {
		direction = dx < 0 ? -1 : 1;
	}

	float tempX = dx * dt;
	float tempY = dy * dt;
	
	float mX = (float)(C::GRID_SIZE - collisionSize.x) / C::GRID_SIZE;
	float mY = (float)(C::GRID_SIZE - collisionSize.y) / C::GRID_SIZE;

	// Check X collisions
	xr += tempX;
	if (dx >= 0 && xr >= mX && game->isWall(cx + 1, cy)) {
		xr = mX;
		if (type == ELK) dx *= -1;
		else dx = 0;
	}
	if (dx <= 0 && xr <= 0.0f && game->isWall(cx - 1, cy)) {
		xr = 0;
		if (type == ELK) dx *= -1;
		else dx = 0;
	}
	while (xr > 1.0f) { xr--; cx++;}
	while (xr < 0.0f) { xr++; cx--; }

	// Check y collisions
	yr += tempY;
	if (dy >= 0 && yr >= mY && game->isWall(cx, cy+1)) {
		dy = 0;
		yr = mY;
		canJump = true;
	}
	if (dy <= 0 && yr <= 0.f && game->isWall(cx, cy - 1)) {
		dy = 0;
		yr = 0;
	}
	while (yr > 1.0f) { yr--; cy++; }
	while (yr < 0.0f) { yr++; cy--; canJump = false;}

	// Compute drag (if not AI)

	float rate = 1.0f / dt;
	float dfr = 60.0f / rate;
	// pow(,dfr)
	if(type != ELK) dx *= 0.97f * dt;
	dy += C::GRAVITY_VALUE * dt;
}

void Entity::Draw(sf::RenderWindow& window)
{
	sprite.setPosition(GetX(), GetY());
	window.draw(sprite);
}

bool Entity::CollidesWith(sf::Sprite other)
{
	return other.getGlobalBounds().intersects(sprite.getGlobalBounds());
}
