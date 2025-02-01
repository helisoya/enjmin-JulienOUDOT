#include "Entity.h"
#include "C.hpp"
#include "Game.hpp"
#include "iostream"
#include "Tweener.h"
#include "math.h"

Entity::Entity(Game* game, EntityType type, int x, int y) : tweener(TweenerType::EASE, 1.0f) {
	this->game = game;

	cx = x;
	cy = y;
	
	xr = 0;
	yr = 0;
	
	xx = x * C::GRID_SIZE;
	yy = y * C::GRID_SIZE;
	
	dx = 0;
	dy = 0;

	direction = 1;

	maxDx = 10;
	maxDy = 20;

	this->type = type;
	if(type == PLAYER) texture.loadFromFile("res/Player/Idle.png");
	else if (type == ELK) texture.loadFromFile("res/Elk/Idle.png");
	else if(type == MISSILE) texture.loadFromFile("res/Missile/Idle.png");
	else if (type == DRONE) texture.loadFromFile("res/Drone/Idle.png");
	else if (type == BULLET) texture.loadFromFile("res/Bullet/Idle.png");

	collisionSize = texture.getSize();
	sprite.setTexture(texture);
	sprite.setPosition(GetX(), GetY());

	muzzleFlareLength = 0.1f;
	muzzleFlareTex.loadFromFile("res/MuzzleFlare/Idle.png");
	muzzleFlare.setTexture(muzzleFlareTex);

	if (type == MISSILE) {
		tweener.SetType(LINEAR);
		tweener.SetSpeed(5);
	}

	droneFireCooldown = 0.2f;

	canJump = false;
}

bool Entity::IsAlive()
{
	return !dead;
}

float Entity::GetX()
{
	if (type == MISSILE) return xx;
	return (cx+xr) * C::GRID_SIZE;
}

