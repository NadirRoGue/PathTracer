#include "PhysicalMaterial.h"

#include "Config.h"
#include <iostream>

// =======================================================================================
// Matte

Vector MatteMaterial::computeAmbientRadiance(HitInfo & hitInfo)
{
	return hitInfo.hittedMaterial.diffuse;
}

Vector MatteMaterial::computeDiffuseRadiance(HitInfo & hitInfo)
{
	// OPTIMIZATION: Cancel PI term and apply only to direct lighting
	return hitInfo.hittedMaterial.diffuse;// / float(M_PI);
}

bool MatteMaterial::sampleDiffuseRadiance(HitInfo & hitInfo, Ray & scatteredRay, Vector &result, float &pdf)
{
	Vector zVector = hitInfo.hitNormal;
	Vector yVector, xVector;
	ComputeOrthoNormalBasis(zVector, yVector, xVector);

	Vector sample = sampler.sampleHemiSphere();
	Vector scatteredDir = WorldUniformHemiSample(sample, zVector, yVector, xVector).Normalize();

	scatteredRay = Ray(hitInfo.hitPoint + scatteredDir * _RT_BIAS, scatteredDir, hitInfo.inRay.getDepth() + 1);
	scatteredRay.setWeight(fabs(zVector.Dot(scatteredDir)));
	// cosine weight hemisphere PDF = cos / PI
	// diffuse-diffuse sampling PDF = 1 / 2*PI
	// since rendering equation has cos, both terms cancel

	// OPTIMIZATION: Cancel PI term with diffuse reflectance term
	pdf = 1.0f / (2.0f);
	return true;
}

// ======================================================================================

void MetallicMaterial::scatterReflexionAndRefraction(HitInfo & hitInfo, Ray & reflectRay, float &kr, Ray &refractRay, float &kt)
{
	kt = 0.0f;
	kr = 1.0f;
	Vector invRayDir(hitInfo.inRay.getDirection());
	Vector reflectedDir = invRayDir.reflect(hitInfo.hitNormal);
	reflectRay = Ray(hitInfo.hitPoint + reflectedDir * _RT_BIAS, reflectedDir, hitInfo.inRay.getDepth() + 1);
}

void MetallicMaterial::sampleScatterReflexionAndRefraction(HitInfo & hitInfo, Ray & reflectRay, float &kr, float &RPdf, Ray &refractRay, float &kt, float &TPdf)
{
	kt = 0.0f;
	kr = 1.0f;
	Vector invRayDir(hitInfo.inRay.getDirection());
	Vector reflectedDir = invRayDir.reflect(hitInfo.hitNormal);
	reflectRay = Ray(hitInfo.hitPoint + reflectedDir * _RT_BIAS, reflectedDir, hitInfo.inRay.getDepth() + 1);
	RPdf = 1.0f;
	TPdf = 0.0f;
}

// ======================================================================================

void GlassMaterial::scatterReflexionAndRefraction(HitInfo & hitInfo, Ray & reflectRay, float &kr, Ray &refractRay, float &kt)
{
	Vector refracted;
	float percentage;
	attemptToTransmitRay(hitInfo, refracted, percentage);

	kr = percentage;
	kt = 1.0f - percentage;

	if (kt > 0.0f)
	{
		refractRay = Ray(hitInfo.hitPoint + refracted * _RT_BIAS, refracted, hitInfo.inRay.getDepth() + 1);
	}

	if (kr > 0.0f)
	{
		Vector invRayDir(hitInfo.inRay.getDirection());
		Vector reflectedDir = invRayDir.reflect(hitInfo.hitNormal);
		reflectRay = Ray(hitInfo.hitPoint + hitInfo.hitNormal * _RT_BIAS, reflectedDir, hitInfo.inRay.getDepth() + 1);
	}
}

