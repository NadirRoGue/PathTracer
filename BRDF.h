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

class Dielectric : public BRDF
{
public:
	Dielectric() : BRDF("Dielectric") {}
	bool evaluate(const Ray & incidentRay, HitInfo & hitInfo, const Vector & lightVector, Ray & scatteredRay, Vector & color);
};