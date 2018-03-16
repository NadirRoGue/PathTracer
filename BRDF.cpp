#include "BRDF.h"

#include <time.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "Config.h"

// BRDF
bool DiffuseLambertian::value(HitInfo & hitInfo, Ray & scatteredRay, Vector & color)
{
	color = hitInfo.hittedMaterial.diffuse / float(M_PI);
	return false;
}

// Indirect lighting BRDF (computes scattered rays and pdf as well as color)
bool DiffuseLambertian::valueSample(HitInfo & hitInfo, Ray & scatteredRay, Vector & color, float & pdf)
{
	Vector zVector = hitInfo.hitNormal;
	Vector yVector, xVector;
	ComputeOrthoNormalBasis(zVector, yVector, xVector);

	Vector sample = sampler->sampleHemiSphere();
	Vector scatteredDir = (xVector * sample.x + yVector * sample.y + zVector * sample.z).Normalize();
	
	scatteredRay = Ray(hitInfo.hitPoint + zVector * _RT_BIAS, scatteredDir, hitInfo.inRay.getDepth() + 1);
	color = hitInfo.hittedMaterial.diffuse;
	pdf = zVector.Dot(scatteredDir);

	return pdf > 0.0f;
}

// ======================================================================================================================

bool SpecularPhong::value(HitInfo & hitInfo, Ray & scatteredRay, Vector & color)
{
	Vector reflected = hitInfo.lightVector.reflect(hitInfo.hitNormal);
	reflected = reflected * -1.0f;
	Vector invRayDir(hitInfo.inRay.getDirection());
	invRayDir = invRayDir * -1.0f;
	float dotRV = clampValue(reflected.Dot(invRayDir), 0.0f, 1.0f);

	if (dotRV > 0.0f)
	{
		color = (hitInfo.hittedMaterial.specular * pow(dotRV, hitInfo.hittedMaterial.shininess));// *(((hitInfo.hittedMaterial.shininess + 2.0f) / (2.0f*M_PI)));
	}

	return false;
}

bool SpecularPhong::valueSample(HitInfo & hitInfo, Ray & scatteredRay, Vector & color, float & pdf)
{
	// No specular highlights with indirect lighting
	return false;
}

// ======================================================================================================================

bool DielectricTransmissionFresnel::value(HitInfo & hitInfo, Ray & scatteredRay, Vector & color)
{
	Vector refracted;
	float percentage;

	if (AttemptToTransmitRay(hitInfo.inRay, hitInfo, refracted, percentage))
	{
		percentage = 1.0f - percentage;
		color = hitInfo.hittedMaterial.transparent * percentage;
		scatteredRay = Ray(hitInfo.hitPoint + refracted * _RT_BIAS, refracted, hitInfo.inRay.getDepth() + 1);
		return true;
	}

	return false;
}

bool DielectricTransmissionFresnel::valueSample(HitInfo & hitInfo, Ray & scatteredRay, Vector & color, float & pdf)
{
	Vector refracted;
	float percentage;

	if (AttemptToTransmitRay(hitInfo.inRay, hitInfo, refracted, percentage))
	{
		percentage = 1.0f - percentage;
		pdf = percentage;
		color = hitInfo.hittedMaterial.transparent * percentage;
		scatteredRay = Ray(hitInfo.hitPoint + refracted * _RT_BIAS, refracted, hitInfo.inRay.getDepth() + 1);
		return true;
	}
	return false;
}

// ======================================================================================================================

bool SpecularReflectanceFresnel::value(HitInfo & hitInfo, Ray & scatteredRay, Vector & color)
{
	Vector refracted;
	float reflectPercentage = 1.0f;
	
	AttemptToTransmitRay(hitInfo.inRay, hitInfo, refracted, reflectPercentage);

	if (reflectPercentage <= 0.0f)
	{
		return false;
	}

	Vector invRayDir(hitInfo.inRay.getDirection());
	Vector reflectedDir = invRayDir.reflect(hitInfo.hitNormal);
	scatteredRay = Ray(hitInfo.hitPoint + reflectedDir * _RT_BIAS, reflectedDir, hitInfo.inRay.getDepth() + 1);

	float factor = clampValue(scatteredRay.getDirection().Dot(hitInfo.hitNormal), 0.0f, 1.0f);
	color = hitInfo.hittedMaterial.reflective * reflectPercentage;

	return factor > 0.0f;
}

