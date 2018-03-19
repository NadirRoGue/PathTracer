#include "SceneLight.h"

#include <iostream>

PointLight::PointLight() :SceneLight()
{
}

Vector PointLight::sampleDirection(Vector & fromPoint, float &pdf)
{
	/*
	if(sampler == NULL)
	{
		sampler = new MultiJitteredSampler(_RT_MC_BOUNCES_SAMPLES, 83);
		sampler->mapToHemiSphere(0.0f);
	}

	Vector zVector = (fromPoint - position).Normalize();
	Vector yVector, xVector;
	ComputeOrthoNormalBasis(zVector, yVector, xVector);

	Vector sample = sampler->sampleHemiSphere();
	Vector dir = WorldUniformHemiSample(sample, zVector, yVector, xVector);

	dir.w = 0.0f;
	dir = dir.Normalize();

	float cosTheta = fabs(zVector.Dot(dir));
	if (cosTheta > 0.0f)
	{
		pdf = (cosTheta / (float(M_PI)));
	}

	return dir;*/
	pdf = 1.0f;
	return (position - fromPoint);
}

// ======================================================================

AreaLight::AreaLight()
{
	
}

Vector AreaLight::sampleDirection(Vector &fromPoint, float &pdf)
{
	if (sampler == NULL)
	{
		sampler = new MultiJitteredSampler(_RT_MC_BOUNCES_SAMPLES, 83);
		d = std::uniform_int_distribution<unsigned int>(0, shapes.size() - 1);
	}

	SceneObject * shape = shapes[d(engine)];

	float posPdf;
	Vector pos = shape->sampleShape(posPdf);

	pdf = 1.0f;// posPdf * (1.0f / shapes.size());
	return (pos - fromPoint);
}

void AreaLight::addShape(SceneObject * object)
{
	//color.w = 0.0f;
	object->setEmissive(color);
	shapes.push_back(object);
}