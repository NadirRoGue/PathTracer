#include "SceneObject.h"

#include <random>
#include <time.h>

// ==========================================================

void SceneSphere::testIntersection(const Ray & ray, HitInfo & outHitInfo)
{
	outHitInfo.hit = false;

	// Centro del emisor de rayos y dirección de este
	Vector o = ray.getOrigin();
	Vector l = ray.getDirection();
	Vector tempCenter = center;

#ifdef _RT_TRANSFORM_RAY_TO_LOCAL_SPACE
	o.w = 1.0f;
	l.w = 0.0f;
	Vector tempDir = o + l;
	o = worldToLocalMatrix * o;
	tempDir = worldToLocalMatrix * tempDir;
	l = (tempDir - o);
#endif

	Vector OC = o - tempCenter;

	float div = l.Dot(l);
	float B = OC.Dot(l);
	float squaredB = B * B;
	float ac4 = div * OC.Dot(OC);
	float squareRootResult = (squaredB - ac4 + (radius * radius));

	float distance = -1.0f;

	// Si el resultado de B^2 - 4AC es 0, significa que hemos intersectado la esfera
	// en un sólo punto
	if (squareRootResult == 0.0f && (-B) > 0.0f)
	{
		distance = (-B / div);
	}
	else if (squareRootResult > 0.0f)
	{
		float squareRoot = sqrt(squareRootResult);

		float case1 = ((-B - squareRoot) / div);
		float case2 = ((-B + squareRoot) / div);

		if (case1 <= 0.0f && case2 > 0.0f)
		{
			distance = case2;
		}
		else if (case2 <= 0.0f && case1 > 0.0f)
		{
			distance = case1;
		}
		else
		{
			distance = case1 <= case2 ? case1 : case2;
		}
	}

	if (distance > 0.0f)
	{
		Vector hitPoint(o + (l * distance));
#ifdef _RT_TRANSFORM_RAY_TO_LOCAL_SPACE
		hitPoint = localToWorldMatrix * hitPoint;
		tempCenter = localToWorldMatrix *  tempCenter;
#endif
		outHitInfo.hitPoint = hitPoint;
		outHitInfo.hitNormal = ((hitPoint - tempCenter) / radius).Normalize();
		outHitInfo.hittedMaterial = *material;
		outHitInfo.physicalMaterial = physicalMaterial;
		outHitInfo.inRay = ray;
		outHitInfo.hit = true;
		outHitInfo.isLight = isLight;
		outHitInfo.emission = emission;
	}
}

void SceneSphere::applyAffineTransformations()
{
#ifdef _RT_TRANSFORM_RAY_TO_LOCAL_SPACE
	computeMatrices();
#else
	Matrix translate;
	translate.setTranslateMatrix(position);
	center = translate * center;

	position = Vector();
#endif
}

Vector SceneSphere::sampleShape(float &pdf)
{
	Vector sample = sampler.sampleSphere();
	pdf = 1.0f / (float(M_PI) * 4.0f);
	return (center + sample * radius); // In world space
}

// =================================================================================

