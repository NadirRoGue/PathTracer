#pragma once

#include "Utils.h"
#include "SceneMaterial.h"

class Ray
{
private:
	Vector origin;
	Vector direction;
	unsigned int depth;
	float cosineWeight;
	float distance;
public:

	Ray() :origin(Vector()), direction(Vector()), depth(0), cosineWeight(-1.0f) {}
	Ray(Vector origin, Vector direction) :origin(origin), direction(direction), depth(0), cosineWeight(-1.0f) {}
	Ray(Vector origin, Vector direction, unsigned int depth) : origin(origin), direction(direction), depth(depth), cosineWeight(-1.0f) {}

	void setWeight(float weight) { cosineWeight = weight; }
	const Vector & getOrigin() const { return origin; }
	const Vector & getDirection() const { return direction; }
	const unsigned int getDepth() const { return depth; }
	const float getCosineWeight() const { return cosineWeight; }
	float getDistance() { return distance; }
	void setDistance(float d) { distance = d; }
};

struct HitInfo
{
	Ray inRay;
	Vector lightVector;
	Vector hitPoint;
	Vector hitNormal;
	bool hit;
	bool isLight;
	Vector emission;
	SceneMaterial hittedMaterial;
	std::string physicalMaterial;
	float u, v;
} typedef HitInfo;