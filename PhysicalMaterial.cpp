#include "PhysicalMaterial.h"

#include <iostream>

// =======================================================================================
// Matte

Vector MatteMaterial::computeAmbientRadiance(const Ray & incidentRay, HitInfo & hitInfo)
{
	Vector result;
	ambientBRDF->evaluate(incidentRay, hitInfo, Vector(), Ray(), result);

	return result;
}

bool MatteMaterial::computeDiffuseRadiance(const Ray & incidentRay, HitInfo & hitInfo, const Vector & lightVector, Ray & scatteredRay, Vector & result)
{
	return diffuseBRDF->evaluate(incidentRay, hitInfo, lightVector, scatteredRay, result);
}

// =======================================================================================
// Plastic

bool PlasticMaterial::computeSpecularRadiance(const Ray & incidentRay, HitInfo & hitInfo, const Vector & lightVector, Ray & scatteredRay, Vector & result)
{
	return specularBRDF->evaluate(incidentRay, hitInfo, lightVector, scatteredRay, result);
}

// ======================================================================================
// Plastic with reflexions

bool ReflexivePlasticMaterial::scatterReflexion(const Ray & incidentRay, HitInfo & hitInfo, Ray & scatteredRay, Vector & result)
{
	Vector invRayDir(incidentRay.getDirection());
	Vector reflectedDir = invRayDir.reflect(hitInfo.hitNormal).Normalize();
	scatteredRay = Ray(hitInfo.hitPoint + hitInfo.hitNormal * PRECISSION_EPSILON, reflectedDir, incidentRay.getDepth() + 1);

	float factor = clampValue(scatteredRay.getDirection().Dot(hitInfo.hitNormal), 0.0f, 1.0f);
	result = hitInfo.hittedMaterial.reflective * factor;
	return factor > 0.0f;
}

// ======================================================================================
// Mirror

bool MirrorMaterial::scatterReflexion(const Ray & incidentRay, HitInfo & hitInfo, Ray & scatteredRay, Vector & result)
{
	Vector invRayDir(incidentRay.getDirection());
	Vector reflectedDir = invRayDir.reflect(hitInfo.hitNormal).Normalize();
	scatteredRay = Ray(hitInfo.hitPoint + hitInfo.hitNormal * PRECISSION_EPSILON, reflectedDir, incidentRay.getDepth() + 1);

	float factor = clampValue(reflectedDir.Dot(hitInfo.hitNormal), 0.0f, 1.0f);
	result = hitInfo.hittedMaterial.reflective * factor;

	//std::cout << factor << std::endl;

	return factor > 0.0f;
}

// ======================================================================================

bool MetallicMaterial::computeSpecularRadiance(const Ray & incidentRay, HitInfo & hitInfo, const Vector & lightVector, Ray & scatteredRay, Vector & result)
{
	return specularBRDF->evaluate(incidentRay, hitInfo, lightVector, scatteredRay, result);
}


bool MetallicMaterial::scatterReflexion(const Ray & incidentRay, HitInfo & hitInfo, Ray & scatteredRay, Vector & result)
{
	Vector invRayDir(incidentRay.getDirection());
	Vector reflectedDir = invRayDir.reflect(hitInfo.hitNormal);
	scatteredRay = Ray(hitInfo.hitPoint + reflectedDir * PRECISSION_EPSILON, reflectedDir, incidentRay.getDepth() + 1);

	float factor = clampValue(scatteredRay.getDirection().Dot(hitInfo.hitNormal), 0.0, 1.0);
	result = hitInfo.hittedMaterial.reflective;

	return factor > 0.0f;
}

// ======================================================================================
// ======================================================================================

PhysicalMaterialTable * PhysicalMaterialTable::INSTANCE = new PhysicalMaterialTable();

PhysicalMaterialTable::PhysicalMaterialTable()
{
	registerMaterial(new MatteMaterial());
	registerMaterial(new PlasticMaterial());
	registerMaterial(new ReflexivePlasticMaterial());
	registerMaterial(new MirrorMaterial());
	registerMaterial(new MetallicMaterial());
}

PhysicalMaterialTable::~PhysicalMaterialTable()
{
	std::map<std::string, PhysicalMaterial *>::iterator it = table.begin();
	while (it++ != table.end())
	{
		delete it->second;
	}
}

void PhysicalMaterialTable::registerMaterial(PhysicalMaterial * material)
{
	std::map<std::string, PhysicalMaterial *>::iterator it = table.find(material->getName());
	if (it == table.end())
	{
		table[material->getName()] = material;
	}
}

PhysicalMaterial * PhysicalMaterialTable::getMaterialByName(std::string name)
{
	std::map<std::string, PhysicalMaterial *>::iterator it = table.find(name);
	PhysicalMaterial * material = NULL;
	if (it != table.end())
	{
		material = it->second;
	}

	return material;
}