#pragma once

#include "Utils.h"
#include "SceneMaterial.h"
#include "Ray.h"
#include "Sampler.h"

class BRDF
{
protected:
	std::string name;
	Sampler * sampler;
public:
	BRDF(std::string name) :name(name) {}
	std::string getName() { return name; }

	virtual bool value(HitInfo & hitInfo, Ray & scatteredRay, Vector & color) = 0;
	virtual bool valueSample(HitInfo & hitInfo, Ray & scatteredRay, Vector & color, float & pdf) = 0;
};

class DiffuseLambertian : public BRDF
{
public:
	DiffuseLambertian() :BRDF("DiffuseLambertian") {}

	bool value(HitInfo & hitInfo, Ray & scatteredRay, Vector & color);
	bool valueSample(HitInfo & hitInfo, Ray & scatteredRay, Vector & color, float & pdf);
};

class SpecularPhong : public BRDF
{
public:
	SpecularPhong() :BRDF("SpecularPhong") {}

	bool value(HitInfo & hitInfo, Ray & scatteredRay, Vector & color);
	bool valueSample(HitInfo & hitInfo, Ray & scatteredRay, Vector & color, float & pdf);
};

class DielectricTransmissionFresnel : public BRDF
{
public:
	DielectricTransmissionFresnel() : BRDF("FresnelTransmission") {}

	bool value(HitInfo & hitInfo, Ray & scatteredRay, Vector & color);
	bool valueSample(HitInfo & hitInfo, Ray & scatteredRay, Vector & color, float & pdf);
};

class SpecularReflectanceFresnel : public BRDF
{
public:
	SpecularReflectanceFresnel() :BRDF("FresnelReflectance") {}

	bool value(HitInfo & hitInfo, Ray & scatteredRay, Vector & color);
	bool valueSample(HitInfo & hitInfo, Ray & scatteredRay, Vector & color, float & pdf);
};

bool ComputeSnellRefractedDirection(float inIOR, float outIOR, Vector inDir, Vector outNormal, Vector & outDir);
float ComputeFresnelRefractedEnergy(float iIOR, Vector inDir, Vector inNormal, float oIOR, Vector outDir, Vector outNormal);

bool AttemptToTransmitRay(const Ray & incidentRay, HitInfo & hitInfo, Vector & refracted, float &transmittedPercentage);

void ComputeOrthoNormalBasis(Vector zVector, Vector &yVector, Vector &xVector);