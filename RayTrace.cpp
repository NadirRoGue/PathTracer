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

#include <chrono>


void RayTrace::releaseBuffer()
{
	if (buffer != NULL)
	{
		for (int i = 0; i < Scene::WINDOW_HEIGHT; i++)
		{
			delete[] buffer[i];
		}

		delete[] buffer;
	}
}

void RayTrace::Render()
{
#ifdef _RT_MEASURE_PERFORMANCE
	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
#endif

	completedPixels = 0;
	screenSize = unsigned int(Scene::WINDOW_HEIGHT * Scene::WINDOW_WIDTH);
	wrapper.wrap(m_Scene.GetCamera(), Scene::WINDOW_WIDTH, Scene::WINDOW_HEIGHT);


	if (buffer == NULL)
	{
		buffer = new Vector*[Scene::WINDOW_HEIGHT];
		for (int i = 0; i < Scene::WINDOW_HEIGHT; i++)
		{
			buffer[i] = new Vector[Scene::WINDOW_WIDTH];
		}
	}

#ifdef _RT_PROCESS_PER_PIXEL
	std::unique_lock<std::mutex> lock(mut);
	for (int i = 0; i < Scene::WINDOW_HEIGHT; i++)
	{
		for (int j = 0; j < Scene::WINDOW_WIDTH; j++)
		{
			pool.addTask(std::make_unique<RaytracePixelTask>(this, i, j));
		}
	}
#else
	std::unique_lock<std::mutex> lock(mut);

	unsigned int xBatchSize = unsigned int(ceil(Scene::WINDOW_WIDTH / _RT_PROCESS_PIXEL_BATCH_SIZE_X));
	unsigned int yBatchSize = unsigned int(ceil(Scene::WINDOW_HEIGHT / _RT_PROCESS_PIXEL_BATCH_SIZE_Y));
#ifdef _RT_DEBUG
	unsigned int numBatches = 0;
#endif
	for (unsigned int i = 0; i < yBatchSize; i++)
	{
		for (unsigned int j = 0; j < xBatchSize; j++)
		{
			unsigned int xBatchStart = j * _RT_PROCESS_PIXEL_BATCH_SIZE_X;
			xBatchStart = min(xBatchStart, Scene::WINDOW_WIDTH - 1);

			unsigned int xBatchLen = xBatchStart + _RT_PROCESS_PIXEL_BATCH_SIZE_X > Scene::WINDOW_WIDTH? 
				Scene::WINDOW_WIDTH - xBatchStart : _RT_PROCESS_PIXEL_BATCH_SIZE_X;
			
			unsigned int yBatchStart = i * _RT_PROCESS_PIXEL_BATCH_SIZE_Y;
			yBatchStart = min(yBatchStart, Scene::WINDOW_HEIGHT - 1);
			
			unsigned int yBatchLen = yBatchStart + _RT_PROCESS_PIXEL_BATCH_SIZE_Y > Scene::WINDOW_HEIGHT? 
				Scene::WINDOW_HEIGHT - yBatchStart : _RT_PROCESS_PIXEL_BATCH_SIZE_Y;
			
			pool.addTask(std::make_unique<RaytraceBatchTask>(this, xBatchStart, xBatchLen, yBatchStart, yBatchLen));

#if _RT_DEBUG
			numBatches++;
#endif
		}
	}

#if _RT_DEBUG
	std::cout << "Num batches: " << numBatches << std::endl;
#endif
	
#endif

	monitor.wait(lock);

#ifdef _RT_MEASURE_PERFORMANCE
	std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

	std::cout << "Elapsed milliseconds: " << duration << std::endl;
#endif
}

#ifndef _RT_PROCESS_PER_PIXEL
void RayTrace::notifyThreadBatchEnd(unsigned int completedPix)
{
	std::unique_lock<std::mutex> lock(mut);
	completedPixels += completedPix;
	if (completedPixels == (Scene::WINDOW_HEIGHT * Scene::WINDOW_WIDTH))
	{
		monitor.notify_one();
	}
	lock.unlock();
}
#endif

