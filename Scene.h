/*
  15-462 Computer Graphics I
  Assignment 3: Ray Tracer
  C++ Scene Descriptor Class
  Author: rtark
  Oct 2007

  NOTE: You do not need to edit this file for this assignment but may do so

  This file defines the following:
	SceneObjectType Enumeration
	SceneBackground Class
	SceneLight Class
	SceneMaterial Class
	SceneObject Class -> SceneSphere, SceneTriangle, SceneModel
	Scene Class

  Scene Usage: Several Functions
	Scene::Load (sceneFile) - load a scene descriptor file

	Scene::GetDescription () - get the scene description string
	Scene::GetAuthor () - get the scene author string
	Scene::GetBackground () - get the scene background information
	Scene::GetNumLights () - get the number of lights in the scene
	Scene::GetLight (lightIndex) - get one of the lights in the scene
	Scene::GetNumMaterials () - get the number of materials in the scene
	Scene::GetMaterial (materialIndex OR materialName) - get a material's description
	Scene::GetNumObjects () - get the number of objects in the scene
	Scene::GetObject (objectIndex) - get an object's description
	Scene::GetCamera - get the current camera description

	The trickiest one of these is the GetObject () function
	It is used like this:
	[The object is type-casted to its corresponding type]

	SceneObject tempObject = m_Scene.GetObject (0);
	if (tempObject.IsTriangle ())
	{
		Vector vertices[3];
		
		for (int n = 0; n < 3; n++)
		{
			vertices[n] = ((SceneTriangle)tempObject).vertex[n];
		}
	}
*/

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <fstream>

#include "Utils.h"
#include "pic.h"

// XML Parser by Frank Vanden Berghen
// Available: http://iridia.ulb.ac.be/~fvandenb/tools/xmlParser.html
#include "xmlParser.h"

// 3DS File by bkenwright@xbdev.net
//    Updated by Raphael Mun
#include "3ds.h"

#include "Ray.h"

// Max Line Length for OBJ File Loading
#define MAX_LINE_LEN 1000

#define CHECK_ATTR(a) (a == NULL ? "" : a)
#define CHECK_ATTR2(a,b) (a == NULL ? b : a)

/*
	SceneObjectType Namespace - Holds the ObjectType Enumeration
*/
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
	SceneBackground Class - The Background properties of a ray-trace scene

	This class defines the background in the Scene
*/
class SceneBackground
{
public:
	Vector color;
	Vector ambientLight;
};

/*
	SceneLight Class - The light properties of a single light-source in a ray-trace scene

	The Scene class holds a list of these
*/
class SceneLight
{
public:
	float attenuationConstant, attenuationLinear, attenuationQuadratic;
	Vector color;
	Vector position;
};

/*
	SceneObject Class - A base object class that defines the common features of all objects

	This is the base object class that the various scene object types derive from
*/
class SceneObject
{
public:
	std::string name;
	SceneObjectType::ObjectType type;
	Vector scale, rotation, position;

	// -- Constructors & Destructors --
	SceneObject (void) { scale.x = 1.0f; scale.y = 1.0f; scale.z = 1.0f; }
	SceneObject (SceneObjectType::ObjectType tp) : type (tp) { scale.x = 1.0f; scale.y = 1.0f; scale.z = 1.0f; }
	SceneObject (std::string nm, SceneObjectType::ObjectType tp) : name(nm), type (tp) { scale.x = 1.0f; scale.y = 1.0f; scale.z = 1.0f; }

	// -- Object Type Checking Functions --
	bool IsSphere (void) { return (type == SceneObjectType::Sphere); }
	bool IsTriangle (void) { return (type == SceneObjectType::Triangle); }
	bool IsModel (void) { return (type == SceneObjectType::Model); }

	virtual bool testIntersection(const Ray & ray, HitInfo & outInfo) = 0;
};

/*
	SceneSphere Class - The sphere scene object

	Sphere object derived from the SceneObject
*/
class SceneSphere : public SceneObject
{
public:
	SceneMaterial * material;
	Vector center;
	float radius;

	// -- Constructors & Destructors --
	SceneSphere (void) : SceneObject ("Sphere", SceneObjectType::Sphere) {}
	SceneSphere (std::string nm) : SceneObject (nm, SceneObjectType::Sphere) {}

	bool testIntersection(const Ray & ray, HitInfo & outInfo);
};

/*
	SceneTriangle Class - The triangle scene object

	Single triangle object derived from the SceneObject
*/
class SceneTriangle : public SceneObject
{
public:
	SceneMaterial * material[3];
	Vector vertex[3];
	Vector normal[3];
	float u[3], v[3];

	// -- Constructors & Destructors --
	SceneTriangle (void) : SceneObject ("Triangle", SceneObjectType::Triangle) {}
	SceneTriangle (std::string nm) : SceneObject (nm, SceneObjectType::Triangle) {}

	bool testIntersection(const Ray & ray, HitInfo & outInfo);
};

/*
	SceneModel Class - The model scene object

	A model object consisting of a list of triangles derived from the SceneObject
*/
class SceneModel : public SceneObject
{
public:
	std::string filename;
	std::vector<SceneTriangle> triangleList;

