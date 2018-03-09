#pragma once

#include "Utils.h"

class BRDF
{
public:
	virtual Vector evaluate(Vector rayVector, Vector lightVector) = 0;
};

class Lambertian : public BRDF
{
public:
	Vector evaluate(Vector rayVector, Vector lightVector);
};