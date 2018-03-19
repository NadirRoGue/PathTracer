#pragma once

#include <random>
#define _USE_MATH_DEFINES
#include <math.h>

#include "Utils.h"

template<class T>
class Sampler
{
protected:
	std::default_random_engine generator;

	T intervalStart;
	T intervalEnd;
public:
	Sampler(T is, T ie)
	{
		intervalStart = is;
		intervalEnd = ie;
	}

	Vector sampleSphere()
	{
		T a, b;
		sample2D(a, b);

		float theta = 2.0f * float(M_PI) * a;
		float phi = acos(1 - 2 * b);
		float x = sin(phi) * cosf(theta);
		float y = sinf(phi) * sinf(theta);
		float z = cosf(phi);

		return Vector(x, y, z);
	}

	Vector sampleHemiSphere()
	{
		T a, b;
		sample2D(a, b);

		float sinTheta = sqrtf(1.0f - a * b);
		float phi = 2.0f * float(M_PI) * b;
		float x = sinTheta * cosf(phi);
		float z = sinTheta * sinf(phi);

		return Vector(x, a, z);
	}

	virtual Vector samplePlane()
	{
		T a, b;
		sample2D(a, b);
		return Vector(float(a), float(b), 0.0f);
	}

	T sampleRect()
	{
		T a;
		sample1D(a);
		return a;
	}
protected:
	virtual void sample1D(T &a) { };
	virtual void sample2D(T &a, T &b) { };
};

class IntegerSampler : public Sampler<int>
{
private:
	std::uniform_int_distribution<int> distribution;
public:
	IntegerSampler(int istart = 0, int iend = 1) :Sampler(istart, iend)
	{ 
		distribution = std::uniform_int_distribution<int>(istart, iend);
	}
protected:
	void sample1D(int &a);
	void sample2D(int &a, int &b);
};

class FloatSampler : public Sampler<float>
{
private:
	std::uniform_real_distribution<float> distribution;
public:
	FloatSampler(float intervalStart = 0.0f, float intervalEnd = 1.0f) :Sampler(intervalStart, intervalEnd) 
	{
		distribution = std::uniform_real_distribution<float>(intervalStart, intervalEnd);
	}
protected:
	void sample1D(float &a);
	void sample2D(float &a, float &b);
};

class MultiJitteredSampler : public Sampler<float>
{
private:
	std::uniform_real_distribution<float> distribution;
};

//http://www.cs.princeton.edu/~funk/tog02.pdf
inline Vector mapSquareSampleToTrianglePoint(const Vector & sample, Vector & A, Vector & B, Vector & C)
{
	//P = (1 - sqrt(r1)) * A + (sqrt(r1) * (1 - r2)) * B + (sqrt(r1) * r2) * C
	float sqrtX = sqrtf(sample.x);
	return (A * (1.0f - sqrtX)) + (B * (sqrtX * (1.0f - sample.y))) + (C * (sqrtX * sample.y));
}