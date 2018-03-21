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

	//uniform distributed pdf($) = 1 / (b - a)
	//pdf(pos) = from shape->sampleShape
	//pdf(choosen shape) = 1 / (shapes.length - 1) (or (shapes.lenght - 1) - 0)
	//pdf = pdf(pos) * pdf(choosen shape)
	float shapePdf = shapes.size() < 2 ? 1 : shapes.size() - 1;
	pdf = posPdf * (1.0f / shapePdf);
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