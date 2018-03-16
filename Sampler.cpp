#include "Sampler.h"
#include "Config.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <vector>
#include <algorithm>

/*
* Based on the code from "Raytracing from Ground Up", from Kevin Suffern
*/

Sampler::Sampler(unsigned int numSamples, unsigned int numItems) :numSamples(numSamples), numItems(numItems)
{
	// Reserve space for both type of samples
	planeSamples = new Vector[numSamples * numItems];
	// Shuffle sample access indices to get more uniformly distributed samples
	// between adjacents
	shuffleIndices();
	// Generate the samples (in unit square, directly valid for plane samples)
	//initSamples();
}

Vector Sampler::samplePlane()
{
	// Start sampling a new item, jump to a different item sample buffer
	if (usedSamples % numSamples == 0)
	{
		srand(time(NULL));
		jump = (rand() % numItems) * numSamples;
	}

	return (planeSamples[jump + suffledIndices[jump + usedSamples++ % numSamples]]);
}

Vector Sampler::sampleHemiSphere()
{
	// Start sampling a new item
	if (usedSamples % numSamples == 0)
	{
		srand(time(NULL));
		jump = (rand() % numItems) * numSamples;
	}

	return (hemiSamples[jump + suffledIndices[jump + numItems++ % numSamples]]);
}

void Sampler::shuffleIndices()
{
	suffledIndices = new unsigned int[numSamples * numItems];
	std::vector<int> indices;
	indices.reserve(numSamples);

	// Gather all our local indices (thats it, the indices of the samples of a single item)
	for (unsigned int j = 0; j < numSamples; j++)
		indices.push_back(j);

	// For each item
	for (unsigned int p = 0; p < numItems; p++) {
		// Randomly shuffle the items
		random_shuffle(indices.begin(), indices.end());

		// Add them to the current item indice buffer
		for (unsigned int j = 0; j < numSamples; j++)
			suffledIndices[p * numSamples + j] = indices[j];
	}
}

void Sampler::mapToHemiSphere(float cosineExp)
{
	// Cosine weighted hemisphere distribution
	unsigned int size = numSamples * numItems;
	hemiSamples = new Vector[size];

	for (unsigned int j = 0; j < size; j++) {
		float cosPhi = float(cos(2.0 * M_PI * planeSamples[j].x));
		float sinPhi = float(sin(2.0 * M_PI * planeSamples[j].x));
		float cosTheta = float(pow((1.0 - planeSamples[j].y), 1.0 / (cosineExp + 1.0f)));
		float sinTheta = float(sqrt(1.0 - cosTheta * cosTheta));
		float pu = sinTheta * cosPhi;
		float pv = sinTheta * sinPhi;
		float pw = cosTheta;
		hemiSamples[j] = Vector(pu, pv, pw);
	}
}

// ==========================================================================

void DummySampler::initSamples()
{
	// Distribute grid-like regular samples
	unsigned int n = (unsigned int)sqrt((float)numSamples);

	for (unsigned int j = 0; j < numItems; j++)
	{
		for (unsigned int p = 0; p < n; p++)
		{
			for (unsigned int q = 0; q < n; q++)
			{
				planeSamples[p * n + q] = Vector(float((q + 0.5) / n), float((p + 0.5) / n), 0.0f);
			}
		}
	}
}

// ==========================================================================

void MultiJitteredSampler::initSamples()
{
	// Top level grid for Jittered
	unsigned int n = (unsigned int)sqrt((float)numSamples);
	// Bottom level grid for n-rocks
	float nRockCellW = 1.0f / ((float)numSamples);

	srand(time(NULL));

	float randomPosInCell;
	// distribute points in the initial patterns
	for (unsigned int p = 0; p < numItems; p++)
	{
		for (unsigned int i = 0; i < n; i++)
		{
			for (unsigned int j = 0; j < n; j++)
			{
				randomPosInCell = float((rand() % _RT_RAND_PRECISSION) / float(_RT_RAND_PRECISSION)) * nRockCellW;
				planeSamples[i * n + j + p * numSamples].x = (i * n + j) * nRockCellW + randomPosInCell;

				randomPosInCell = float((rand() % _RT_RAND_PRECISSION) / float(_RT_RAND_PRECISSION)) * nRockCellW;
				planeSamples[i * n + j + p * numSamples].y = (j * n + i) * nRockCellW + randomPosInCell;
			}
		}
	}

	// Shuffle coordinates within the grid to ensure that patterns are not accessed sistematically
	// creating tiling like renders
	// Must ensure n-Rocks conditions are still meet (same column or row shouldnt have values within the same sub grid)

	// shuffle x coordinates
	for (unsigned int p = 0; p < numItems; p++)
	{
		for (unsigned int i = 0; i < n; i++)
		{
			for (unsigned int j = 0; j < n; j++) 
			{
				int indexDiff = (n - 1) - j;
				int k = j + (rand() % indexDiff);// rand_int(j, n - 1);
				float t = planeSamples[i * n + j + p * numSamples].x;
				planeSamples[i * n + j + p * numSamples].x = planeSamples[i * n + k + p * numSamples].x;
				planeSamples[i * n + k + p * numSamples].x = t;
			}
		}
	}

	// shuffle y coordinates
	for (unsigned int p = 0; p < numItems; p++)
	{
		for (unsigned int i = 0; i < n; i++)
		{
			for (unsigned int j = 0; j < n; j++)
			{
				int indexDiff = (n - 1) - j;
				int k = j + (rand() % indexDiff);// rand_int(j, n - 1);
				float t = planeSamples[j * n + i + p * numSamples].y;
				planeSamples[j * n + i + p * numSamples].y = planeSamples[k * n + i + p * numSamples].y;
				planeSamples[k * n + i + p * numSamples].y = t;
			}
		}
	}
}