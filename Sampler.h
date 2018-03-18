#pragma once

#include "Utils.h"
#include <random>
#include <vector>

/*
* Based on the code from "Raytracing from Ground Up", from Kevin Suffern
*
* We generate samples in advance and store them to access them when needed
* We also generate a configurable number of sample batches. The book author
* states that "83" its a good number of batches that works very well in all
* situations. This means that, for 4 samples Monte Carlo anti-aliasing, we
* would generate 83 * 4 = 332 samples
* We also shuffle the sample accesors to avoid pattern-like rendering when
* sampling
*/

class Sampler
{
protected:
	std::vector<Vector> hemiSamples;
	std::vector<Vector> sphereSamples;
	std::vector<Vector> planeSamples;
	std::vector<unsigned int> suffledIndices;

	std::default_random_engine generator;
	std::uniform_real_distribution<float> ditrib;

	unsigned int numSamples;
	unsigned int numItems;	// Raytracing from ground up: A very good numer is "83"
	unsigned int usedSamples;
	unsigned int jump;
public:
	Sampler(unsigned int numSamples, unsigned int numItems);
	~Sampler();

	unsigned int getNumSamples() { return numSamples; }

	void mapToHemiSphere(float cosineExp);
	void mapToUniformSphere();

	Vector sampleSphere();
	Vector sampleHemiSphere();
	Vector samplePlane();

protected:
	virtual void initSamples() = 0;
private:
	void shuffleIndices();
};

// Sampler used when the sample count is 1
class DummySampler : public Sampler
{
public:
	DummySampler(unsigned int numSamples, unsigned int numItems) :Sampler(numSamples, numItems) { initSamples(); }
	void initSamples();
};

class MultiJitteredSampler : public Sampler
{
public:
	MultiJitteredSampler(unsigned int numSamples, unsigned int numItems) :Sampler(numSamples, numItems) { initSamples(); }
	void initSamples();
};

//http://www.cs.princeton.edu/~funk/tog02.pdf
inline Vector mapSquareSampleToTrianglePoint(const Vector & sample, Vector & A, Vector & B, Vector & C)
{
	//P = (1 - sqrt(r1)) * A + (sqrt(r1) * (1 - r2)) * B + (sqrt(r1) * r2) * C
	float sqrtX = sqrtf(sample.x);
	return (A * (1.0f - sqrtX)) + (B * (sqrtX * (1.0f - sample.y))) + (C * (sqrtX * sample.y));
}