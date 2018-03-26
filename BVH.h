#pragma once

#include <vector>
#include <iostream>

#include "Utils.h"
#include "Ray.h"

struct BoxPlane {
	Vector corner;
	float hx, hy, hz;
	float lx, ly, lz;
	Vector normal;
} typedef BoxPlane;

class BoundingVolume
{
protected:
	BoundingVolume * parent;
public:
	BoundingVolume()
	{
		parent = NULL;
	}

	~BoundingVolume()
	{
		if (parent != NULL)
		{
			delete parent;
		}
	}

	virtual void setCorners(Vector highestV, Vector lowestV) { }
	virtual bool testIntersect(const Ray & ray) = 0;
};

class BoundingSphere : public BoundingVolume
{
private:
	Vector center;
	float radius;
public:
	BoundingSphere() : BoundingVolume() {

	}

	void setCorners(Vector highestV, Vector lowestV)
	{
		center = (highestV + lowestV) / 2.0f;
		radius = (highestV - lowestV).Magnitude() / 2.0f;
	}

	bool testIntersect(const Ray & ray)
	{
		if (parent != NULL && !parent->testIntersect(ray))
		{
			return false;
		}

		Vector o = ray.getOrigin();
		Vector l = ray.getDirection();

		Vector OC = o - center;

		float div = l.Dot(l);
		float B = OC.Dot(l);
		float squaredB = B * B;
		float ac4 = div * OC.Dot(OC);
		float squareRootResult = (squaredB - ac4 + (radius * radius));

		float distance = -1.0f;

		if (squareRootResult == 0.0f && (-B) > 0.0f)
		{
			return true;
		}
		else if (squareRootResult > 0.0f)
		{
			float squareRoot = sqrt(squareRootResult);

			float case1 = ((-B - squareRoot) / div);
			float case2 = ((-B + squareRoot) / div);

			if (case1 > 0.0f || case2 > 0.0f)
				return true;
		}

		return false;
	}

};

class BoundingBox : public BoundingVolume
{
private:
	std::vector<BoxPlane> planes;
public:
	BoundingBox() : BoundingVolume() {

	}

	void setCorners(Vector highestV, Vector lowestV)
	{
		planes.reserve(6);
		buildPlanes(highestV, lowestV);
		parent = new BoundingSphere();
		parent->setCorners(highestV, lowestV);
	}

	bool testIntersect(const Ray & ray)
	{
		if (!parent->testIntersect(ray))
		{
			return false;
		}

		Vector o = ray.getOrigin();
		Vector d = ray.getDirection();
		for (BoxPlane & plane : planes)
		{
			Vector n = plane.normal;
			float dotNR;

			if ((dotNR = n.Dot(d)) != 0.0f)
			{
				float nc = n.Dot(o);
				float np = n.Dot(plane.corner);
				float planeIntersectResult = -(nc - np) / dotNR;
				if (planeIntersectResult > 0.0f)
				{
					Vector hittedPoint = o + (d * planeIntersectResult);
					float x = hittedPoint.x + _RT_BIAS;
					float y = hittedPoint.y + _RT_BIAS;
					float z = hittedPoint.z + _RT_BIAS;

					float minx = hittedPoint.x - _RT_BIAS;
					float miny = hittedPoint.y - _RT_BIAS;
					float minz = hittedPoint.z - _RT_BIAS;
					
					if (x >= plane.lx && minx <= plane.hx
						&& y >= plane.ly && miny <= plane.hy
						&& z >= plane.lz && minz <= plane.hz)
					{
						return true;
					}
				}
			}
		}

		return false;
	}

private:
	void buildPlanes(Vector &highestV, Vector &lowestV);
	void findHighestLowestValues(Vector &a, Vector &b, Vector &c, Vector &d, BoxPlane & plane);
};