void SceneTriangle::testIntersection(const Ray & ray, HitInfo & outHitInfo)
{
	outHitInfo.hit = false;

	// Punto desde donde se emite el rayo y su dirección
	Vector center = ray.getOrigin();
	Vector dir = ray.getDirection();

#ifdef _RT_TRANSFORM_RAY_TO_LOCAL_SPACE
	center.w = 1.0f;
	dir.w = 0.0f;
	Vector tempDir = center + dir;
	center = worldToLocalMatrix * center;
	tempDir = worldToLocalMatrix * tempDir;
	dir = (tempDir - center);
#endif

	// Tras despejar la distancia t de la ecuación de pertenencia de un punto
	// a un plano, comprobamos que el divisor es distinto de 0 (igual a 0 significa
	// que el rayo es paralelo al plano, y por lo tanto, nunca intersectarían)

	Vector triangleNormal = (this->vertex[1] - this->vertex[0]).Cross(this->vertex[2] - this->vertex[0]);

	float tSurf = triangleNormal.Magnitude();
	triangleNormal.Normalize();

	float div = dir.Dot(triangleNormal);
	if (div != 0.0f)
	{
		// Completamos el resto de la ecuación
		float nc = triangleNormal.Dot(center);
		float np = triangleNormal.Dot(vertex[0]);

		// Si la distancia obtenida es negativa, significa que el triángulo
		// está detrás del emisor de rayos
		float planeIntersectResult = -((nc - np) / div);
		if (planeIntersectResult > 0.0f)
		{
			// Una vez hemos encontrado un punto en el plano definido por el triángulo,
			// comprobamos que está dentro de este.
			Vector hittedPoint = center + (dir * planeIntersectResult);

			// Para comprobarlo, la suma de las áreas de los 3 sub-triángulos formados por el punto 
			// contenido dentro del triángulo debe ser igual a la del triánglo original

			float Ta = (vertex[1] - hittedPoint).Cross(vertex[2] - hittedPoint).Magnitude();
			float Tb = (vertex[0] - hittedPoint).Cross(vertex[2] - hittedPoint).Magnitude();
			float Tc = (vertex[0] - hittedPoint).Cross(vertex[1] - hittedPoint).Magnitude();

			float a = Ta / tSurf;
			float b = Tb / tSurf;
			float c = Tc / tSurf;

			float total = a + b + c;

			if (abs(total - 1.0f) < _RT_BIAS)
			{
				Vector averageNormal(normal[0] * a + normal[1] * b + normal[2] * c);
				averageNormal.Normalize();

#ifdef _RT_TRANSFORM_RAY_TO_LOCAL_SPACE
				hittedPoint = localToWorldMatrix * hittedPoint;
				averageNormal = localToWorldMatrix.Inverse().Transpose() * Vector(averageNormal.x, averageNormal.y, averageNormal.z, 0.0f);
#endif
				outHitInfo.hitPoint = hittedPoint;
				outHitInfo.hitNormal = averageNormal;
				outHitInfo.u = ((abs(u[0]) * a) + (abs(u[1]) * b) + (abs(u[2]) * c));
				outHitInfo.v = ((abs(v[0]) * a) + (abs(v[1]) * b) + (abs(v[2]) * c));
				outHitInfo.u -= floor(outHitInfo.u);
				outHitInfo.v -= floor(outHitInfo.v);
				outHitInfo.hittedMaterial = averageMaterials(a, b, c, outHitInfo.u, outHitInfo.v);
				outHitInfo.physicalMaterial = physicalMaterial;
				outHitInfo.inRay = ray;
				outHitInfo.hit = true;
				outHitInfo.isLight = isLight;
				outHitInfo.emission = emission;
			}
		}
	}
}

SceneMaterial SceneTriangle::averageMaterials(float u, float v, float w, float finalU, float finalV)
{
	SceneMaterial averaged;

	SceneMaterial * a = material[0];
	SceneMaterial * b = material[1];
	SceneMaterial * c = material[2];

	// Emissive
	// Will be the same in all vertices
	averaged.emissive = a->emissive * u + b->emissive * v + c->emissive * w;

	// Compute the rest of the parameters only if the object is not a light source
	if (averaged.emissive.Magnitude() == 0.0f)
	{

		// Diffuse average (with texture)
		averaged.diffuse = (a->diffuse * a->GetTextureColor(finalU, finalV)) * u
			+ (b->diffuse * b->GetTextureColor(finalU, finalV)) * v
			+ (c->diffuse * c->GetTextureColor(finalU, finalV)) * w;

		// Reflective average
		averaged.reflective = a->reflective * u
			+ b->reflective * v
			+ c->reflective * w;

		// IOR average
		averaged.refraction_index = a->refraction_index * u
			+ b->refraction_index * v
			+ c->refraction_index * w;

		// Shineness average
		averaged.shininess = a->shininess * u
			+ b->shininess * v
			+ c->shininess * w;

		// Specular average
		averaged.specular = a->specular * u
			+ b->specular * v
			+ c->specular * w;

		// Transparent average
		averaged.transparent = a->transparent * u
			+ b->transparent * v
			+ c->transparent * w;
	}

	return averaged;
}

