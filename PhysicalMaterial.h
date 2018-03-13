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

	virtual Vector computeAmbientRadiance(const Ray & incidentRay, HitInfo & hitInfo) { return Vector(); }
	virtual bool computeDiffuseRadiance(const Ray & incidentRay, HitInfo & hitInfo, const Vector & lightVector, Ray & scatteredRay, Vector & result) { result = Vector(); return false;  }
	virtual bool computeSpecularRadiance(const Ray & incidentRay, HitInfo & hitInfo, const Vector & lightVector, Ray & scatteredRay, Vector & result) { result = Vector(); return false;  }

	virtual bool scatterReflexion(const Ray & incidentRay, HitInfo & hitInfo, Ray & scatteredRay, Vector & result) { return false; }
	virtual bool scatterTransmission(const Ray & incidentRay, HitInfo & hitInfo, Ray & scatteredRay, Vector & result) { return false; }
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

	Vector computeAmbientRadiance(const Ray & incidentRay, HitInfo & hitInfo);
	bool computeDiffuseRadiance(const Ray & incidentRay, HitInfo & hitInfo, const Vector & lightVector, Ray & scatteredRay, Vector & result);
};

// =====================================================================================================

class PlasticMaterial : public MatteMaterial
{
private:
	BRDF * specularBRDF;

public:
	PlasticMaterial(std::string name = "Plastic") :MatteMaterial(name) { specularBRDF = new SpecularPhong(); }
	~PlasticMaterial() { delete specularBRDF; }

	bool computeSpecularRadiance(const Ray & incidentRay, HitInfo & hitInfo, const Vector & lightVector, Ray & scatteredRay, Vector & result);
};

// =====================================================================================================

class ReflexivePlasticMaterial : public PlasticMaterial
{
public:
	ReflexivePlasticMaterial(std::string name = "ReflexivePlastic") : PlasticMaterial(name) {}

	bool scatterReflexion(const Ray & incidentRay, HitInfo & hitInfo, Ray & scatteredRay, Vector & result);
};

// =====================================================================================================

class MirrorMaterial : public MatteMaterial
{
public:
	MirrorMaterial(std::string name = "Mirror") : MatteMaterial(name) {}

	bool scatterReflexion(const Ray & incidentRay, HitInfo & hitInfo, Ray & scatteredRay, Vector & result);
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
	bool scatterReflexion(const Ray & incidentRay, HitInfo & hitInfo, Ray & scatteredRay, Vector & result);
};

// =====================================================================================================

class GlassMaterial : public PlasticMaterial
{
private:
	BRDF * transmissive;
	BRDF * reflective;
public:
	GlassMaterial(std::string name = "Glass") : PlasticMaterial(name) { transmissive = new DielectricTransmissionFresnel(); reflective = new SpecularReflectanceFresnel();  }
	~GlassMaterial() { delete transmissive; delete reflective; }

	bool scatterReflexion(const Ray & incidentRay, HitInfo & hitInfo, Ray & scatteredRay, Vector & result);
	bool scatterTransmission(const Ray & incidentRay, HitInfo & hitInfo, Ray & scatteredRay, Vector & result);
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
