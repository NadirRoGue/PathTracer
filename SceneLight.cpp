#include "SceneLight.h"

#include <iostream>


Vector PointLight::sampleDirection(Vector & fromPoint, float &pdf)
{
	pdf = 1.0f;
	return (position - fromPoint);
}

// ======================================================================


Vector AreaLight::sampleDirection(Vector &fromPoint, float &pdf)
{
	int indice = shapeSampler.sampleRect();
	SceneObject * shape = shapes[indice];

	float posPdf;
	Vector pos = shape->sampleShape(posPdf);

	pdf = 1.0f;// posPdf * ((indice + 1) / shapes.size());
	return (pos - fromPoint);
}

void AreaLight::addShapes(std::vector<SceneObject *> & objects)
{
	for (SceneObject * obj : objects)
	{
		obj->setEmissive(color);
	}

	shapes = objects;

	shapeSampler = IntegerSampler(0, unsigned int(shapes.size() - 1));
}