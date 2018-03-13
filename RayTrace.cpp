#ifdef _OS_X_
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>	

#elif defined(WIN32)
#include <windows.h>
#include "GL/gl.h"
#include "GL/glu.h"
#include "GL/glut.h"

#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include <time.h>

#include "PhysicalMaterial.h"
#include "Scene.h"
#include "RayTrace.h"



void RayTrace::preRenderInit()
{
	wrapper.wrap(m_Scene.GetCamera(), Scene::WINDOW_WIDTH, Scene::WINDOW_HEIGHT);
}

// -- Main Functions --
// - CalculatePixel - Returns the Computed Pixel for that screen coordinate
Vector RayTrace::CalculatePixel(int screenX, int screenY)
{
	if ((screenX < 0 || screenX > Scene::WINDOW_WIDTH - 1) ||
		(screenY < 0 || screenY > Scene::WINDOW_HEIGHT - 1))
	{
		// Off the screen, return black
		return Vector(0.0f, 0.0f, 0.0f);
	}

	if (m_Scene.supersample)
	{
		return DoSuperSamplingRayTrace(screenX, screenY);
	}
	else if(m_Scene.montecarlo)
	{
		return DoMonteCarloRayTrace(screenX, screenY);
	}
	else
	{
		return DoRayTrace(screenX, screenY);
	}
}

// ===================================================================================

Vector RayTrace::DoRayTrace(int screenX, int screenY)
{
	Scene &la_escena = m_Scene;
	Camera &la_camara = la_escena.GetCamera();

	float t = float(screenX) / float(Scene::WINDOW_WIDTH);
	float s = float(screenY) / float(Scene::WINDOW_HEIGHT);

	Ray ray = wrapper.getRayForPixel(t, s);

	return Shade(ray);
}

// ===================================================================================

Vector RayTrace::DoSuperSamplingRayTrace(int screenX, int screenY)
{
	Scene &la_escena = m_Scene;
	Camera &la_camara = la_escena.GetCamera();

	srand(time(NULL));

	Vector color;

	static float max = 1.0 - FLT_EPSILON;

	Ray ray;

	float rand1, rand2, t, s;

	unsigned int samples = 100;

	static unsigned int numObjects = la_escena.GetNumObjects();

	for (unsigned int pass = 0; pass < samples; pass++)
	{
		rand1 = float(rand() % 1000) / 1000.0f;
		rand2 = float(rand() % 1000) / 1000.0f;

		t = (float(screenX) + rand1 * max) / float(Scene::WINDOW_WIDTH);
		s = (float(screenY) + rand2 * max) / float(Scene::WINDOW_HEIGHT);

		ray = wrapper.getRayForPixel(t, s);

		color = color + Shade(ray);
	}

	return (color / float(samples));
}

// ===================================================================================


Vector RayTrace::DoMonteCarloRayTrace(int screenX, int screenY)
{
	return Vector(1, 0, 1);
}

// ===================================================================================

