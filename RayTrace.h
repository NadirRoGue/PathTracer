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
#include "BRDF.h"

#include "Threadpool.h"

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

	BRDF * lambertian;
	BRDF * metallic;
	BRDF * fresnel;
public:
	/* - Scene Variable for the Scene Definition - */
	Scene m_Scene;

	// -- Constructors & Destructors --
	RayTrace(void) { lambertian = new DiffuseLambertian(); metallic = new SpecularPhong(); /*fresnel = new DielectricFresnel();*/ }
	~RayTrace(void) { delete lambertian; delete metallic; /*delete fresnel;*/ }

	void preRenderInit();

	// -- Main Functions --
	// - CalculatePixel - Returns the Computed Pixel for that screen coordinate
   Vector CalculatePixel (int screenX, int screenY);

   Vector DoRayTrace(int screenX, int screenY);
   Vector DoSuperSamplingRayTrace(int screenX, int screenY);
   Vector DoMonteCarloRayTrace(int screenX, int screenY);

   HitInfo Intersect(const Ray & ray);

   Vector Shade(const Ray & ray);

   Vector LightContribution(HitInfo & info, SceneLight * light);
};

class RaytracePixelTask : public Runnable
{
private:
	RayTrace tracer;
	unsigned int x;
	unsigned int y;
public:
	RaytracePixelTask(RayTrace & tracer, unsigned int x, unsigned int y);
	void run();
};
