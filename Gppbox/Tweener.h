#pragma once

#include "SFML/Graphics.hpp"


enum TweenerType {
	LINEAR,
	EASE,
	RANDOM
};


class Tweener
{
public:

private:

	TweenerType type;
	float speed;

	sf::Vector2f startPosition;
	sf::Vector2f endPosition;
	float progress;
	float n;

	bool hasReachedEnd;

public:

	Tweener(TweenerType type, float speed);

	void SetType(TweenerType type);
	void SetSpeed(float speed);
	void SetPositions(sf::Vector2f startPosition, sf::Vector2f endPosition);
	void ResetProgress();

	bool HasReachedEnd();

	sf::Vector2f Step(float dt);

private:
	
	float lerp(float a, float b, float t);
	float interp();
	float bezier(float t, float p0, float p1, float p2, float p3);

};