bool SpecularReflectanceFresnel::valueSample(HitInfo & hitInfo, Ray & scatteredRay, Vector & color, float & pdf)
{
	Vector refracted;
	float reflectPercentage = 1.0f;
	
	AttemptToTransmitRay(hitInfo.inRay, hitInfo, refracted, reflectPercentage);

	if (reflectPercentage <= 0.0f)
	{
		return false;
	}

	pdf = reflectPercentage;

	Vector invRayDir(hitInfo.inRay.getDirection());
	Vector reflectedDir = invRayDir.reflect(hitInfo.hitNormal);
	scatteredRay = Ray(hitInfo.hitPoint + reflectedDir * _RT_BIAS, reflectedDir, hitInfo.inRay.getDepth() + 1);

	float factor = clampValue(scatteredRay.getDirection().Dot(hitInfo.hitNormal), 0.0, 1.0);
	color = hitInfo.hittedMaterial.reflective * reflectPercentage;

	return factor > 0.0f;
}

// ======================================================================================================================

bool ComputeSnellRefractedDirection(float inIOR, float outIOR, Vector inDir, Vector outNormal, Vector & outDir)
{
	float inCosAngle = inDir.Dot(outNormal); // We use the outgoing normal so we can compute the cosine without negating the incident ray direction

	float ioIOR = inIOR / outIOR;

	// change sin(x) for 1 - cos^2(x)
	// pow refraction index factor to be able to operate with cosines
	float reflectance = 1.0f - (ioIOR * ioIOR) * (1.0f - inCosAngle * inCosAngle);
	if (reflectance > 0)
	{
		outDir = (inDir - outNormal*inCosAngle) * ioIOR - outNormal*sqrt(reflectance);
		return true;
	}

	return false;
}

float ComputeFresnelRefractedEnergy(float iIOR, Vector inDir, Vector inNormal, float oIOR, Vector outDir, Vector outNormal)
{
	float cosi = inDir.Dot(inNormal);
	float coso = outDir.Dot(outNormal);

	float rs = (oIOR * cosi - iIOR * coso) / (oIOR * cosi + iIOR * coso);
	float rp = (iIOR * cosi - oIOR * coso) / (iIOR * cosi + oIOR * coso);

	return ((rs*rs + rp*rp) * 0.5f);
}

bool AttemptToTransmitRay(const Ray & incidentRay, HitInfo & hitInfo, Vector & refracted, float &transmittedPercentage)
{
	Vector inNormal, outNormal;
	float inIOR, outIOR;

	//Vector d(incidentRay.getDirection());
	//d = d * -1.0f;
	bool entering = (incidentRay.getDirection()).Dot(hitInfo.hitNormal) < 0.0f;
	//bool entering = d.Dot(hitInfo.hitNormal) > 0.0f;
	if (entering)
	{
		inIOR = 1.0f;
		outIOR = hitInfo.hittedMaterial.refraction_index.x; // a vector, wtf?
		inNormal = hitInfo.hitNormal;
		outNormal = hitInfo.hitNormal * -1.0f;
	}
	else
	{
		inIOR = hitInfo.hittedMaterial.refraction_index.x;
		outIOR = 1.0f;
		inNormal = hitInfo.hitNormal * -1.0f;
		outNormal = hitInfo.hitNormal;// *-1.0f;
	}

	if (ComputeSnellRefractedDirection(inIOR, outIOR, incidentRay.getDirection(), inNormal, refracted))
	{
		transmittedPercentage = ComputeFresnelRefractedEnergy(inIOR, incidentRay.getDirection(), inNormal * -1.0f, outIOR, refracted, outNormal);
		return transmittedPercentage > 0.0f;
	}

	return false;
}

void ComputeOrthoNormalBasis(Vector zVector, Vector &yVector, Vector &xVector)
{
	yVector = Vector(0.1f, 1.0f, 1.0f).Cross(zVector).Normalize();
	xVector = yVector.Cross(zVector).Normalize();
}
