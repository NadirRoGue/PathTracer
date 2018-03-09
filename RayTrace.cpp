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

	srand(time(NULL));

	static float max = 1.0 - FLT_EPSILON;

	HitInfo closer;
	HitInfo info;
	Ray ray;

	float rand1, rand2, t, s;

	static unsigned int numObjects = la_escena.GetNumObjects();

	closer.hit = false;

	rand1 = float(rand() % 1000) / 1000.0f;
	rand2 = float(rand() % 1000) / 1000.0f;

	t = float(screenX) / float(Scene::WINDOW_WIDTH);
	s = float(screenY) / float(Scene::WINDOW_HEIGHT);

	ray = wrapper.getRayForPixel(t, s);

	for (unsigned int i = 0; i < numObjects; i++)
	{
		SceneObject * object = la_escena.GetObjectW(i);

		if (object->testIntersection(ray, info))
		{
			if (closer.hit)
			{
				float closerDist = (la_camara.GetPosition() - closer.hitPoint).Magnitude();
				float currentDist = (la_camara.GetPosition() - info.hitPoint).Magnitude();

				if (closerDist <= currentDist)
				{
					continue;
				}
			}
			closer = info;
		}
	}

	return Shade(closer);
}

Vector RayTrace::DoSuperSamplingRayTrace(int screenX, int screenY)
{
	Scene &la_escena = m_Scene;
	Camera &la_camara = la_escena.GetCamera();

	srand(time(NULL));

	Vector color;

	static float max = 1.0 - FLT_EPSILON;

	HitInfo closer;
	HitInfo info;
	Ray ray;

	float rand1, rand2, t, s;

	unsigned int samples = 100;

	static unsigned int numObjects = la_escena.GetNumObjects();

	for (unsigned int pass = 0; pass < samples; pass++)
	{
		closer.hit = false;

		rand1 = float(rand() % 1000) / 1000.0f;
		rand2 = float(rand() % 1000) / 1000.0f;

		t = (float(screenX) + rand1 * max) / float(Scene::WINDOW_WIDTH);
		s = (float(screenY) + rand2 * max) / float(Scene::WINDOW_HEIGHT);

		ray = wrapper.getRayForPixel(t, s);

		for (unsigned int i = 0; i < numObjects; i++)
		{
			SceneObject * object = la_escena.GetObjectW(i);

			if (object->testIntersection(ray, info))
			{
				if (closer.hit)
				{
					float closerDist = (la_camara.GetPosition() - closer.hitPoint).Magnitude();
					float currentDist = (la_camara.GetPosition() - info.hitPoint).Magnitude();

					if (closerDist <= currentDist)
					{
						continue;
					}
				}
				closer = info;
			}
		}

		color = color + Shade(closer);
	}

	return (color / float(samples));
}

Vector RayTrace::DoMonteCarloRayTrace(int screenX, int screenY)
{
	return Vector(1, 0, 1);
}

// ===================================================================================

Vector RayTrace::Shade(HitInfo & info)
{
	if (info.hit)
	{
		SceneMaterial averageMaterialAtPoint = AverageMaterial(info.hittedMaterials, info.contributions, info.numHittedMaterials);
		Vector materialContribution = MaterialContribution(averageMaterialAtPoint, Vector());
		Vector Lr;
		for (unsigned int i = 0; i < m_Scene.GetNumLights(); i++)
		{
			SceneLight * sl = m_Scene.GetLight(i);
			Vector lightVector = (sl->position - info.hitPoint).Normalize();

			Vector lightContribution = LightContribution(info, sl);
			//Vector materialContribution = MaterialContribution(info, lightVector);

			float cosValue = info.hitNormal.Dot(lightVector);
			cosValue = cosValue < 0.0f ? 0.0f : cosValue;
			Lr = Lr + (lightContribution * materialContribution * cosValue);
		}

		Lr = Lr + materialContribution * m_Scene.GetBackground().ambientLight;

		return Lr;
	}
	else
	{
		return m_Scene.GetBackground().color;
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
		
		if (so->testIntersection(lightVisibilityTest, visibilityInfo))
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
		//return m_Scene.GetBackground().ambientLight;
	}
}

SceneMaterial RayTrace::AverageMaterial(SceneMaterial * materials[3], float contributions[3],  unsigned int numMaterials)
{
	numMaterials = numMaterials > 3 ? 3 : numMaterials;

	SceneMaterial result;
	for (unsigned int i = 0; i < numMaterials; i++)
	{
		float contribution = contributions[i];
		result.diffuse = result.diffuse + (materials[i]->diffuse * contribution);
		result.reflective = result.reflective + (materials[i]->reflective * contribution);
		result.refraction_index = result.refraction_index + (materials[i]->refraction_index * contribution);
		result.shininess += (materials[i]->shininess * contribution);
		result.specular = result.specular + (materials[i]->specular * contribution);
		result.transparent = result.transparent + (materials[i]->transparent * contribution);
	}

	return result;
}

Vector RayTrace::MaterialContribution(SceneMaterial & material, Vector LightVector)
{
	Vector finalColor;
	Vector uv;

	/*for (unsigned int z = 0; z < info.numHittedMaterials; z++)
	{
		uv = uv + Vector(info.u[z], info.v[z], 0.0f) * info.contributions[z];
	}*/

	finalColor = material.diffuse;

	/*for (unsigned int z = 0; z < info.numHittedMaterials; z++)
	{
		SceneMaterial * mat = info.hittedMaterials[z];
		finalColor = finalColor + (mat->diffuse * info.contributions[z]);
	}*/
	
	/*
	for (unsigned int z = 0; z < info.numHittedMaterials; z++)
	{
		for (unsigned int m = 0; m < m_Scene.GetNumMaterials(); m++)
		{
			SceneMaterial * mat = m_Scene.GetMaterial(m);
			if (mat->name == info.hittedMaterials[z])
			{
				finalColor = finalColor * mat->GetTextureColor(uv.x, uv.y);
				break;
			}
		}
	}*/
	return finalColor;
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