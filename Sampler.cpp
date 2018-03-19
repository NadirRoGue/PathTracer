#include "Sampler.h"

void IntegerSampler::sample1D(int &a)
{
	a = distribution(generator);
}

void IntegerSampler::sample2D(int &a, int &b)
{
	a = distribution(generator);
	b = distribution(generator);
}

void FloatSampler::sample1D(float &a)
{
	a = distribution(generator);
}

void FloatSampler::sample2D(float &a, float &b)
{
	a = distribution(generator);
	b = distribution(generator);
}