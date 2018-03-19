#include "PhysicalMaterial.h"

#include "Config.h"
#include <iostream>

// =======================================================================================
// Matte

Vector MatteMaterial::computeAmbientRadiance(HitInfo & hitInfo)
{
	return hitInfo.hittedMaterial.diffuse;
}

bool MatteMaterial::computeDiffuseRadiance(HitInfo & hitInfo, Ray & scatteredRay, Vector & result)
{
	return diffuseBRDF->value(hitInfo, scatteredRay, result);
}

bool MatteMaterial::sampleDiffuseRadiance(HitInfo & hitInfo, Ray & scatteredRay, Vector &result, float &pdf)
{
	diffuseBRDF->valueSample(hitInfo, scatteredRay, result, pdf);	
	return true;
}

// =======================================================================================
// Plastic

bool PlasticMaterial::computeSpecularRadiance(HitInfo & hitInfo, Ray & scatteredRay, Vector & result)
{
	return specularBRDF->value(hitInfo, scatteredRay, result);
}

// ======================================================================================
// Plastic with reflexions

bool ReflexivePlasticMaterial::scatterReflexion(HitInfo & hitInfo, Ray & scatteredRay, Vector & result)
{
	Vector invRayDir(hitInfo.inRay.getDirection());
	Vector reflectedDir = invRayDir.reflect(hitInfo.hitNormal).Normalize();
	scatteredRay = Ray(hitInfo.hitPoint + hitInfo.hitNormal * _RT_BIAS, reflectedDir, hitInfo.inRay.getDepth() + 1);

	float factor = clampValue(scatteredRay.getDirection().Dot(hitInfo.hitNormal), 0.0f, 1.0f);
	result = hitInfo.hittedMaterial.reflective;// *factor;
	return factor > 0.0f;
}

// ======================================================================================
// Mirror

bool MirrorMaterial::scatterReflexion(HitInfo & hitInfo, Ray & scatteredRay, Vector & result)
{
	Vector invRayDir(hitInfo.inRay.getDirection());
	Vector reflectedDir = invRayDir.reflect(hitInfo.hitNormal).Normalize();
	scatteredRay = Ray(hitInfo.hitPoint + hitInfo.hitNormal * _RT_BIAS, reflectedDir, hitInfo.inRay.getDepth() + 1);

	float factor = clampValue(reflectedDir.Dot(hitInfo.hitNormal), 0.0f, 1.0f);
	result = hitInfo.hittedMaterial.reflective * factor;

	return factor > 0.0f;
}

// ======================================================================================

bool MetallicMaterial::scatterReflexion(HitInfo & hitInfo, Ray & scatteredRay, Vector & result)
{
	Vector invRayDir(hitInfo.inRay.getDirection());
	Vector reflectedDir = invRayDir.reflect(hitInfo.hitNormal);
	scatteredRay = Ray(hitInfo.hitPoint + reflectedDir * _RT_BIAS, reflectedDir, hitInfo.inRay.getDepth() + 1);

	float factor = clampValue(scatteredRay.getDirection().Dot(hitInfo.hitNormal), 0.0, 1.0);
	result = hitInfo.hittedMaterial.reflective;

	return factor > 0.0f;
}

bool MetallicMaterial::sampleScatterReflexion(HitInfo & hitInfo, Ray & scatteredRay, Vector & result)
{
	Vector invRayDir(hitInfo.inRay.getDirection());
	Vector reflectedDir = invRayDir.reflect(hitInfo.hitNormal);
	scatteredRay = Ray(hitInfo.hitPoint + reflectedDir * _RT_BIAS, reflectedDir, hitInfo.inRay.getDepth() + 1);

	float factor = clampValue(scatteredRay.getDirection().Dot(hitInfo.hitNormal), 0.0, 1.0);
	result = hitInfo.hittedMaterial.reflective;

	return factor > 0.0f;
}

// ======================================================================================

bool GlassMaterial::scatterReflexion(HitInfo & hitInfo, Ray & scatteredRay, Vector & result)
{
	return reflective->value(hitInfo, scatteredRay, result);
}

bool GlassMaterial::sampleScatterReflexion(HitInfo & hitInfo, Ray & scatteredRay, Vector & result)
{
	float pdf;
	reflective->valueSample(hitInfo, scatteredRay, result, pdf);
	result = result / pdf;
	return pdf > 0.0f;
}

bool GlassMaterial::scatterTransmission(HitInfo & hitInfo, Ray & scatteredRay, Vector & result)
{
	return transmissive->value(hitInfo, scatteredRay, result);
}

bool GlassMaterial::sampleScatterTransmission(HitInfo & hitInfo, Ray & scatteredRay, Vector & result)
{
	float pdf;
	transmissive->valueSample(hitInfo, scatteredRay, result, pdf);
	result = result / pdf;
	return pdf > 0.0f;
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
	registerMaterial(new GlassMaterial());
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