#include "Tweener.h"
#include "iostream"


Tweener::Tweener(TweenerType type, float speed)
{
	this->type = type;
	progress = 0;
	this->speed = speed;
	hasReachedEnd = true;
	n = 0;
}

void Tweener::SetType(TweenerType type)
{
	this->type = type;
}

void Tweener::SetSpeed(float speed)
{
	this->speed = speed;
}

void Tweener::SetPositions(sf::Vector2f startPosition, sf::Vector2f endPosition)
{
	this->startPosition.x = startPosition.x;
	this->startPosition.y = startPosition.y;
	this->endPosition.x = endPosition.x;
	this->endPosition.y = endPosition.y;
}

void Tweener::ResetProgress()
{
	progress = 0;
	hasReachedEnd = false;
	n = 0;
}

bool Tweener::HasReachedEnd()
{
	return hasReachedEnd;
}

sf::Vector2f Tweener::Step(float dt)
{
	float dist = std::sqrt(((startPosition.x+endPosition.x)* (startPosition.x + endPosition.x)) + ((startPosition.y + endPosition.y) * (startPosition.y + endPosition.y)));
	
	if (type == RANDOM) {
		progress += dt * (std::rand() % 100 < 33 ? speed : 0);
	}
	else {
		progress += dt * speed;
	}

	if (progress > 1) {
		progress = 1;
		hasReachedEnd = true;
	}

	n = interp();

	return sf::Vector2(lerp(startPosition.x, endPosition.x, n), lerp(startPosition.y, endPosition.y, n));
}

float Tweener::lerp(float a, float b, float t)
{
	return a + t * (b - a);
}

float Tweener::interp()
{
	switch (type) {
	case LINEAR: return progress;
	case RANDOM: return progress;
	case EASE: return bezier(progress, 0, 0, 1, 1);
	}
}

float Tweener::bezier(float t,float p0, float p1, float p2, float p3)
{
		float minust = 1.0 - t;

		return
			(minust * minust * minust) * p0 +
			3 * (t * (minust*minust) * p1 + (t*t) * (minust)*p2) +
			(t*t*t) * p3;
}