	// -- Constructors & Destructors --
	SceneModel (void) : SceneObject ("Model", SceneObjectType::Model) {}
	SceneModel (std::string file) : SceneObject ("Model", SceneObjectType::Model) { filename = file; }
	SceneModel (std::string file, std::string nm) : SceneObject (nm, SceneObjectType::Model) { filename = file; }

	// -- Accessor Functions --
	// - GetNumTriangles - Returns the number of triangles in the model
	unsigned int GetNumTriangles (void) { return (unsigned int)triangleList.size (); }

	// - GetTriangle - Gets the nth SceneTriangle
	SceneTriangle *GetTriangle (int triIndex) { return &triangleList[triIndex]; }

	bool testIntersection(const Ray & ray, HitInfo & outInfo);
};

/*
	Scene Class - The main scene definition class with the scene information

	This is the base scene class
*/
class Scene
{
public:
   static const int WINDOW_HEIGHT, WINDOW_WIDTH;
   static bool supersample;
   static bool montecarlo;

private:
	std::string m_Desc, m_Author;
	SceneBackground m_Background;
	std::vector<SceneLight *> m_LightList;
	std::vector<SceneMaterial *> m_MaterialList;
	std::vector<SceneObject *> m_ObjectList;

	// - Private utility Functions used by Load () -
	Vector ParseColor (XMLNode node)
	{
		if (node.isEmpty ())
			return Vector (0.0f, 0.0f, 0.0f);
		return Vector (atof(node.getAttribute("red")), 
						atof(node.getAttribute("green")),
						atof(node.getAttribute("blue")));
	}

	Vector ParseXYZ (XMLNode node)
	{
		if (node.isEmpty ())
			return Vector (0.0f, 0.0f, 0.0f);
		return Vector (atof(node.getAttribute("x")), 
					   atof(node.getAttribute("y")),
					   atof(node.getAttribute("z")));
	}

	void ParseOBJCommand (char *line, int max, char *command, int &position);
	Vector ParseOBJVector (char *str);
	bool ParseOBJCoords (char *str, int &num, int v_index[3], int n_index[3]);
public:
	Camera m_Camera;


	// -- Constructors & Destructors --
	Scene (void) {}
	~Scene (void)
	{
		// Free the memory allocated from the objects
		unsigned int numObj = GetNumObjects ();
		for (unsigned int n = 0; n < numObj; n++)
		{
			SceneObject *sceneObj = m_ObjectList[n];
			if (sceneObj->IsSphere ())
			{
				delete ((SceneSphere *)sceneObj);
			}
			else if (sceneObj->IsTriangle ())
			{
				delete ((SceneTriangle *)sceneObj);
			}
			else if (sceneObj->IsModel ())
			{
				delete ((SceneModel *)sceneObj);
			}
		}
		m_ObjectList.clear ();
      // Free the memory allocated for the materials
      unsigned int numMat = GetNumMaterials ();
		for (unsigned int n = 0; n < numMat; n++)
		{
		   delete m_MaterialList[n];
		}
		m_MaterialList.clear ();
      // Free the memory allocated for the lights
      unsigned int numLight = GetNumLights ();
		for (unsigned int n = 0; n < numLight; n++)
		{
		   delete m_LightList[n];
		}
		m_LightList.clear ();
	}

	// -- Main Functions --
	// - Load - Loads the Scene XML file
	bool Load (char *filename);

	// -- Accessor Functions --
	// - GetDescription - Returns the Description String
	const char * GetDescription (void) { return m_Desc.c_str(); }

	// - GetAuthor - Return the Author String
	const char * GetAuthor (void) { return m_Author.c_str(); }

	// - GetBackground - Returns the SceneBackground
	const SceneBackground& GetBackground (void) const { return m_Background; }

	// - GetNumLights - Returns the number of lights in the scene
	unsigned int GetNumLights (void) { return (unsigned int)m_LightList.size (); }

	// - GetLight - Returns the nth SceneLight
	SceneLight* GetLight (int lightIndex) const { return m_LightList[lightIndex]; }

	// - GetNumMaterials - Returns the number of materials in the scene
	unsigned int GetNumMaterials (void) { return (unsigned int)m_MaterialList.size (); }

	// - GetMaterial - Returns the nth SceneMaterial
	SceneMaterial* GetMaterial (int matIndex) const { return m_MaterialList[matIndex]; }
	SceneMaterial* GetMaterial (std::string matName) const
	{
		unsigned int numMats = (unsigned int)m_MaterialList.size ();
		for (unsigned int n = 0; n < numMats; n++)
		{
			if (matName == m_MaterialList[n]->name)
				return m_MaterialList[n];
		}

		return NULL;
	}

	// - GetNumObjects - Returns the number of objects in the scene
	unsigned int GetNumObjects (void) { return (unsigned int)m_ObjectList.size (); }

	// - GetObject - Returns the nth object [NOTE: The Object will need to be type-casted afterwards]
	SceneObject *GetObject (int objIndex) { return m_ObjectList[objIndex]; }

	// - GetCamera - Returns the camera class
	Camera GetCamera (void) { return m_Camera; }
};




