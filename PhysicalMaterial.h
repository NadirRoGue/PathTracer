#pragma once

#include <string>
#include <map>

#include "Utils.h"
#include "Ray.h"
#include "BRDF.h"

#include <iostream>

class PhysicalMaterial
{
private:
	std::string name;
public:
	PhysicalMaterial(std::string name) :name(name) {}

	std::string getName() { return name; }

	virtual Vector computeAmbientRadiance(HitInfo & hitInfo) { return Vector(); }
	virtual bool computeDiffuseRadiance(HitInfo & hitInfo, Ray & scatteredRay, Vector & result) { result = Vector(); return false;  }
	virtual bool computeSpecularRadiance(HitInfo & hitInfo, Ray & scatteredRay, Vector & result) { result = Vector(); return false;  }

	virtual bool sampleDiffuseRadiance(HitInfo & hitInfo, Ray & scatteredRay, Vector &result, float &pdf) { result = Vector(); return false; }

	virtual bool scatterReflexion(HitInfo & hitInfo, Ray & scatteredRay, Vector & result) { return false; }
	virtual bool scatterTransmission(HitInfo & hitInfo, Ray & scatteredRay, Vector & result) { return false; }

	virtual bool sampleScatterReflexion(HitInfo & hitInfo, Ray & scatteredRay, Vector & result) { return false; }
	virtual bool sampleScatterTransmission(HitInfo & hitInfo, Ray & scatteredRay, Vector & result) { return false; }
};

// =====================================================================================================

class MatteMaterial : public PhysicalMaterial
{
private:
	BRDF * ambientBRDF;
	BRDF * diffuseBRDF;

public:
	MatteMaterial(std::string name = "Matte") :PhysicalMaterial(name) { ambientBRDF = new DiffuseLambertian(); diffuseBRDF = new DiffuseLambertian(); }
	~MatteMaterial() { delete ambientBRDF; delete diffuseBRDF; }

	Vector computeAmbientRadiance(HitInfo & hitInfo);
	bool computeDiffuseRadiance(HitInfo & hitInfo, Ray & scatteredRay, Vector & result);
	bool sampleDiffuseRadiance(HitInfo & hitInfo, Ray & scatteredRay, Vector &result, float &pdf);
};

// =====================================================================================================

class PlasticMaterial : public MatteMaterial
{
private:
	BRDF * specularBRDF;

public:
	PlasticMaterial(std::string name = "Plastic") :MatteMaterial(name) { specularBRDF = new SpecularPhong(); }
	~PlasticMaterial() { delete specularBRDF; }

	bool computeSpecularRadiance(HitInfo & hitInfo, Ray & scatteredRay, Vector & result);
};

// =====================================================================================================

class ReflexivePlasticMaterial : public PlasticMaterial
{
public:
	ReflexivePlasticMaterial(std::string name = "ReflexivePlastic") : PlasticMaterial(name) {}

	bool scatterReflexion(HitInfo & hitInfo, Ray & scatteredRay, Vector & result);
};

// =====================================================================================================

class MirrorMaterial : public MatteMaterial
{
public:
	MirrorMaterial(std::string name = "Mirror") : MatteMaterial(name) {}

	bool scatterReflexion(HitInfo & hitInfo, Ray & scatteredRay, Vector & result);
};

// =====================================================================================================

class MetallicMaterial : public PhysicalMaterial
{
private:
	BRDF * specularBRDF;
public:
	MetallicMaterial(std::string name = "Metallic") : PhysicalMaterial(name) { specularBRDF = new SpecularPhong(); }
	~MetallicMaterial() { delete specularBRDF; }

	//bool computeSpecularRadiance(const Ray & incidentRay, HitInfo & hitInfo, const Vector & lightVector, Ray & scatteredRay, Vector & result);
	bool scatterReflexion(HitInfo & hitInfo, Ray & scatteredRay, Vector & result);
	bool sampleScatterReflexion(HitInfo & hitInfo, Ray & scatteredRay, Vector & result);
};

// =====================================================================================================

class GlassMaterial : public PhysicalMaterial
{
private:
	BRDF * transmissive;
	BRDF * reflective;
public:
	GlassMaterial(std::string name = "Glass") : PhysicalMaterial(name) { transmissive = new DielectricTransmissionFresnel(); reflective = new SpecularReflectanceFresnel();  }
	~GlassMaterial() { delete transmissive; delete reflective; }

	bool scatterReflexion(HitInfo & hitInfo, Ray & scatteredRay, Vector & result);
	bool scatterTransmission(HitInfo & hitInfo, Ray & scatteredRay, Vector & result);

	bool sampleScatterReflexion(HitInfo & hitInfo, Ray & scatteredRay, Vector & result);
	bool sampleScatterTransmission(HitInfo & hitInfo, Ray & scatteredRay, Vector & result);
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
