#include "BVH.h"

void BoundingBox::buildPlanes(Vector &highestV, Vector &lowestV)
{
	Vector xLen(lowestV.x - highestV.x, 0.0f, 0.0f);
	Vector yLen(0.0f, lowestV.y - highestV.y, 0.0f);
	Vector zLen(0.0f, 0.0f, lowestV.z - highestV.z);

	Vector center = (lowestV + highestV) / 2.0f;
	Vector test;

	// front side
	BoxPlane front;
	Vector fronttopRight = highestV;
	Vector fronttopLeft = highestV + xLen;
	Vector frontbottomLeft = fronttopLeft + yLen;
	Vector frontbottomRight = highestV + yLen;
	front.corner = fronttopRight;
	test = (front.corner - center).Normalize();
	front.normal = (fronttopLeft - fronttopRight).Cross(frontbottomRight - fronttopRight).Normalize();

	if (test.Dot(front.normal) < 0.0f)
	{
		front.normal = front.normal * -1.0f;
	}
	findHighestLowestValues(fronttopLeft, fronttopRight, frontbottomLeft, frontbottomRight, front);

	// back side
	BoxPlane back;
	Vector backtopRight = highestV + zLen;
	Vector backtopLeft = backtopRight + xLen;
	Vector backbottomLeft = backtopLeft + yLen;
	Vector backbottomRight = backtopRight + yLen;
	back.corner = backtopRight;
	back.normal = (backtopLeft - backtopRight).Cross(backbottomRight - backtopRight).Normalize();
	test = (back.corner - center).Normalize();
	if (test.Dot(back.normal) < 0.0f)
	{
		back.normal = back.normal * -1.0f;
	}
	findHighestLowestValues(backtopRight, backtopLeft, backbottomLeft, backbottomRight, back);

	// left side
	BoxPlane left;
	Vector lefttopRight = highestV + xLen;
	Vector lefttopLeft = lefttopRight + zLen;
	Vector leftbottomRight = lefttopRight + yLen;
	Vector leftbottomLeft = lefttopLeft + yLen;
	left.corner = lefttopRight;
	left.normal = (lefttopLeft - lefttopRight).Cross(leftbottomRight - lefttopRight).Normalize();
	test = (left.corner - center).Normalize();
	if (test.Dot(left.normal) < 0.0f)
	{
		left.normal = left.normal * -1.0f;
	}
	findHighestLowestValues(lefttopRight, lefttopLeft, leftbottomLeft, leftbottomRight, left);

	// right side
	BoxPlane right;
	Vector righttopRight = highestV + zLen;
	Vector righttopLeft = highestV;
	Vector rightbottomRight = righttopRight + yLen;
	Vector rightbottomLeft = righttopLeft + yLen;
	right.corner = righttopRight;
	right.normal = (righttopLeft - righttopRight).Cross(rightbottomRight - righttopRight).Normalize();
	test = (right.corner - center).Normalize();
	if (test.Dot(right.normal) < 0.0f)
	{
		right.normal = right.normal * -1.0f;
	}
	findHighestLowestValues(righttopLeft, righttopRight, rightbottomLeft, rightbottomRight, right);

	// top side
	BoxPlane top;
	Vector toptopRight = highestV + xLen;
	Vector toptopLeft = toptopRight + zLen;
	Vector topbottomRight = highestV;
	Vector topbottomLeft = highestV + zLen;
	top.corner = toptopRight;
	top.normal = (toptopLeft - toptopRight).Cross(topbottomRight - toptopRight).Normalize();
	test = (top.corner - center).Normalize();
	if (test.Dot(top.normal) < 0.0f)
	{
		top.normal = top.normal * -1.0f;
	}
	findHighestLowestValues(toptopRight, toptopLeft, topbottomLeft, topbottomRight, top);

	// bottom side
	BoxPlane bottom;
	Vector bottomtopRight = toptopRight + yLen;
	Vector bottomtopLeft = toptopLeft + yLen;
	Vector bottombottomLeft = topbottomLeft + yLen;
	Vector bottombottomRight = topbottomRight + yLen;
	bottom.corner = bottomtopRight;
	bottom.normal = (bottomtopLeft - bottomtopRight).Cross(bottombottomRight - bottomtopRight).Normalize();
	test = (bottom.corner - center).Normalize();
	if (test.Dot(bottom.normal) < 0.0f)
	{
		bottom.normal = bottom.normal * -1.0f;
	}
	findHighestLowestValues(bottomtopRight, bottomtopLeft, bottombottomLeft, bottombottomRight, bottom);

	planes.push_back(front);
	planes.push_back(back);
	planes.push_back(left);
	planes.push_back(right);
	planes.push_back(top);
	planes.push_back(bottom);
}

void BoundingBox::findHighestLowestValues(Vector &a, Vector &b, Vector &c, Vector &d, BoxPlane & plane)
{
	plane.hx = a.x;
	plane.hy = a.y;
	plane.hz = a.z;
	plane.lx = a.x;
	plane.ly = a.y;
	plane.lz = a.z;

	std::vector<Vector> rest({ b, c, d });

	for (Vector & v : rest)
	{
		if (v.x > plane.hx)
			plane.hx = v.x;
		else if (v.x < plane.lx)
			plane.lx = v.x;

		if (v.y > plane.hy)
			plane.hy = v.y;
		else if (v.y < plane.ly)
			plane.ly = v.y;

		if (v.z > plane.hz)
			plane.hz = v.z;
		else if (v.z < plane.lz)
			plane.lz = v.z;
	}
}