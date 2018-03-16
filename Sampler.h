#pragma once

#include "Utils.h"

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
	Vector * hemiSamples;
	Vector * planeSamples;
	unsigned int * suffledIndices;

	unsigned int numSamples;
	unsigned int numItems;	// Raytracing from ground up: A very good numer is "83"
	unsigned int usedSamples;
	unsigned int jump;
public:
	Sampler(unsigned int numSamples, unsigned int numItems);
	~Sampler() { delete[] hemiSamples; delete[] planeSamples; }

	void mapToHemiSphere(float cosineExp);

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
	DummySampler(unsigned int numSamples, unsigned int numItems) :Sampler(numSamples, numItems) {}
	void initSamples();
};

class MultiJitteredSampler : public Sampler
{
public:
	MultiJitteredSampler(unsigned int numSamples, unsigned int numItems) :Sampler(numSamples, numItems) {}
	void initSamples();
};