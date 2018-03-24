#include "PhysicalMaterial.h"

#include "Config.h"
#include <algorithm>

// =======================================================================================
// Matte

Vector MatteMaterial::computeAmbientRadiance(HitInfo & hitInfo)
{
	return hitInfo.hittedMaterial.diffuse;
}

Vector MatteMaterial::computeDiffuseRadiance(HitInfo & hitInfo)
{
	// OPTIMIZATION: Cancel PI term and apply only to direct lighting
	return hitInfo.hittedMaterial.diffuse / float(M_PI);
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
	pdf = 1.0f / (2.0f * float(M_PI));
	result = computeDiffuseRadiance(hitInfo);
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
	Vector diffuseTerm =  hitInfo.hittedMaterial.diffuse / float(M_PI);

	Vector n = hitInfo.hitNormal;
	Vector l = hitInfo.lightVector;
	Vector v = Vector(hitInfo.inRay.getDirection()) * -1.0f;
	Vector h = (l + v).Normalize();
	float roughness = hitInfo.hittedMaterial.roughness;

	float F = conductorFresnel(l, h, hitInfo.hittedMaterial.refraction_index.x); // m o h
	float G = geometricSmithSchlick(v, h, n, l, roughness); // m o h
	float D = distributionBeckman(h, n, roughness); // m o h

	float bottom = 4.0f * std::max(0.0f, n.Dot(l)) * std::max(0.0f, n.Dot(v));
	
	float fr = 0.0f;
	if (bottom > 0.0f)
	{
		fr = std::max((F * G * D) / bottom, 0.0f);
	}

	// Energy conservation
	float diffuseFresnelV = 1.0f - F;

	return diffuseTerm * diffuseFresnelV + Vector(1.0f, 1.0f, 1.0f) * fr;
}

bool RoughMaterial::sampleDiffuseRadiance(HitInfo & hitInfo, Ray & scatteredRay, Vector &result, float &pdf)
{
	float roughness = hitInfo.hittedMaterial.roughness;

	Vector v = hitInfo.inRay.getDirection();
	Vector invV = v * -1.0f;
	Vector n = hitInfo.hitNormal;
	Vector m = sampleMicrofacetNormal(n, roughness);
	Vector l = hitInfo.lightVector;
	Vector h = (m + invV).Normalize();

	Vector scatteredDir = v.reflect(m);

	float F = conductorFresnel(scatteredDir, h, hitInfo.hittedMaterial.refraction_index.x);
	float G = geometricSmithSchlick(invV, h, n, scatteredDir, roughness);
	
	float bottom = clampValue(4.0f * n.Dot(scatteredDir) * n.Dot(invV), 0.0f, 1.0f);

	float cos = clampValue(m.Dot(n), 0.0f, 1.0f);
	float sin = sqrtf(1.0f - cos*cos);

	float fr = 0.0f;
	if (bottom > 0.0f)
	{
		fr = std::max((F * G * sin) / bottom, 0.0f);
	}

	// Energy conservation
	float diffuseFresnelV = 1.0f - conductorFresnel(scatteredDir, n, hitInfo.hittedMaterial.refraction_index.x);

	result = hitInfo.hittedMaterial.diffuse * diffuseFresnelV + Vector(1.0f, 1.0f, 1.0f) * fr;

	//std::cout << result.x << ", " << result.y << ", " << result.z << std::endl;

	scatteredRay = Ray(hitInfo.hitPoint + scatteredDir * _RT_BIAS, scatteredDir, hitInfo.inRay.getDepth() + 1);
	scatteredRay.setWeight(fabs(m.Dot(scatteredDir)));
	
	//pdf = 1.0f / roughness;
	pdf = fabs(m.Dot(n));

	return pdf > 0.0f;
}

float RoughMaterial::computeXi(float a)
{
	return a > 0.0f? 1.0f : 0.0f;
}

float RoughMaterial::geometricSmithSchlick(Vector v, Vector h, Vector n, Vector l, float roughness)
{
	float cosnv = n.Dot(v);
	float cosnl = n.Dot(l);
	float coshv = h.Dot(v);
	float coshl = h.Dot(l);

	float clampCosNv = std::max(0.0f, cosnv);

	float tannv = tanf(acosf(clampCosNv));

	float a = 1.0f / (roughness * tannv);

	// Shlick aproximation
	float result = 1.0f;
	if (a < 1.6f)
	{
		result = (3.535f*a + 2.181f*a*a) / (1.0f + 2.276f*a + 2.577f*a*a);
	}

	return (computeXi(coshl / cosnl) * result) * (computeXi(coshv / cosnv) * result);
}

float RoughMaterial::conductorFresnel(Vector l, Vector h, float ior)
{
	float R0 = (1.0f - ior) / (1.0f + ior);
	R0 *= R0;

	float cosTheta = 1.0f - fabs(l.Dot(h));

	return R0 + (1.0f - R0)*cosTheta*cosTheta*cosTheta*cosTheta*cosTheta;
}

float RoughMaterial::distributionBeckman(Vector h, Vector n, float roughness)
{
	float dotnh = std::max(0.0f, (n.Dot(h)));
	
	if (dotnh == 0.0f)
		return 0.0f;

	if (roughness <= 0.0f)
		return 0.0f;

	float dotnh2 = dotnh * dotnh;
	float m2 = roughness * roughness;
	float expValue = (dotnh2 - 1.0f) / (m2*dotnh2);

	float e = std::exp(expValue);
	
	return e / (float(M_PI)*m2*dotnh2*dotnh2);
}

Vector RoughMaterial::sampleMicrofacetNormal(Vector n, float roughness)
{
	float a = sampler.sampleRect();
	float b = sampler.sampleRect();

	float theta = atan(sqrtf(-(roughness*roughness)*log(1.0f - a)));
	float phi = 2.0f * float(M_PI) * b;

	float sinTheta = sinf(theta);
	float x = sinTheta * cosf(phi);
	float z = sinTheta * sinf(phi);

	Vector local(x, a, z);

	Vector yVector, xVector;
	ComputeOrthoNormalBasis(n, yVector, xVector);

	return WorldUniformHemiSample(local, n, yVector, xVector).Normalize();
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