float Entity::GetY()
{
	if (type == MISSILE) return yy;
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

void Entity::SetPosition(int x, int y)
{
	cx = x;
	cy = y;
	xr = 0;
	yr = 0;
}

void Entity::AddForce(int x, int y, bool ignoreClamp)
{
	dx += x;
	dy += y;

	if (!ignoreClamp) {
		dx = std::clamp(dx, -maxDx, maxDx);
		dy = std::clamp(dy, -maxDy, maxDy);

		if (dx != 0) {
			direction = dx < 0 ? -1 : 1;
		}
	}
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

	if (dx != 0) {
		direction = dx < 0 ? -1 : 1;
	}
}

void Entity::Update(float dt)
{
	std::vector<Entity*>& entities = game->getEntities();


	// Muzzle flare

	if (currentMuzzleFlareLength > 0) currentMuzzleFlareLength -= dt;

	// AI
	if (type == ELK) {
		// Elk
		if (dx == 0) dx = 4;
	}
	else if (type == MISSILE) { 
		// Missile

		// Check if destroyed / killed an elk
		if (game->isWall((int)xx / C::GRID_SIZE, (int)yy / C::GRID_SIZE)) {
			Kill();
			game->addShakes(5);
			return;
		}
		else {
			for (Entity* entity : entities) {
				if (entity->type == ELK && entity->IsAlive()) {
					if (entity->CollidesWith(sprite)) {
						game->addShakes(5);
						Kill();
						entity->Kill();
						return;
					}
				}
			}
		}



		// Missile only use tweener
		if (tweener.HasReachedEnd()) {

			// Search for a target
			float closestXX = 0;
			float closestYY = 0;
			float closestDist = 0;
			float ennemyX = 0;
			float ennemyY = 0;
			float dist = 0;
			bool foundEnnemy = false;

			for (Entity* entity : entities) {
				if (entity->type == ELK && entity->IsAlive()) {
					ennemyX = entity->GetX();
					ennemyY = entity->GetY();
					dist = sqrt( (ennemyX - xx) * (ennemyX - xx) + (ennemyY - yy) * (ennemyY - yy));
					if (!foundEnnemy || closestDist > dist) {
						std::cout << "New Closest Ennemy " << ennemyX << " " << ennemyY << std::endl;
						closestXX = ennemyX;
						closestYY = ennemyY;
						closestDist = dist;
						foundEnnemy = true;
					}
				}
			}

			// If no ennemy found, go up
			if (!foundEnnemy) {
				closestXX = xx;
				closestYY = yy - 3 * C::GRID_SIZE;
			}


			std::cout << xx << " " << yy << " -> " << closestXX << " " << closestYY << std::endl;
			tweener.SetPositions(sf::Vector2f(xx, yy), sf::Vector2f(closestXX, closestYY));
			tweener.ResetProgress();
		}

		sf::Vector2f newPos = tweener.Step(dt);
		sprite.setRotation(atan2(newPos.y - yy, newPos.x - xx) * 180.0 / M_PI);
		
		xx = newPos.x;
		yy = newPos.y;

		// Kill missile if has gone too far
		if (xx < -C::GRID_SIZE || yy < -C::GRID_SIZE || xx > 1280 + C::GRID_SIZE || yy > 720 + C::GRID_SIZE) {
			game->addShakes(5);
			Kill();
		}

		return;
	}
	else if (type == DRONE) {
		// Drone

		Entity* player = game->getPlayer();

		float x0 = GetX();
		float x1 = player->GetX();
		float y0 = GetY();
		float y1 = player->GetY();

		float dist = sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
		
		if (dist > C::GRID_SIZE && abs(dx) <= 0.05f && abs(dy) < 0.05f) {
			// Propulsion towards player
			AddForce((x1 - x0) / dist * 5, (y1 - y0) / dist * 5);
			sprite.rotate(5 * dt);
		}

		if (droneCurrentCooldown > 0) {
			droneCurrentCooldown -= dt;
		}
		else {
			float closestXX = 0;
			float closestYY = 0;
			float closestDist = C::GRID_SIZE * 8;
			float ennemyX = 0;
			float ennemyY = 0;
			bool foundEnnemy = false;


			for (Entity* entity : entities) {
				if (entity->type == ELK && entity->IsAlive()) {
					ennemyX = entity->GetX();
					ennemyY = entity->GetY();
					dist = sqrt((ennemyX - x0) * (ennemyX - x0) + (ennemyY - y0) * (ennemyY - y0));
					if (closestDist > dist) {
						closestXX = ennemyX;
						closestYY = ennemyY;
						closestDist = dist;
						foundEnnemy = true;
					}
				}
			}


			if (foundEnnemy) {
				Entity* bullet = new Entity(game, BULLET, cx, cy);
				bullet->SetForce((closestXX - x0) / dist * 16.0, (closestYY - y0) / dist * 16.0);
				game->entitiesToAddAfterUpdate.push_back(bullet);
				droneCurrentCooldown = droneFireCooldown;
				game->addShakes(2);
				AddForce(-dx* 50, -dy*50, true);
				currentMuzzleFlareLength = muzzleFlareLength;
			}
		}
	}
	else if (type == BULLET) {
		float x0 = GetX();
		float x1 = x0 + dx;
		float y0 = GetY();
		float y1 = y0 + dy;
		sprite.setRotation(atan2(y1 - y0, x1 - x0) * 180.0 / M_PI);

		// Destroy if Out of bounds
		if (x0 < -C::GRID_SIZE || y0 < -C::GRID_SIZE || x0 > 1280 + C::GRID_SIZE || y0 > 720 + C::GRID_SIZE) {
			Kill();
			return;
		}

		// Check if killed an elk
		for (Entity* entity : entities) {
			if (entity->type == ELK && entity->IsAlive() && entity->CollidesWith(sprite)) {
				Kill();
				entity->Kill();
				return;
			}
		}


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
		else if (type == BULLET) Kill();
		else dx = 0;
	}
	if (dx <= 0 && xr <= 0.0f && game->isWall(cx - 1, cy)) {
		xr = 0;
		if (type == ELK) dx *= -1;
		else if (type == BULLET) Kill();
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
		if (type == BULLET) Kill();
	}
	if (dy <= 0 && yr <= 0.f && game->isWall(cx, cy - 1)) {
		dy = 0;
		yr = 0;
		if (type == BULLET) Kill();
	}
	while (yr > 1.0f) { yr--; cy++; }
	while (yr < 0.0f) { yr++; cy--; canJump = false;}

	// Compute drag (if not AI)

	if(type == PLAYER || type == DRONE) dx *= 0.97f * dt;
	if (type == PLAYER || type == ELK) dy += C::GRAVITY_VALUE * dt;
	else if (type == DRONE) dy *= 0.97f * dt;
}

void Entity::Draw(sf::RenderWindow& window)
{
	if (dead) return;

	float x = GetX();
	float y = GetY();

	muzzleFlare.setPosition(x,y+collisionSize.y/2);
	sprite.setPosition(x, y);
	window.draw(sprite);
	
	if (currentMuzzleFlareLength > 0) window.draw(muzzleFlare);
}

void Entity::Kill()
{
	dead = true;
}

void Entity::Reset()
{
	dead = false;
}

bool Entity::CollidesWith(sf::Sprite other)
{
	return other.getGlobalBounds().intersects(sprite.getGlobalBounds());
}
