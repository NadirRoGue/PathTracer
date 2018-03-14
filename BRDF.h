#pragma once

#include "Utils.h"
#include "SceneMaterial.h"
#include "Ray.h"

class BRDF
{
protected:
	std::string name;
public:
	BRDF(std::string name) :name(name) {}
	std::string getName() { return name; }
	virtual bool evaluate(const Ray & incidentRay, HitInfo & hitInfo, const Vector & lightVector, Ray & scatteredRay, Vector & color) = 0;
};

class DiffuseLambertian : public BRDF
{
public:
	DiffuseLambertian() :BRDF("DiffuseLambertian") {}
	bool evaluate(const Ray & incidentRay, HitInfo & hitInfo, const Vector & lightVector, Ray & scatteredRay, Vector & color);
private:
	Vector randomPointInSphere();
};

class SpecularPhong : public BRDF
{
public:
	SpecularPhong() :BRDF("SpecularPhong") {}
	bool evaluate(const Ray & incidentRay, HitInfo & hitInfo, const Vector & lightVector, Ray & scatteredRay, Vector & color);
};

class DielectricTransmissionFresnel : public BRDF
{
public:
	DielectricTransmissionFresnel() : BRDF("FresnelTransmission") {}
	bool evaluate(const Ray & incidentRay, HitInfo & hitInfo, const Vector & lightVector, Ray & scatteredRay, Vector & color);
};

class SpecularReflectanceFresnel : public BRDF
{
public:
	SpecularReflectanceFresnel() :BRDF("FresnelReflectance") {}
	bool evaluate(const Ray & incidentRay, HitInfo & hitInfo, const Vector & lightVector, Ray & scatteredRay, Vector & color);
};

bool ComputeSnellRefractedDirection(float inIOR, float outIOR, Vector inDir, Vector outNormal, Vector & outDir);
float ComputeFresnelRefractedEnergy(float iIOR, Vector & inDir, Vector & inNormal, float oIOR, Vector & outDir, Vector & outNormal);

bool AttemptToTransmitRay(const Ray & incidentRay, HitInfo & hitInfo, Vector & refracted, float &transmittedPercentage);