void GlassMaterial::sampleScatterReflexionAndRefraction(HitInfo & hitInfo, Ray & reflectRay, float &kr, float &RPdf, Ray &refractRay, float &kt, float &TPdf)
{
	Vector refracted;
	float percentage;
	attemptToTransmitRay(hitInfo, refracted, percentage);
	
	kr = percentage;
	kt = 1.0f - percentage;

	bool outside = hitInfo.inRay.getDirection().Dot(hitInfo.hitNormal) < 0;

	if (kt > 0.0f)
	{
		TPdf = kt;
		Vector transOrigin = outside ? hitInfo.hitPoint - (hitInfo.hitNormal * _RT_BIAS) : hitInfo.hitPoint + (hitInfo.hitNormal * _RT_BIAS);
		refractRay = Ray(transOrigin, refracted, hitInfo.inRay.getDepth() + 1);
	}

	if (kr > 0.0f)
	{
		RPdf = kr;
		Vector reflOrigin = outside ? hitInfo.hitPoint + (hitInfo.hitNormal * _RT_BIAS) : hitInfo.hitPoint - (hitInfo.hitNormal * _RT_BIAS);
		Vector invRayDir(hitInfo.inRay.getDirection());
		Vector reflectedDir = invRayDir.reflect(hitInfo.hitNormal);
		reflectRay = Ray(reflOrigin, reflectedDir, hitInfo.inRay.getDepth() + 1);
	}
}

bool GlassMaterial::computeSnellRefractedDirection(float inIOR, float outIOR, Vector inDir, Vector hitNormal, Vector & outDir)
{
	float inCosAngle = inDir.Dot(hitNormal); // We use the outgoing normal so we can compute the cosine without negating the incident ray direction

	if (inCosAngle > 0.0f)
	{
		hitNormal = hitNormal * -1.0f;
	}

	inCosAngle = fabs(inCosAngle);

	float ioIOR = inIOR / outIOR;

	// change sin(x) for 1 - cos^2(x)
	// pow refraction index factor to be able to operate with cosines
	float reflectance = 1.0f - (1.0f - inCosAngle * inCosAngle) * (ioIOR * ioIOR);
	if (reflectance > 0.0f)
	{
		outDir = ((inDir * ioIOR) + hitNormal * (inCosAngle * ioIOR - sqrt(reflectance))).Normalize();
		return true;
	}

	return false;
}

float GlassMaterial::computeFresnelReflectedEnergy(float iIOR, Vector inDir, Vector inNormal, float oIOR, Vector outDir, Vector outNormal)
{
	float cosi = fabs(inDir.Dot(outNormal));
	float coso = outDir.Dot(outNormal);
	
	float rs = (iIOR * cosi - oIOR * coso) / (iIOR * cosi + oIOR * coso);
	float rp = (iIOR * coso - oIOR * cosi) / (iIOR * coso + oIOR * cosi);

	return 0.5f * (rs*rs + rp*rp);
}

void GlassMaterial::attemptToTransmitRay(HitInfo & hitInfo, Vector & refracted, float &reflectedPercentage)
{
	Vector inDir = hitInfo.inRay.getDirection();
	Vector hitNormal = hitInfo.hitNormal;

	bool exiting = clampValue(inDir.Dot(hitNormal), -1.0f, 1.0f) > 0.0f;

	Vector outNormal = hitNormal;
	float inIOR = 1.0f, outIOR = hitInfo.hittedMaterial.refraction_index.x;

	if (exiting)
	{
		std::swap(inIOR, outIOR);
	}
	else
	{
		outNormal = outNormal * -1.0f;
	}

	if (computeSnellRefractedDirection(inIOR, outIOR, inDir, hitNormal, refracted))
	{
		reflectedPercentage = computeFresnelReflectedEnergy(inIOR, inDir, hitNormal, outIOR, refracted, outNormal);
	}
	else
	{
		reflectedPercentage = 1.0f;
	}
}

// ======================================================================================

Vector RoughMaterial::computeAmbientRadiance(HitInfo & hitInfo)
{
	return hitInfo.hittedMaterial.diffuse;
}

Vector RoughMaterial::computeDiffuseRadiance(HitInfo & hitInfo)
{
	return hitInfo.hittedMaterial.diffuse;
}

// ======================================================================================

PhysicalMaterialTable * PhysicalMaterialTable::INSTANCE = new PhysicalMaterialTable();

PhysicalMaterialTable::PhysicalMaterialTable()
{
	registerMaterial(new MatteMaterial());
	registerMaterial(new MetallicMaterial());
	registerMaterial(new GlassMaterial());
	registerMaterial(new RoughMaterial());
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