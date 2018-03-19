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
	HitInfo intersect(const Ray & ray);
	Vector lightContribution(HitInfo & info, Vector & lightVector, SceneLight * light);
};

// =================================================================================

class RayTracer : public Tracer
{
public:
	RayTracer(Scene * scene) :Tracer(scene) {}
	virtual Vector doTrace(int screenX, int screenY);
protected:
	virtual Vector shade(const Ray & ray);
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
private:
	Sampler * pixelSampler;
	float pdfArea;

	std::default_random_engine engine;
	std::uniform_real_distribution<float> d;
public:
	MonteCarloRayTracer(Scene * scene) :RayTracer(scene) 
	{ 
		pixelSampler = new MultiJitteredSampler(_RT_MC_PIXEL_SAMPLES, 83);
		pdfArea = 1.0f / (Scene::WINDOW_HEIGHT * Scene::WINDOW_WIDTH);

		d = std::uniform_real_distribution<float>(0.0f, 1.0f);
	}

	Vector doTrace(int screenX, int screenY);
	Vector shade(const Ray & ray);

private:
	void samplePixel(int x, int y, float &st, float &ss, float &pdf);
};