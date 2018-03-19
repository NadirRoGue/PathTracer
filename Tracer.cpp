#include <time.h>

#include "Tracer.h"
#include "Config.h"
#include "PhysicalMaterial.h"

// =====================================================================

void Tracer::init()
{
	wrapper.wrap(scene->GetCamera(), Scene::WINDOW_WIDTH, Scene::WINDOW_HEIGHT);
}

HitInfo Tracer::intersect(const Ray & ray)
{
	HitInfo info;
	HitInfo closer;
	closer.hit = false;

	Vector camPos = scene->GetCamera().GetPosition();

	for (unsigned int i = 0; i < scene->GetNumObjects(); i++)
	{
		SceneObject * object = scene->GetObject(i);

		object->testIntersection(ray, info);
		if (info.hit)
		{
			if (closer.hit)
			{
				float closerDist = (camPos - closer.hitPoint).Magnitude();
				float currentDist = (camPos - info.hitPoint).Magnitude();

				if (closerDist <= currentDist)
				{
					continue;
				}
			}
			closer = info;
		}
	}

	return closer;
}

Vector Tracer::lightContribution(HitInfo & info, Vector & lightVector, SceneLight * light)
{
	float distToLight = lightVector.Magnitude();
	lightVector = lightVector.Normalize();

	const unsigned int sceneObjectCount = scene->GetNumObjects();

	Ray lightVisibilityTest(info.hitPoint + lightVector * _RT_BIAS, lightVector);
	HitInfo visibilityInfo;
	bool visible = true;
	for (unsigned int i = 0; i < sceneObjectCount && visible; i++)
	{
		SceneObject * so = scene->GetObject(i);

		so->testIntersection(lightVisibilityTest, visibilityInfo);
		if (visibilityInfo.hit)
		{
			float distanceToHit = (visibilityInfo.hitPoint - info.hitPoint).Magnitude();
			if (distanceToHit < distToLight)
			{
				visible = false;
			}
		}
	}

	if (visible)
	{
		float squaredDist = distToLight * distToLight;
		return (light->color / (light->attenuationConstant + light->attenuationLinear * distToLight + light->attenuationQuadratic * squaredDist));
	}
	else
	{
		return Vector();
	}
}

// =====================================================================

Vector RayTracer::doTrace(int screenX, int screenY)
{
	float t = float(screenX) / float(Scene::WINDOW_WIDTH);
	float s = float(screenY) / float(Scene::WINDOW_HEIGHT);

	Ray ray = wrapper.getRayForPixel(t, s);

	return shade(ray);
}

Vector RayTracer::shade(const Ray & ray)
{
	HitInfo info;

	if (ray.getDepth() < _RT_MAX_BOUNCES && (info = intersect(ray)).hit)
	{
		SceneMaterial averageMaterialAtPoint = info.hittedMaterial;
		Vector Lr;
		Ray scattered;
		Vector lightVector;
		Vector I;

		PhysicalMaterial * BRDF = PhysicalMaterialTable::getInstance().getMaterialByName(info.physicalMaterial);
		if (BRDF == NULL)
		{
			return Vector(1.0, 0.0, 1.0);
		}

		// Direct lighting
		for (unsigned int i = 0; i < scene->GetNumLights(); i++)
		{
			Vector diffuseC, specularC;
			SceneLight * sl = scene->GetLight(i);

			lightVector = (sl->position - info.hitPoint);
			I = lightContribution(info, lightVector, sl);

			lightVector.Normalize();
			info.lightVector = lightVector;
			float cosValue = clampValue(info.hitNormal.Dot(lightVector), 0.0f, 1.0f);

			// Diffuse reflectance
			if (BRDF->computeDiffuseRadiance(info, scattered, diffuseC))
			{
				diffuseC = diffuseC * shade(scattered);
			}

			// Specular reflectance
			if (BRDF->computeSpecularRadiance(info, scattered, specularC))
			{
				specularC = specularC * shade(scattered);
			}

			Lr = Lr + (I * (diffuseC + specularC) * cosValue);
		}

		// Specular reflection
		Vector reflexion;
		if (BRDF->scatterReflexion(info, scattered, reflexion))
		{
			Lr = Lr + (reflexion * shade(scattered));
		}

		// Refraction
		Vector refracted;
		if (BRDF->scatterTransmission(info, scattered, refracted))
		{
			Lr = Lr + (refracted * shade(scattered));
		}

		// Ambient lighting
		Lr = Lr + BRDF->computeAmbientRadiance(info) * scene->GetBackground().ambientLight;

		//fixGammut(Lr);

		return Lr;
	}
	else
	{
		return scene->GetBackground().color;
	}
}

// =====================================================================

