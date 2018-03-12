#include "BRDF.h"

#include <time.h>
#define _USE_MATH_DEFINES
#include <math.h>

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

bool SpecularPhong::evaluate(const Ray & incidentRay, HitInfo & hitInfo, const Vector & lightVector, Ray & scatteredRay, Vector & color)
{
	Vector reflected = lightVector.reflect(hitInfo.hitNormal);
	reflected = reflected * -1.0f;
	Vector invRayDir(incidentRay.getDirection());
	invRayDir = invRayDir * -1.0f;
	float dotRV = clampValue(reflected.Dot(invRayDir), 0.0f, 1.0f);

	if (dotRV > 0.0f)
	{
		color = (hitInfo.hittedMaterial.specular * pow(dotRV, hitInfo.hittedMaterial.shininess));
	}

	return false;
}

bool Dielectric::evaluate(const Ray & incidentRay, HitInfo & hitInfo, const Vector & lightVector, Ray & scatteredRay, Vector & color)
{

	return false;
}

