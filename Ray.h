#pragma once

#include "Utils.h"

#include "SceneMaterial.h"

#define PRECISSION_EPSILON 0.00005f
#define MAX_BOUNCES 4

struct HitInfo
{
	Vector hitPoint;
	Vector hitNormal;
	bool hit;
	SceneMaterial hittedMaterial;
	std::string physicalMaterial;
	float u, v;
} typedef HitInfo;

class Ray
{
private:
	Vector origin;
	Vector direction;

	unsigned int depth;

public:

	Ray() :origin(Vector()), direction(Vector()), depth(0) {}
	Ray(Vector origin, Vector direction) :origin(origin), direction(direction), depth(0) {}
	Ray(Vector origin, Vector direction, unsigned int depth) : origin(origin), direction(direction), depth(depth) {}

	const Vector & getOrigin() const { return origin; }
	const Vector & getDirection() const { return direction; }
	const unsigned int getDepth() const { return depth; };
	//Vector getPoint(float offset) { return origin + direction * offset; }
};