HitInfo RayTrace::Intersect(const Ray & ray)
{
	HitInfo info;
	HitInfo closer;
	closer.hit = false;

	Vector camPos = m_Scene.GetCamera().GetPosition();

	for (unsigned int i = 0; i < m_Scene.GetNumObjects(); i++)
	{
		SceneObject * object = m_Scene.GetObjectW(i);

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

Vector RayTrace::Shade(const Ray & ray)
{
	HitInfo info;
	
	if (ray.getDepth() < MAX_BOUNCES && (info = Intersect(ray)).hit)
	{
		SceneMaterial averageMaterialAtPoint = info.hittedMaterial;
		Vector Lr;
		Ray scattered;
		Vector lightVector;
		Vector lightContribution;

		PhysicalMaterial * BRDF = PhysicalMaterialTable::getInstance().getMaterialByName(info.physicalMaterial);
		if (BRDF == NULL)
		{
			return Vector();
		}

		// Direct lighting
		for (unsigned int i = 0; i < m_Scene.GetNumLights(); i++)
		{
			Vector diffuseC, specularC;
			SceneLight * sl = m_Scene.GetLight(i);

			lightVector = (sl->position - info.hitPoint).Normalize();

			lightContribution = LightContribution(info, sl);

			float cosValue = clampValue(info.hitNormal.Dot(lightVector), 0.0f, 1.0f);

			// Diffuse reflectance
			if (BRDF->computeDiffuseRadiance(ray, info, lightVector, scattered, diffuseC))
			{
				diffuseC = diffuseC * Shade(scattered);
			}

			// Specular reflectance
			if (BRDF->computeSpecularRadiance(ray, info, lightVector, scattered, specularC))
			{
				specularC = specularC * Shade(scattered);
			}

			Lr = Lr + (lightContribution * (diffuseC + specularC) * cosValue);
		}
		
		// Specular reflection
		Vector reflexion;
		if (BRDF->scatterReflexion(ray, info, scattered, reflexion))
		{
			Lr = Lr + (reflexion * Shade(scattered));
		}

		// Refraction
		Vector refracted;
		if (BRDF->scatterTransmission(ray, info, scattered, refracted))
		{
			Lr = Lr + (refracted * Shade(scattered));
		}

		// Ambient lighting
		Lr = Lr + BRDF->computeAmbientRadiance(ray, info) * m_Scene.GetBackground().ambientLight;

		//fixGammut(Lr);
		if (Lr.Magnitude() > 3.0f)
		{
			Lr = Vector(1.0, 0.0, 0.0);
		}

		return Lr;
	}
	else
	{
		Vector topColor(m_Scene.GetBackground().color);
		Vector bottomColor(0.8, 0.8, 1);
		float alpha = 1.0f - ray.getDirection().y;
		return bottomColor;// *alpha + topColor * (1.0f - alpha);
	}
}

Vector RayTrace::LightContribution(HitInfo & info, SceneLight * light)
{
	Vector lightVector = light->position - info.hitPoint;
	float distToLight = lightVector.Magnitude();
	lightVector = lightVector.Normalize();

	const unsigned int sceneObjectCount = m_Scene.GetNumObjects();

	lightVector.Normalize();
	Ray lightVisibilityTest(info.hitPoint + lightVector * PRECISSION_EPSILON, lightVector);
	HitInfo visibilityInfo;
	bool visible = true;
	for (unsigned int i = 0; i < sceneObjectCount && visible; i++)
	{
		SceneObject * so = m_Scene.GetObjectW(i);
		
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

// =========================================================================
// =========================================================================

CameraWrapper::CameraWrapper()
{
}

void CameraWrapper::wrap(Camera & openglCam, unsigned int screenWidth, unsigned int screenHeigth)
{
	Vector L = (openglCam.GetPosition() - openglCam.GetTarget()).Normalize();
	Vector s = openglCam.GetUp().Cross(L).Normalize();
	Vector u = L.Cross(s);

	COP = openglCam.GetPosition();

	float FOV = openglCam.GetFOV();

	float aspectRatio = float(screenWidth) / float(screenHeigth);

	float halfHeight = tan((FOV * M_PI / 180.0f) / 2.0f);
	float halfWidth = aspectRatio * halfHeight;

	LowerLeftCorner = COP - s * halfWidth - u * halfHeight - L;
	horizontal = s * halfWidth * 2.0f;
	vertical = u * halfHeight * 2.0f;
}

Ray CameraWrapper::getRayForPixel(float t, float s)
{
	return Ray(COP, (LowerLeftCorner + horizontal * t + vertical * s - COP).Normalize());
}

// ==============================================================================

RaytracePixelTask::RaytracePixelTask(RayTrace & tracer, unsigned int x, unsigned int y)
	:tracer(tracer),x(x),y(y)
{
}

void RaytracePixelTask::run()
{

}