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

class BoundingBox
{
private:
	Vector highestVertex;
	Vector lowestVertex;

	std::vector<BoxPlane> planes;
public:
	BoundingBox() 
	{
	}

	void setCorners(Vector highestV, Vector lowestV)
	{
		this->highestVertex = highestV;
		this->lowestVertex = lowestV;
		planes.reserve(6);
		buildPlanes(highestVertex, lowestVertex);
	}

	bool testIntersect(const Ray & ray)
	{
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