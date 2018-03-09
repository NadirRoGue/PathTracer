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
public:
	/* - Scene Variable for the Scene Definition - */
	Scene m_Scene;

	// -- Constructors & Destructors --
	RayTrace(void) { }
	~RayTrace(void) { }

	void preRenderInit();

	// -- Main Functions --
	// - CalculatePixel - Returns the Computed Pixel for that screen coordinate
   Vector CalculatePixel (int screenX, int screenY);

   Vector DoRayTrace(int screenX, int screenY);
   Vector DoSuperSamplingRayTrace(int screenX, int screenY);
   Vector DoMonteCarloRayTrace(int screenX, int screenY);

   Vector Shade(HitInfo & info);

   Vector LightContribution(HitInfo & info, SceneLight * light);

   SceneMaterial AverageMaterial(SceneMaterial * materials[3], float contributions[3], unsigned int numMaterials);

   Vector MaterialContribution(SceneMaterial & material, Vector LightVector);
};
