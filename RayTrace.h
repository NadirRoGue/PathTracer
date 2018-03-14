/*
  15-462 Computer Graphics I
  Assignment 3: Ray Tracer
  C++ RayTracer Class
  Author: rtark
  Oct 2007

  NOTE: This is the file you will need to begin working with
		You will need to implement the RayTrace::CalculatePixel () function

  This file defines the following:
	RayTrace Class
*/
#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "Utils.h"

#include "pic.h"
#include "Ray.h"

#include "Threadpool.h"

#include "Config.h"

class CameraWrapper
{
private:
	Vector COP;
	Vector LowerLeftCorner;
	Vector horizontal;
	Vector vertical;

public:
	CameraWrapper();

	void wrap(Camera & openglCam, unsigned int screenWidth, unsigned int screenHeight);

	Ray getRayForPixel(float t, float s);
};


/*
	RayTrace Class - The class containing the function you will need to implement

	This is the class with the function you need to implement
*/
class RayTrace
{
private:
	CameraWrapper wrapper;

	Vector ** buffer;
	unsigned int completedPixels;

	ThreadPool pool;

	unsigned int screenSize;

	std::mutex mut;
	std::condition_variable monitor;

public:
	/* - Scene Variable for the Scene Definition - */
	Scene m_Scene;

	// -- Constructors & Destructors --
	RayTrace(void):buffer(NULL),completedPixels(0) {  }
	~RayTrace(void) { releaseBuffer(); }

	void Render();
	Vector ** getBuffer();

#ifndef _RT_PROCESS_PER_PIXEL
	void notifyThreadBatchEnd(unsigned int pix);
#endif
	void addPixel(unsigned int x, unsigned int y, Vector color);

	Vector calculatePixel (int screenX, int screenY);

    Vector doRayTrace(int screenX, int screenY);
    Vector doSuperSamplingRayTrace(int screenX, int screenY);
    Vector doMonteCarloRayTrace(int screenX, int screenY);

    HitInfo intersect(const Ray & ray);

    Vector shade(const Ray & ray);

    Vector lightContribution(HitInfo & info, SceneLight * light);
private:
	void releaseBuffer();
};

#ifdef _RT_PROCESS_PER_PIXEL
class RaytracePixelTask : public Runnable
{
private:
	RayTrace * tracer;
	unsigned int x;
	unsigned int y;
public:
	RaytracePixelTask(RayTrace * tracer, unsigned int x, unsigned int y);
	void run();
};

#else
class RaytraceBatchTask : public Runnable
{
private:
	RayTrace * tracer;
	unsigned int xStart;
	unsigned int yStart;
	unsigned int xLen;
	unsigned int yLen;

public:
	RaytraceBatchTask(RayTrace * tracer, unsigned int xStart, unsigned int xLen, unsigned int yStart, unsigned int yLen)
		:tracer(tracer),xStart(xStart), xLen(xLen), yStart(yStart), yLen(yLen) {}
	void run();
};
#endif