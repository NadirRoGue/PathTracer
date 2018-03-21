#include <time.h>

#include "Tracer.h"
#include "Config.h"
#include "PhysicalMaterial.h"

// =====================================================================

// Computes the necessary data to cast rays from camera before starting rendering
void Tracer::init()
{
	wrapper.wrap(scene->GetCamera(), Scene::WINDOW_WIDTH, Scene::WINDOW_HEIGHT);
}

// Checks whether the given ray intersect with any scene geometry
HitInfo Tracer::intersect(const Ray & ray)
{
	HitInfo info;

	// Initialize to false. If no objects are hit, it will remain as no hit at the end
	HitInfo closer;
	closer.hit = false;

	Vector camPos = scene->GetCamera().GetPosition();

	// Iterate over all scene objects
	for (unsigned int i = 0; i < scene->GetNumObjects(); i++)
	{
		SceneObject * object = scene->GetObject(i);

		object->testIntersection(ray, info);
		if (info.hit)
		{
			if (closer.hit)
			{
				// After 2+ hits, check to keep the closer one
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

// Checks whether the light can be seen from the hitPoint contained in the HitInfo struct
Vector Tracer::lightContribution(HitInfo & info, Vector & lightVector, SceneLight * light)
{
	float distToLight = lightVector.Magnitude();
	lightVector = lightVector.Normalize();

	const unsigned int sceneObjectCount = scene->GetNumObjects();

	Ray lightVisibilityTest(info.hitPoint + lightVector * _RT_BIAS, lightVector);
	HitInfo visibilityInfo;
	bool visible = true;
	// Iterate over all scene objects to check for occlusions
	for (unsigned int i = 0; i < sceneObjectCount && visible; i++)
	{
		SceneObject * so = scene->GetObject(i);

		so->testIntersection(lightVisibilityTest, visibilityInfo);
		
		if (visibilityInfo.isLight)
			continue;

		if (visibilityInfo.hit)
		{
			// check whether light or occluder is closer
			float distanceToHit = (visibilityInfo.hitPoint - info.hitPoint).Magnitude();
			if (distanceToHit < distToLight)
			{
				visible = false;
			}
		}
	}

	// If visible, return attenuated light color
	if (visible)
	{
		float squaredDist = distToLight * distToLight;
		return (light->color / (light->attenuationConstant + light->attenuationLinear * distToLight + light->attenuationQuadratic * squaredDist));
	}
	else // return black otherwise
	{
		return Vector();
	}
}

// =====================================================================

// Launchs a ray from the camara given the screen pixel coordinates
Vector RayTracer::doTrace(int screenX, int screenY)
{
	float t = float(screenX) / float(Scene::WINDOW_WIDTH);
	float s = float(screenY) / float(Scene::WINDOW_HEIGHT);

	Ray ray = wrapper.getRayForPixel(t, s);

	return shade(ray);
}

// Return the color of a given point by casting a ray in a direction from that point
// and accumulating the radiance
Vector RayTracer::shade(const Ray & ray)
{
	HitInfo info;

	// Check whether this ray has already reached max depth
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
			// Clearly signal an object without proper material
			return Vector(1.0, 0.0, 1.0);
		}

		// Direct lighting
		for (unsigned int i = 0; i < scene->GetNumLights(); i++)
		{
			Vector diffuseC;
			SceneLight * sl = scene->GetLight(i);

			lightVector = (sl->position - info.hitPoint);
			I = lightContribution(info, lightVector, sl) / sl->color.Magnitude();
			
			lightVector.Normalize();
			info.lightVector = lightVector;
			float cosValue = clampValue(info.hitNormal.Dot(lightVector), 0.0f, 1.0f);

			// Diffuse reflectance
			diffuseC = BRDF->computeDiffuseRadiance(info);

			Lr = Lr + (I * diffuseC * cosValue) / float(M_PI);
		}

		// Specular reflection
		float kr, kt;
		Ray reflected, refracted;
		BRDF->scatterReflexionAndRefraction(info, reflected, kr, refracted, kt);

		if (kr > 0.0f)
		{
			Lr = Lr + (averageMaterialAtPoint.reflective * kr) * shade(reflected);
		}

		if (kt > 0.0f)
		{
			Lr = Lr + (averageMaterialAtPoint.transparent * kt) * shade(refracted);
		}

		// Ambient lighting
		Lr = Lr + BRDF->computeAmbientRadiance(info) * scene->GetBackground().ambientLight;

		return Lr;
	}
	else
	{
		return scene->GetBackground().color;
	}
}

// =====================================================================

// Ray tracing but tracing 100 rays per pixel using a non uniform random "sampler"
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

	// Monte carlo AA
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
	if (ray.getDepth() > _RT_RUSSIAN_ROULETE_MIN_BOUNCE && ray.getCosineWeight() >= 0.0f)
	{
		float p = russianRouletteSampler.sampleRect();

		if (p > ray.getCosineWeight())
		{
			return Vector();
		}
	}

	HitInfo info;

	if (ray.getDepth() < _RT_MAX_BOUNCES && (info = intersect(ray)).hit)
	{
		// If its a light, return the color and stop bouncing
		if (info.isLight)
		{
			return info.emission;
		}

		SceneMaterial averageMaterialAtPoint = info.hittedMaterial;
		Vector diffuseC = averageMaterialAtPoint.diffuse;

		// If this ray passed the roulette test, divide by its probability
		if (ray.getCosineWeight() > 0.0f)
		{
			diffuseC = diffuseC / ray.getCosineWeight();
		}

		Vector Lr;
		Ray scattered;
		Vector lightVector;
		Vector I;

		// Purple color to identify wrong setted scene objects
		PhysicalMaterial * BRDF = PhysicalMaterialTable::getInstance().getMaterialByName(info.physicalMaterial);
		if (BRDF == NULL)
		{
			return Vector(1.0, 0.0, 1.0);
		}

		// Direct lighting
		for (unsigned int i = 0; i < scene->GetNumLights(); i++)
		{
			// Get light vector by sampling a point in the light
			// (for point light its always the same point)
			Vector diffuseC, specularC;
			SceneLight * sl = scene->GetLight(i);

			float dirPdf;
			lightVector = sl->sampleDirection(info.hitPoint, dirPdf);

			if (dirPdf == 0.0f)
			{
				continue;
			}

			I = lightContribution(info, lightVector, sl);
			info.lightVector = lightVector;
			float cosValue = clampValue(info.hitNormal.Dot(lightVector), 0.0f, 1.0f);

			// Diffuse reflectance
			diffuseC = BRDF->computeDiffuseRadiance(info);

			// Diffuse - Diffuse light transport
			Vector indirectLighting;
			for (unsigned int s = 0; s < _RT_MC_BOUNCES_SAMPLES; s++)
			{
				float dPdf;
				if(BRDF->sampleDiffuseRadiance(info, scattered, Vector(), dPdf))
				{
					indirectLighting = indirectLighting + (shade(scattered) / dPdf);
				}
			}

			indirectLighting = indirectLighting / _RT_MC_BOUNCES_SAMPLES;

			// Compute total radiance. Only direct lighting is multiplied by cosine because
			// it has been optimized to reduce computation needed for diffuse and indirect lighting pdf's
			Lr = (Lr + (((I * cosValue) / float(M_PI) + indirectLighting) * diffuseC) / (dirPdf));
			fixGammut(Lr);
		}

		// REFLECTION AND REFRACTION
		float kr, RPdf, kt, TPdf;
		Ray reflected, refracted;
		BRDF->sampleScatterReflexionAndRefraction(info, reflected, kr, RPdf, refracted, kt, TPdf);

		// Russian roulette depth reached and material has both reflection and refraction
		if (ray.getDepth() > _RT_RUSSIAN_ROULETE_MIN_BOUNCE && kr > 0.0f && kt > 0.0f)
		{
			float reflectiveProbability = russianRouletteSampler.sampleRect();
			if (kr > reflectiveProbability)
			{
				Lr = Lr + (averageMaterialAtPoint.reflective * kr) * shade(reflected) / kr;
			}
			else
			{
				Lr = Lr + (averageMaterialAtPoint.transparent * kt) * shade(refracted) / (1 - kr);
			}
		}
		else  // No depth enough to apply russian roulette
		{
			if (kr > 0.0f)
			{
				Lr = Lr + (averageMaterialAtPoint.reflective * kr) * shade(reflected);
			}
			
			if(kt > 0.0f)
			{
				Lr = Lr + (averageMaterialAtPoint.transparent * kt) * shade(refracted);
			}
		}

		return Lr;
	}
	else
	{
		return scene->GetBackground().color;
	}
}

void MonteCarloRayTracer::samplePixel(int x, int y, float &st, float &ss, float &pdf)
{
	Vector sample = pixelSampler.samplePlane();
	float sampledPixelX = float(x) + ((sample.x * 2.0f) - 1.0f);
	float sampledPixelY = float(y) + ((sample.y * 2.0f) - 1.0f);

	st = float(sampledPixelX) / float(Scene::WINDOW_WIDTH);
	ss = float(sampledPixelY) / float(Scene::WINDOW_HEIGHT);

	//uniform distributed pdf($) = 1 / (b - a)
	//pdf(x) = 1 / (1 - 0) = 1
	//pdf(y) = 1 / (1 - 0) = 1
	//pdf = 1 * 1 = 1
	pdf = 1.0f;
}