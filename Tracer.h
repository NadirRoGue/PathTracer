#pragma once

#include "Utils.h"
#include "Ray.h"
#include "Scene.h"
#include <random>

// =================================================================================
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

// =================================================================================

class Tracer
{
protected:
	Scene * scene;
	CameraWrapper wrapper;
public:
	Tracer(Scene * scene) :scene(scene) {}

	void init();

	virtual Vector doTrace(int screenX, int screenY) = 0;
protected:
	HitInfo intersect(Ray & ray);
	Vector lightContribution(HitInfo & info, Vector & lightVector, SceneLight * light);
	float getLightAttenuation(Ray & ray)
	{
		return (0.15f + 0.03f * ray.getDistance());
	}
};

// =================================================================================

class RayTracer : public Tracer
{
public:
	RayTracer(Scene * scene) :Tracer(scene) {}
	virtual Vector doTrace(int screenX, int screenY);
protected:
	virtual Vector shade(Ray & ray);
};

// =================================================================================

class BBTracer : public RayTracer
{
public:
	BBTracer(Scene * scene):RayTracer(scene){}
protected:
	Vector shade(Ray & ray);
};

// =================================================================================

class SuperSamplingRayTracer : public RayTracer
{
public:
	SuperSamplingRayTracer(Scene * scene) : RayTracer(scene) {}
	Vector doTrace(int screenX, int screenY);
};

// =================================================================================

class MonteCarloRayTracer : public RayTracer
{
protected:
	FloatSampler pixelSampler;
	FloatSampler russianRouletteSampler;
	float pdfArea;

public:
	MonteCarloRayTracer(Scene * scene) :RayTracer(scene) 
	{ 
		pdfArea = 1.0f / (Scene::WINDOW_HEIGHT * Scene::WINDOW_WIDTH);
	}

	virtual Vector doTrace(int screenX, int screenY);
	virtual Vector shade(Ray & ray);

protected:
	void samplePixel(int x, int y, float &st, float &ss, float &pdf);
};

// =================================================================================

class PathTracer : public MonteCarloRayTracer
{
public:
	PathTracer(Scene * scene) : MonteCarloRayTracer(scene){}
	Vector doTrace(int screenX, int screenY);
	Vector shade(Ray & ray);
};