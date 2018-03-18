#pragma once

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
	Sampler * sampler;
public:
	SceneLight() { sampler = NULL; }
	~SceneLight() { if (sampler != NULL) delete sampler; }
	virtual Vector getPosition() = 0;
	virtual Vector samplePosition(float & pdf) = 0;
	virtual Vector sampleDirection(Vector &fromPoint, float &pdf) = 0;

	float attenuationConstant, attenuationLinear, attenuationQuadratic;
	Vector color;
	Vector position;
	unsigned int id;
};

class PointLight: public SceneLight
{
public:
	PointLight();
	Vector getPosition();
	Vector samplePosition(float &pdf);
	Vector sampleDirection(Vector &fromPoint, float &pdf);
};

class AreaLight : public SceneLight
{
private:
	SceneObject * shape;
public:
	AreaLight();
	Vector getPosition();
	Vector samplePosition(float &pdf);
	Vector sampleDirection(Vector &fromPoint, float &pdf);
	void setShape(SceneObject * object);
};