#include "SceneLight.h"

#include <iostream>

PointLight::PointLight() :SceneLight()
{
}

Vector PointLight::getPosition()
{
	return position;
}

Vector PointLight::samplePosition(float &pdf)
{
	pdf = 1.0f;
	return position;
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
	return (position - fromPoint).Normalize();
}

// ======================================================================

AreaLight::AreaLight()
{
	
}

Vector AreaLight::getPosition()
{
	return position;
}

Vector AreaLight::samplePosition(float &pdf)
{

	std::cout << "AQUI WTFFFF" << std::endl;
	Vector shapePos = shape->sampleShape(pdf);
	return shapePos;
}

Vector AreaLight::sampleDirection(Vector &fromPoint, float &pdf)
{
	if (sampler == NULL)
	{
		sampler = new MultiJitteredSampler(_RT_MC_BOUNCES_SAMPLES, 83);
		sampler->mapToHemiSphere(0.0f);
	}

	float posPdf;
	Vector lightPoint = samplePosition(posPdf);
	Vector hemiSample = sampler->sampleHemiSphere();

	Vector zVector = (fromPoint - lightPoint).Normalize();
	Vector yVector, xVector;
	ComputeOrthoNormalBasis(zVector, yVector, xVector);
	Vector realSample = WorldUniformHemiSample(hemiSample, zVector, yVector, xVector).Normalize();

	pdf = posPdf * (1.0f / 2.0f * float(M_PI));
	return realSample;
}

void AreaLight::setShape(SceneObject * object)
{
	color.w = 0.0f;
	object->setEmissive(color);
	object->position = position;
}