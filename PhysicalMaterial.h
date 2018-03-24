#pragma once

#include <string>
#include <map>

#include "Utils.h"
#include "Ray.h"
#include "Sampler.h"

#include <iostream>

class PhysicalMaterial
{
private:
	std::string name;
public:
	PhysicalMaterial(std::string name) :name(name) {}

	std::string getName() { return name; }

	virtual Vector computeAmbientRadiance(HitInfo & hitInfo) { return Vector(); }

	virtual Vector computeDiffuseRadiance(HitInfo & hitInfo) { return Vector(); }
	virtual void scatterReflexionAndRefraction(HitInfo & hitInfo, Ray & reflectRay, float &kr, Ray &refractRay, float &kt) { kr = 0.0f; kt = 0.0f; }

	virtual bool sampleDiffuseRadiance(HitInfo & hitInfo, Ray & scatteredRay, Vector &result, float &pdf) { result = Vector(); return false; }
	virtual void sampleScatterReflexionAndRefraction(HitInfo & hitInfo, Ray & reflectRay, float &kr, float &RPdf, Ray &refractRay, float &kt, float &TPdf) { kr = 0.0f; kt = 0.0f; }
};

// =====================================================================================================
// Lambertian implementation
class MatteMaterial : public PhysicalMaterial
{
private:
	FloatSampler sampler;
public:
	MatteMaterial(std::string name = "Matte") :PhysicalMaterial(name) { }
	
	Vector computeAmbientRadiance(HitInfo & hitInfo);
	Vector computeDiffuseRadiance(HitInfo & hitInfo);
	bool sampleDiffuseRadiance(HitInfo & hitInfo, Ray & scatteredRay, Vector &result, float &pdf);
};

// =====================================================================================================
// Perfect specular implementation
class MetallicMaterial : public PhysicalMaterial
{
public:
	MetallicMaterial(std::string name = "Metallic") : PhysicalMaterial(name) { }
	
	void scatterReflexionAndRefraction(HitInfo & hitInfo, Ray & reflectRay, float &kr, Ray &refractRay, float &kt);
	void sampleScatterReflexionAndRefraction(HitInfo & hitInfo, Ray & reflectRay, float &kr, float &RPdf, Ray &refractRay, float &kt, float &TPdf);
};

// =====================================================================================================
// Dielectric implementation
class GlassMaterial : public PhysicalMaterial
{
public:
	GlassMaterial(std::string name = "Glass") : PhysicalMaterial(name) { }
	
	void scatterReflexionAndRefraction(HitInfo & hitInfo, Ray & reflectRay, float &kr, Ray &refractRay, float &kt);
	void sampleScatterReflexionAndRefraction(HitInfo & hitInfo, Ray & reflectRay, float &kr, float &RPdf, Ray &refractRay, float &kt, float &TPdf);
private:
	bool computeSnellRefractedDirection(float inIOR, float outIOR, Vector inDir, Vector hitNormal, Vector & outDir);
	float computeFresnelReflectedEnergy(float iIOR, Vector inDir, Vector inNormal, float oIOR, Vector outDir, Vector outNormal);
	void attemptToTransmitRay(HitInfo & hitInfo, Vector & refracted, float &reflectedPercentage);
};

// =====================================================================================================
// Microfacets implementation for conductors using GGX
class RoughMaterial : public PhysicalMaterial
{
private:
	FloatSampler sampler;
public:
	RoughMaterial(std::string name = "Rough") : PhysicalMaterial(name) { }

	Vector computeAmbientRadiance(HitInfo & hitInfo);
	Vector computeDiffuseRadiance(HitInfo & hitInfo);
	bool sampleDiffuseRadiance(HitInfo & hitInfo, Ray & scatteredRay, Vector &result, float &pdf);
protected:
	//χ(a) Equal to one if a > 0 and zero if a ≤ 0
	float computeXi(float a);

	//Geometry term component
	// Gi (geometric term component, G = Gi(eyevector, microfacetNormal)*Gi(lightVector, microfacetNormal)
	float geometricSmithSchlick(Vector v, Vector h, Vector n, Vector l, float roughness);

	// F (fresnel term, for conductors)
	float conductorFresnel(Vector l, Vector h, float ior);

	// D (Normal distribution, using GGX)
	float distributionBeckman(Vector h, Vector n, float roughness);

	// Samples a microfacet normal using uniform distribution
	Vector sampleMicrofacetNormal(Vector n, float roughness);
};

// =====================================================================================================

class PhysicalMaterialTable
{
private:
	static PhysicalMaterialTable * INSTANCE;

	std::map<std::string, PhysicalMaterial *> table;

public:
	static PhysicalMaterialTable & getInstance() { return *INSTANCE; }

	~PhysicalMaterialTable();

	PhysicalMaterial * getMaterialByName(std::string name);

private:
	PhysicalMaterialTable();
	void registerMaterial(PhysicalMaterial * newMaterial);
};
