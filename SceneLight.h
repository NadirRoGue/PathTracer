#pragma once

#include <vector>

#include "Utils.h"
#include "Sampler.h"
#include "SceneObject.h"


/*
SceneLight Class - The light properties of a single light-source in a ray-trace scene

The Scene class holds a list of these
*/
class SceneLight
{
protected:
	FloatSampler sample;
public:
	SceneLight() { }
	~SceneLight() { }
	virtual Vector sampleDirection(Vector &fromPoint, float &pdf) = 0;

	float attenuationConstant, attenuationLinear, attenuationQuadratic;
	Vector color;
	Vector position;
	unsigned int id;
};

class PointLight: public SceneLight
{
public:
	PointLight() { }
	Vector sampleDirection(Vector &fromPoint, float &pdf);
};

class AreaLight : public SceneLight
{
private:
	IntegerSampler shapeSampler;
	std::vector<SceneObject*> shapes;

	std::default_random_engine engine;
	std::uniform_int_distribution<unsigned int> d;
public:
	AreaLight() { }
	Vector sampleDirection(Vector &fromPoint, float &pdf);
	void addShapes(std::vector<SceneObject *> & objects);
};