Vector SuperSamplingRayTracer::doTrace(int screenX, int screenY)
{
	srand(unsigned int(time(NULL)));

	Vector color;
	Ray ray;
	static float max = 1.0 - FLT_EPSILON;
	float rand1, rand2, t, s;

	for (unsigned int pass = 0; pass < _RT_SUPERSAMPLING_SAMPLES; pass++)
	{
		rand1 = float(rand() % _RT_RAND_PRECISSION) / float(_RT_RAND_PRECISSION);
		rand2 = float(rand() % _RT_RAND_PRECISSION) / float(_RT_RAND_PRECISSION);

		t = (float(screenX) + rand1 * max) / float(Scene::WINDOW_WIDTH);
		s = (float(screenY) + rand2 * max) / float(Scene::WINDOW_HEIGHT);

		ray = wrapper.getRayForPixel(t, s);

		color = color + shade(ray);
	}

	return (color / float(_RT_SUPERSAMPLING_SAMPLES));
}

// =============================================================================

Vector MonteCarloRayTracer::doTrace(int screenX, int screenY)
{
	Vector pixelColor;
	float st, ss;
	float pdf;
	Ray ray;

	for (unsigned int i = 0; i < _RT_MC_PIXEL_SAMPLES; i++)
	{
		samplePixel(screenX, screenY, st, ss, pdf);
		ray = wrapper.getRayForPixel(st, ss);

		pixelColor = pixelColor + shade(ray) / pdf;
	}

	pixelColor = pixelColor / _RT_MC_PIXEL_SAMPLES;

	return pixelColor;
}

Vector MonteCarloRayTracer::shade(const Ray & ray)
{
	HitInfo info;

	if (ray.getDepth() < _RT_MAX_BOUNCES && (info = intersect(ray)).hit)
	{
		if (info.isLight)
		{
			return info.emission;
		}

		SceneMaterial averageMaterialAtPoint = info.hittedMaterial;
		Vector Lr;
		Ray scattered;
		Vector lightVector;
		Vector I;

		PhysicalMaterial * BRDF = PhysicalMaterialTable::getInstance().getMaterialByName(info.physicalMaterial);
		if (BRDF == NULL)
		{
			return Vector(1.0, 0.0, 1.0);
		}

		// Direct lighting
		for (unsigned int i = 0; i < scene->GetNumLights(); i++)
		{
			Vector diffuseC, specularC;
			SceneLight * sl = scene->GetLight(i);

			float dirPdf;
			lightVector = sl->sampleDirection(info.hitPoint, dirPdf);

			if (dirPdf == 0.0f)
				continue;

			I = lightContribution(info, lightVector, sl);
			info.lightVector = lightVector;
			float cosValue = clampValue(info.hitNormal.Dot(lightVector), 0.0f, 1.0f);

			// Diffuse reflectance
			BRDF->computeDiffuseRadiance(info, scattered, diffuseC);

			Vector currentShading = (I * cosValue);
			if (ray.getDepth() > _RT_RUSSIAN_ROULETE_MIN_BOUNCE)
			{
				Vector tempShading = currentShading * diffuseC;
				float max = std::max(tempShading.x, std::max(tempShading.y, tempShading.z));
				float p = d(engine);

				if (p > max)
				{
					return tempShading;
				}
			}

			Vector indirectLighting;
			for (unsigned int s = 0; s < _RT_MC_BOUNCES_SAMPLES; s++)
			{
				Ray difRay;
				float dPdf;
				if(BRDF->sampleDiffuseRadiance(info, difRay, Vector(), dPdf))
				{
					//Vector shaded = shade(difRay);
					indirectLighting = indirectLighting + (shade(difRay) / dPdf);
				}
			}

			indirectLighting = indirectLighting / _RT_MC_BOUNCES_SAMPLES;

			Lr = (Lr + ((currentShading + indirectLighting) * diffuseC) / (dirPdf));
			fixGammut(Lr);
		}

		// Specular reflection
		Vector reflexion;
		if (BRDF->scatterReflexion(info, scattered, reflexion))
		{
			Lr = Lr + (reflexion * shade(scattered));
		}

		// Refraction
		Vector refracted;
		if (BRDF->scatterTransmission(info, scattered, refracted))
		{
			Lr = Lr + (refracted * shade(scattered));
		}

		//Lr = Lr + BRDF->computeAmbientRadiance(info) * scene->GetBackground().ambientLight;

		return Lr;
	}
	else
	{
		return scene->GetBackground().color;
	}
}

void MonteCarloRayTracer::samplePixel(int x, int y, float &st, float &ss, float &pdf)
{
	Vector sample = pixelSampler->samplePlane();
	float sampledPixelX = float(x) + sample.x;
	float sampledPixelY = float(y) + sample.y;

	st = float(sampledPixelX) / float(Scene::WINDOW_WIDTH);
	ss = float(sampledPixelY) / float(Scene::WINDOW_HEIGHT);
	pdf = 1.0f; // / float(pixelSampler->getNumSamples());
}