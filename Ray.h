#pragma once

#include "Utils.h"

#include "SceneMaterial.h"

#define PRECISSION_EPSILON 0.00005f


struct HitInfo
{
	Vector hitPoint;
	Vector hitNormal;

	bool hit;

	unsigned int numHittedMaterials;
	SceneMaterial *hittedMaterials[3];
	float contributions[3];

	float u[3], v[3];

} typedef HitInfo;

class Ray
{
private:
	Vector origin;
	Vector direction;

public:

	Ray() :origin(Vector()), direction(Vector()) {}
	Ray(Vector origin, Vector direction) :origin(origin), direction(direction) {}

	const Vector & getOrigin() const { return origin; }
	const Vector & getDirection() const { return direction; }
	Vector getPoint(float offset) { return origin + direction * offset; }
};