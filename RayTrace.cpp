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

#ifdef _RT_MEASURE_PERFORMANCE
#include <chrono>
#endif

#include "PhysicalMaterial.h"
#include "Scene.h"
#include "RayTrace.h"

// =====================================================================

void RayTrace::initializeBuffer()
{
	if (buffer == NULL)
	{
		buffer = new Vector*[Scene::WINDOW_HEIGHT];
		for (int i = 0; i < Scene::WINDOW_HEIGHT; i++)
		{
			buffer[i] = new Vector[Scene::WINDOW_WIDTH];
		}
	}
}

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

void RayTrace::initializeTracer()
{
	if (tracer != NULL)
	{
		delete tracer;
	}

	if (Scene::montecarlo)
	{
		tracer = new MonteCarloRayTracer(&m_Scene);
	}
	else if (Scene::supersample)
	{
		tracer = new SuperSamplingRayTracer(&m_Scene);
	}
	else if (Scene::boundingbox)
	{
		tracer = new BBTracer(&m_Scene);
	}
	else
	{
		tracer = new RayTracer(&m_Scene);
	}
}

void RayTrace::Render()
{
#ifdef _RT_MEASURE_PERFORMANCE
	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
#endif

	initializeTracer();
	tracer->init();

	completedPixels = 0;
	screenSize = unsigned int(Scene::WINDOW_HEIGHT * Scene::WINDOW_WIDTH);
	initializeBuffer();
	
	std::unique_lock<std::mutex> lock(mut);

#ifdef _RT_PROCESS_PER_PIXEL
	for (int i = 0; i < Scene::WINDOW_HEIGHT; i++)
	{
		for (int j = 0; j < Scene::WINDOW_WIDTH; j++)
		{
			pool.addTask(std::make_unique<RaytracePixelTask>(this, i, j));
		}
	}
#else
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

	std::cout << "Elapsed time: " << duration << " ms" << std::endl;
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
	return tracer->doTrace(screenX, screenY);
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

// =========================================================================
// =========================================================================

#ifdef _RT_PROCESS_PER_PIXEL
void RaytracePixelTask::run()
{
	tracer->addPixel(x, y, tracer->calculatePixel(y, x));
}

// =========================================================================
// =========================================================================

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