void RayTrace::addPixel(unsigned int x, unsigned int y, Vector color)
{
	buffer[x][y] = color;
#ifdef _RT_PROCESS_PER_PIXEL
	std::unique_lock<std::mutex> lock(mut);
	completedPixels++;
	if (completedPixels == (Scene::WINDOW_HEIGHT * Scene::WINDOW_WIDTH))
	{
		monitor.notify_one();
	}
	lock.unlock();
#endif
}

Vector ** RayTrace::getBuffer()
{
	return buffer;
}

Vector RayTrace::calculatePixel(int screenX, int screenY)
{
	if (m_Scene.supersample)
	{
		return doSuperSamplingRayTrace(screenX, screenY);
	}
	else if(m_Scene.montecarlo)
	{
		return doMonteCarloRayTrace(screenX, screenY);
	}
	else
	{
		return doRayTrace(screenX, screenY);
	}
}

// ===================================================================================

Vector RayTrace::doRayTrace(int screenX, int screenY)
{
	Scene &la_escena = m_Scene;
	Camera &la_camara = la_escena.GetCamera();

	float t = float(screenX) / float(Scene::WINDOW_WIDTH);
	float s = float(screenY) / float(Scene::WINDOW_HEIGHT);

	Ray ray = wrapper.getRayForPixel(t, s);

	return shade(ray);
}

// ===================================================================================

Vector RayTrace::doSuperSamplingRayTrace(int screenX, int screenY)
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

		color = color + shade(ray);
	}

	return (color / float(samples));
}

// ===================================================================================


Vector RayTrace::doMonteCarloRayTrace(int screenX, int screenY)
{
	return Vector(1, 0, 1);
}

// ===================================================================================

HitInfo RayTrace::intersect(const Ray & ray)
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

Vector RayTrace::shade(const Ray & ray)
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
			return Vector();
		}

		// Direct lighting
		for (unsigned int i = 0; i < m_Scene.GetNumLights(); i++)
		{
			Vector diffuseC, specularC;
			SceneLight * sl = m_Scene.GetLight(i);

			lightVector = (sl->position - info.hitPoint).Normalize();

			I = lightContribution(info, sl);

			float cosValue = clampValue(info.hitNormal.Dot(lightVector), 0.0f, 1.0f);

			// Diffuse reflectance
			if (BRDF->computeDiffuseRadiance(ray, info, lightVector, scattered, diffuseC))
			{
				diffuseC = diffuseC * shade(scattered);
			}

			// Specular reflectance
			if (BRDF->computeSpecularRadiance(ray, info, lightVector, scattered, specularC))
			{
				specularC = specularC * shade(scattered);
			}

			Lr = Lr + (I * (diffuseC + specularC) * cosValue);
		}
		
		// Specular reflection
		Vector reflexion;
		if (BRDF->scatterReflexion(ray, info, scattered, reflexion))
		{
			Lr = Lr + (reflexion * shade(scattered));
		}

		// Refraction
		Vector refracted;
		if (BRDF->scatterTransmission(ray, info, scattered, refracted))
		{
			Lr = Lr + (refracted * shade(scattered));
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

Vector RayTrace::lightContribution(HitInfo & info, SceneLight * light)
{
	Vector lightVector = light->position - info.hitPoint;
	float distToLight = lightVector.Magnitude();
	lightVector = lightVector.Normalize();

	const unsigned int sceneObjectCount = m_Scene.GetNumObjects();

	lightVector.Normalize();
	Ray lightVisibilityTest(info.hitPoint + lightVector * _RT_BIAS, lightVector);
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

#ifdef _RT_PROCESS_PER_PIXEL
RaytracePixelTask::RaytracePixelTask(RayTrace * tracer, unsigned int x, unsigned int y)
	:tracer(tracer),x(x),y(y)
{
}

void RaytracePixelTask::run()
{
	tracer->addPixel(x, y, tracer->calculatePixel(y, x));
}

#else

void RaytraceBatchTask::run()
{
	unsigned int xEnd = xStart + xLen;
	unsigned int yEnd = yStart + yLen;

	for (unsigned int i = xStart; i < xEnd; i++)
	{
		for (unsigned int j = yStart; j < yEnd; j++)
		{
			tracer->addPixel(i, j, tracer->calculatePixel(j, i));
		}
	}

	tracer->notifyThreadBatchEnd(xLen * yLen);
}
#endif