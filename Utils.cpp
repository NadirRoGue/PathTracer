#include "Utils.h"

// FROM https://www.scratchapixel.com/
void ComputeOrthoNormalBasis(Vector zVector, Vector &yVector, Vector &xVector)
{
	if (fabs(zVector.x) > fabs(zVector.y))
		yVector = Vector(zVector.z, 0.0f, -zVector.x).Normalize();
	else
		yVector = Vector(0.0f, -zVector.z, zVector.y).Normalize();
	xVector = zVector.Cross(yVector);
}

Vector WorldUniformHemiSample(Vector uniHemiSample, Vector & zVector, Vector &yVector, Vector &xVector)
{
	Vector result (
		uniHemiSample.x * xVector.x + uniHemiSample.y * zVector.x + uniHemiSample.z * yVector.x,
		uniHemiSample.x * xVector.y + uniHemiSample.y * zVector.y + uniHemiSample.z * yVector.y,
		uniHemiSample.x * xVector.z + uniHemiSample.y * zVector.z + uniHemiSample.z * yVector.z);

	return result;
}
