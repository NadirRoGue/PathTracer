#pragma once

#include <vector>

#include "Utils.h"
#include "Ray.h"
#include "Sampler.h"
#include "BoundingBox.h"

namespace SceneObjectType
{
	enum ObjectType
	{
		Sphere = 0,
		Triangle = 1,
		Model = 2,
	};
};

/*
SceneObject Class - A base object class that defines the common features of all objects

This is the base object class that the various scene object types derive from
*/
class SceneObject
{
protected:
	bool isLight;
	Vector emission;
public:
	std::string name;
	SceneObjectType::ObjectType type;
	Vector scale, rotation, position;

	std::string physicalMaterial;

#ifdef _RT_TRANSFORM_RAY_TO_LOCAL_SPACE
	Matrix worldToLocalMatrix;
	Matrix localToWorldMatrix;
#endif

	// -- Constructors & Destructors --
	SceneObject(void): isLight(false) { scale.x = 1.0f; scale.y = 1.0f; scale.z = 1.0f; }
	SceneObject(SceneObjectType::ObjectType tp) : isLight(false),type(tp) { scale.x = 1.0f; scale.y = 1.0f; scale.z = 1.0f; }
	SceneObject(std::string nm, SceneObjectType::ObjectType tp) : isLight(false),name(nm), type(tp) { scale.x = 1.0f; scale.y = 1.0f; scale.z = 1.0f; }
	~SceneObject() {}

	// -- Object Type Checking Functions --
	bool IsSphere(void) { return (type == SceneObjectType::Sphere); }
	bool IsTriangle(void) { return (type == SceneObjectType::Triangle); }
	bool IsModel(void) { return (type == SceneObjectType::Model); }

	void setEmissive(Vector em)
	{
		emission = em;
		isLight = true;
	}

	virtual void computeArea() { }

	virtual void testIntersection(const Ray & ray, HitInfo & outInfo) = 0;
	virtual void applyAffineTransformations() = 0;
	virtual Vector sampleShape(float &pdf) = 0;

#ifdef _RT_TRANSFORM_RAY_TO_LOCAL_SPACE
	void computeMatrices()
	{
		Matrix scaleMatrix;
		scaleMatrix.setScaleMatrix(scale);

		Matrix translateMatrix;
		translateMatrix.setTranslateMatrix(position);

		Quaternion q(rotation);
		Matrix rotationMatrix = q.getRotationMatrix();

		localToWorldMatrix = (rotationMatrix * translateMatrix * scaleMatrix);

		Matrix invT = translateMatrix.Inverse();
		Matrix invS = scaleMatrix.Inverse();
		Matrix invR = rotationMatrix.Inverse();

		worldToLocalMatrix = localToWorldMatrix.Inverse();//(invS * invT * invR);
	}
#endif
};

/*
SceneSphere Class - The sphere scene object

Sphere object derived from the SceneObject
*/
class SceneSphere : public SceneObject
{
private:
	FloatSampler sampler;
public:
	SceneMaterial * material;
	Vector center;
	float radius;
	float area;
	
	// -- Constructors & Destructors --
	SceneSphere(void) : SceneObject("Sphere", SceneObjectType::Sphere) { }
	SceneSphere(std::string nm) : SceneObject(nm, SceneObjectType::Sphere) { }

	void testIntersection(const Ray & ray, HitInfo & outInfo);
	void applyAffineTransformations();
	Vector sampleShape(float &pdf);
};

/*
SceneTriangle Class - The triangle scene object

Single triangle object derived from the SceneObject
*/
class SceneTriangle : public SceneObject
{
private:
	FloatSampler sampler;
public:
	SceneMaterial * material[3];
	Vector vertex[3];
	Vector normal[3];
	float u[3], v[3];
	float area;

	// -- Constructors & Destructors --
	SceneTriangle(void) : SceneObject("Triangle", SceneObjectType::Triangle) {}

	SceneTriangle(std::string nm) : SceneObject(nm, SceneObjectType::Triangle) {}

	void testIntersection(const Ray & ray, HitInfo & outInfo);
	void applyAffineTransformations();
	void computeArea();
	Vector sampleShape(float &pdf);
	
private:
	SceneMaterial averageMaterials(float u, float v, float w, float finalU, float finalV);
};

/*
SceneModel Class - The model scene object

A model object consisting of a list of triangles derived from the SceneObject
*/
class SceneModel : public SceneObject
{
private:
	IntegerSampler sampler;
#ifdef _RT_USE_BB
	BoundingBox box;
#endif
public:
	std::string filename;
	std::vector<SceneTriangle> triangleList;
	float area;

	// -- Constructors & Destructors --
	SceneModel(void) : SceneObject("Model", SceneObjectType::Model) {}
	SceneModel(std::string file) : SceneObject("Model", SceneObjectType::Model) { filename = file; }
	SceneModel(std::string file, std::string nm) : SceneObject(nm, SceneObjectType::Model) { filename = file; 	}

	// -- Accessor Functions --
	// - GetNumTriangles - Returns the number of triangles in the model
	unsigned int GetNumTriangles(void) { return (unsigned int)triangleList.size(); }

	// - GetTriangle - Gets the nth SceneTriangle
	SceneTriangle *GetTriangle(int triIndex) { return &triangleList[triIndex]; }

	void initSampler();

	void testIntersection(const Ray & ray, HitInfo & outInfo);
	void applyAffineTransformations();
	void computeArea();
	Vector sampleShape(float &pdf);
};