void SceneTriangle::applyAffineTransformations()
{
#ifdef _RT_TRANSFORM_RAY_TO_LOCAL_SPACE
	computeMatrices();
#else
	Matrix translateM;
	translateM.setTranslateMatrix(position);
	Quaternion q(rotation);
	Matrix rotationM = q.getRotationMatrix();
	Matrix scaleM;
	scaleM.setScaleMatrix(scale);

	Matrix localToWorld = rotationM * translateM * scaleM;

	vertex[0] = localToWorld * vertex[0];
	vertex[1] = localToWorld * vertex[1];
	vertex[2] = localToWorld * vertex[2];

	normal[0] = localToWorld * Vector(normal[0].x, normal[0].y, normal[0].z, 0.0f);
	normal[1] = localToWorld * Vector(normal[1].x, normal[1].y, normal[1].z, 0.0f);
	normal[2] = localToWorld * Vector(normal[2].x, normal[2].y, normal[2].z, 0.0f);

	position = rotation = Vector();
	scale = Vector(1.0f, 1.0f, 1.0f);
#endif
}

void SceneTriangle::computeArea()
{
	Vector BA = vertex[1] - vertex[0];
	Vector CA = vertex[2] - vertex[0];

	area = 0.5f * BA.Cross(CA).Magnitude();
}

Vector SceneTriangle::sampleShape(float &pdf)
{
	Vector sample = sampler.samplePlane();

	pdf = 1.0f / area;
	//pdf = sample.x * sample.y; // (normalized sample * area) / area

	return mapSquareSampleToTrianglePoint(sample, vertex[0], vertex[1], vertex[2]);
}

// =================================================================================

void SceneModel::testIntersection(const Ray & ray, HitInfo & outInfo)
{
	HitInfo closer;
	closer.hit = false;
	for (SceneTriangle & st : triangleList)
	{
		st.testIntersection(ray, outInfo);
		if (outInfo.hit)
		{
			if (closer.hit)
			{
				float closerDist = (closer.hitPoint - ray.getOrigin()).Magnitude();
				float currentDist = (outInfo.hitPoint - ray.getOrigin()).Magnitude();

				if (closerDist <= currentDist)
				{
					continue;
				}
			}
			closer = outInfo;
		}
	}

	outInfo = closer;
}

void SceneModel::applyAffineTransformations()
{

	for (auto & triangle : triangleList)
	{
		triangle.position = position;
		triangle.rotation = rotation;
		triangle.scale = scale;
		triangle.applyAffineTransformations();
	}

#ifndef _RT_TRANSFORM_RAY_TO_LOCAL_SPACE
	position = rotation = Vector();
	scale = Vector(1.0f, 1.0f, 1.0f);
#endif
}

void SceneModel::computeArea()
{
	area = 0.0f;
	for (auto & triangle : triangleList)
	{
		triangle.computeArea();
		area += triangle.area;
	}
}

void SceneModel::initSampler()
{
	sampler = IntegerSampler(0, triangleList.size() - 1);
}

Vector SceneModel::sampleShape(float & pdf)
{
	int choosenTriangle = sampler.sampleRect();

	SceneTriangle & st = triangleList[choosenTriangle];
	float trianglePdf;
	Vector triangleFinalSample = st.sampleShape(trianglePdf);

	pdf = trianglePdf * (float(choosenTriangle) / float(triangleList.size() - 1));
	return triangleFinalSample;
}