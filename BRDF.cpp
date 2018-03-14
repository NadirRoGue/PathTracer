#include "BRDF.h"

#include <time.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "Config.h"

bool DiffuseLambertian::evaluate(const Ray & incidentRay, HitInfo & hitInfo, const Vector & lightVector, Ray & scatteredRay, Vector & color)
{
	color = hitInfo.hittedMaterial.diffuse / float(M_PI);
	
	/*
	if (incidentRay.getDepth() < MAX_BOUNCES)
	{
		Vector scatteredDir = hitInfo.hitNormal + randomPointInSphere();
		scatteredRay = Ray(hitInfo.hitPoint + scatteredDir * PRECISSION_EPSILON, scatteredDir);
		return true;
	}
	*/

	return false;
}

// From Ray Tracing in One Weekend
Vector DiffuseLambertian::randomPointInSphere()
{
	srand(unsigned int(time(NULL)));
	Vector result;
	Vector fix(1.0f, 1.0f, 1.0f);
	do
	{
		float xRand = float(rand() % 1000) / 1000.0f;
		float yRand = float(rand() % 1000) / 1000.0f;
		float zRand = float(rand() % 1000) / 1000.0f;
		result = Vector(xRand, yRand, zRand) * 2.0f - fix;
	} while (result.Dot(result) >= 1.0f);

	return result;
	
}

// ======================================================================================================================

bool SpecularPhong::evaluate(const Ray & incidentRay, HitInfo & hitInfo, const Vector & lightVector, Ray & scatteredRay, Vector & color)
{
	Vector reflected = lightVector.reflect(hitInfo.hitNormal);
	reflected = reflected * -1.0f;
	Vector invRayDir(incidentRay.getDirection());
	invRayDir = invRayDir * -1.0f;
	float dotRV = clampValue(reflected.Dot(invRayDir), 0.0f, 1.0f);

	if (dotRV > 0.0f)
	{
		color = (hitInfo.hittedMaterial.specular * pow(dotRV, hitInfo.hittedMaterial.shininess));// *(((hitInfo.hittedMaterial.shininess + 2.0f) / (2.0f*M_PI)));
	}

	return false;
}

// ======================================================================================================================

bool DielectricTransmissionFresnel::evaluate(const Ray & incidentRay, HitInfo & hitInfo, const Vector & lightVector, Ray & scatteredRay, Vector & color)
{
	Vector refracted;
	float percentage;

	if (AttemptToTransmitRay(incidentRay, hitInfo, refracted, percentage))
	{
		color = hitInfo.hittedMaterial.transparent * percentage;
		scatteredRay = Ray(hitInfo.hitPoint + refracted * _RT_BIAS, refracted, incidentRay.getDepth() + 1);
		return true;
	}

	return false;
	/*
	bool entering = incidentRay.getDirection().Dot(hitInfo.hitNormal) < 0.0f;
	if (entering)
	{
		inIOR = 1.0f;
		outIOR = hitInfo.hittedMaterial.refraction_index.x; // a vector, wtf?
		outNormal = hitInfo.hitNormal;
	}
	else
	{
		inIOR = hitInfo.hittedMaterial.refraction_index.x;
		outIOR = 1.0f;
		outNormal = hitInfo.hitNormal * -1.0f;
	}

	if (ComputeSnellRefractedDirection(inIOR, outIOR, incidentRay.getDirection(), outNormal, refracted))
	{
		float refractedPercentage = ComputeFresnelRefractedEnergy(inIOR, incidentRay.getDirection(), outIOR, refracted, outNormal);
		if(refractedPercentage > 0.0f)
		{
			color = hitInfo.hittedMaterial.transparent * refractedPercentage;
			scatteredRay = Ray(hitInfo.hitPoint + refracted * _RT_BIAS, refracted, incidentRay.getDepth() + 1);
			return true;
		}
	}

	return false;*/
}

// ======================================================================================================================

bool SpecularReflectanceFresnel::evaluate(const Ray & incidentRay, HitInfo & hitInfo, const Vector & lightVector, Ray & scatteredRay, Vector & color)
{
	Vector refracted;
	float reflectPercentage = 1.0f;
	float percentage;

	if (AttemptToTransmitRay(incidentRay, hitInfo, refracted, percentage))
	{
		reflectPercentage -= percentage;
	}

	if (reflectPercentage <= 0.0f)
	{
		return false;
	}

	Vector invRayDir(incidentRay.getDirection());
	Vector reflectedDir = invRayDir.reflect(hitInfo.hitNormal);
	scatteredRay = Ray(hitInfo.hitPoint + reflectedDir * _RT_BIAS, reflectedDir, incidentRay.getDepth() + 1);

	float factor = clampValue(scatteredRay.getDirection().Dot(hitInfo.hitNormal), 0.0, 1.0);
	color = hitInfo.hittedMaterial.specular * reflectPercentage;

	return factor > 0.0f;

	/*

	// By default we reflect all energy. If we manage to refract, we will substract the refrected percentage
	float reflectedPercentage = 1.0f;
	if (hitInfo.hittedMaterial.refraction_index.x > 0.0f)
	{
		bool entering = incidentRay.getDirection().Dot(hitInfo.hitNormal) < 0.0f;
		if (entering)
		{
			inIOR = 1.0f;
			outIOR = hitInfo.hittedMaterial.refraction_index.x; // a vector, wtf?
			inNormal = hitInfo.hitNormal * -1.0f;
			outNormal = hitInfo.hitNormal;
		}
		else
		{
			inIOR = hitInfo.hittedMaterial.refraction_index.x;
			outIOR = 1.0f;
			inNormal = hitInfo.hitNormal;
			outNormal = hitInfo.hitNormal * -1.0f;
		}

		if (ComputeSnellRefractedDirection(inIOR, outIOR, incidentRay.getDirection(), outNormal, refracted))
		{
			// Substract refracted energy percentage
			reflectedPercentage -= ComputeFresnelRefractedEnergy(inIOR, incidentRay.getDirection(), inNormal, outIOR, refracted, outNormal);
		}
	}

	Vector invRayDir(incidentRay.getDirection());
	Vector reflectedDir = invRayDir.reflect(hitInfo.hitNormal);
	scatteredRay = Ray(hitInfo.hitPoint + reflectedDir * _RT_BIAS, reflectedDir, incidentRay.getDepth() + 1);

	float factor = clampValue(scatteredRay.getDirection().Dot(hitInfo.hitNormal), 0.0, 1.0);
	color = hitInfo.hittedMaterial.specular * reflectedPercentage;

	return factor > 0.0f;*/
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

	return 1.0f - ((rs*rs + rp*rp) * 0.5f);
}

bool AttemptToTransmitRay(const Ray & incidentRay, HitInfo & hitInfo, Vector & refracted, float &transmittedPercentage)
{
	Vector inNormal, outNormal;
	float inIOR, outIOR;

	bool entering = incidentRay.getDirection().Dot(hitInfo.hitNormal) < 0.0f;
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
