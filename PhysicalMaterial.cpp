#include "PhysicalMaterial.h"

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
	invRayDir = invRayDir * 1.0f;
	Vector reflectedDir = invRayDir.reflect(hitInfo.hitNormal);
	scatteredRay = Ray(hitInfo.hitPoint + reflectedDir * PRECISSION_EPSILON, reflectedDir, incidentRay.getDepth() + 1);

	result = hitInfo.hittedMaterial.reflective;

	return scatteredRay.getDirection().Dot(hitInfo.hitNormal) > 0.0f;
}

// ======================================================================================
// Metallic

bool MirrorMaterial::scatterReflexion(const Ray & incidentRay, HitInfo & hitInfo, Ray & scatteredRay, Vector & result)
{
	Vector invRayDir(incidentRay.getDirection());
	invRayDir = invRayDir * 1.0f;
	Vector reflectedDir = invRayDir.reflect(hitInfo.hitNormal);
	scatteredRay = Ray(hitInfo.hitPoint + reflectedDir * PRECISSION_EPSILON, reflectedDir, incidentRay.getDepth() + 1);

	result = hitInfo.hittedMaterial.reflective;

	return scatteredRay.getDirection().Dot(hitInfo.hitNormal) > 0.0f;
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