#ifdef _OS_X_
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>	

#elif defined(WIN32)
#include <windows.h>
#include "GL/gl.h"
#include "GL/glu.h"
#include "GL/glut.h"

#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#include "Scene.h"
#include "Config.h"

// ==========================================================

void SceneSphere::testIntersection(const Ray & ray, HitInfo & outHitInfo)
{
	outHitInfo.hit = false;

	// Centro del emisor de rayos y dirección de este
	Vector o = ray.getOrigin();
	Vector l = ray.getDirection();

	Vector OC = o - center;

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
	else if(squareRootResult > 0.0f)
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
		outHitInfo.hitPoint = (o + (l * distance));
		outHitInfo.hitNormal = (outHitInfo.hitPoint - center) / radius;
		outHitInfo.hitNormal.Normalize();
		outHitInfo.hittedMaterial = *material;
		outHitInfo.physicalMaterial = physicalMaterial;
		outHitInfo.inRay = ray;
		outHitInfo.hit = true;
	}
}

// =================================================================================

void SceneTriangle::testIntersection(const Ray & ray, HitInfo & outHitInfo)
{
	outHitInfo.hit = false;

	// Punto desde donde se emite el rayo y su dirección
	Vector center = ray.getOrigin();
	Vector dir = ray.getDirection();
	
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
				outHitInfo.hitPoint = hittedPoint;
				outHitInfo.hitNormal = (normal[0] * a + normal[1] * b + normal[2] * c).Normalize();
				outHitInfo.u = ((abs(u[0]) * a) + (abs(u[1]) * b) + (abs(u[2]) * c));
				outHitInfo.v = ((abs(v[0]) * a) + (abs(v[1]) * b) + (abs(v[2]) * c));
				outHitInfo.u -= floor(outHitInfo.u);
				outHitInfo.v -= floor(outHitInfo.v);
				outHitInfo.hittedMaterial = averageMaterials(a, b, c, outHitInfo.u, outHitInfo.v);
				outHitInfo.physicalMaterial = physicalMaterial;
				outHitInfo.inRay = ray;
				outHitInfo.hit = true;
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

	return averaged;
}

// =================================================================================

void SceneModel::testIntersection(const Ray & ray, HitInfo & outInfo)
{
	HitInfo closer;
	closer.hit = false;
	for (SceneTriangle & st : this->triangleList)
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

// =================================================================================
// =================================================================================

bool Scene::Load (char *filename)
{
	XMLNode tempNode;

	// Open the Scene XML File
	printf ("Loading Scenefile %s...\n", filename);
	XMLNode sceneXML = XMLNode::openFileHelper(filename, "scene");
	if (sceneXML.isEmpty ())
		return false;
	m_Desc = CHECK_ATTR(sceneXML.getAttribute("desc"));
	m_Author = CHECK_ATTR(sceneXML.getAttribute("author"));

	printf ("Description: \n\t%s\n", m_Desc.c_str ());
	printf ("Author: %s\n", m_Author.c_str ());

	// Load the Background
	printf ("Loading Background...\n");
	tempNode = sceneXML.getChildNode("background");
	if (tempNode.isEmpty ())
		return false;
	m_Background.color = ParseColor (tempNode.getChildNode("color"));
	m_Background.ambientLight = ParseColor (tempNode.getChildNode("ambientLight"));

	// Load the Lights
	printf ("Loading Lights...\n");
	tempNode = sceneXML.getChildNode("light_list");
	if (!tempNode.isEmpty ())
	{
		unsigned int numLights = tempNode.nChildNode ("light");
		for (unsigned int n = 0; n < numLights; n++)
		{
			XMLNode tempLightNode = tempNode.getChildNode("light", n);
			if (tempLightNode.isEmpty ())
				return false;
			SceneLight *tempLight = new SceneLight();
			tempLight->color = ParseColor (tempLightNode.getChildNode("color"));
			tempLight->attenuationConstant = atof(CHECK_ATTR(tempLightNode.getChildNode("attenuation").getAttribute ("constant")));
			tempLight->attenuationLinear = atof(CHECK_ATTR(tempLightNode.getChildNode("attenuation").getAttribute ("linear")));
			tempLight->attenuationQuadratic = atof(CHECK_ATTR(tempLightNode.getChildNode("attenuation").getAttribute ("quadratic")));
			tempLight->position = ParseXYZ (tempLightNode.getChildNode("position"));
			m_LightList.push_back (tempLight);
		}
	}

	// Load the Materials
	printf ("Loading Materials...\n");
	tempNode = sceneXML.getChildNode("material_list");
	if (!tempNode.isEmpty ())
	{
		unsigned int numMaterials = tempNode.nChildNode ("material");
		for (unsigned int n = 0; n < numMaterials; n++)
		{
			XMLNode tempMaterialNode = tempNode.getChildNode("material", n);
			if (tempMaterialNode.isEmpty ())
				return false;
			SceneMaterial *tempMaterial = new SceneMaterial();
			tempMaterial->name = CHECK_ATTR(tempMaterialNode.getAttribute("name"));
			tempMaterial->texture = CHECK_ATTR(tempMaterialNode.getChildNode("texture").getAttribute("filename"));
			tempMaterial->diffuse = ParseColor (tempMaterialNode.getChildNode("diffuse"));
			tempMaterial->specular = ParseColor (tempMaterialNode.getChildNode("specular"));
			tempMaterial->shininess = atof(CHECK_ATTR(tempMaterialNode.getChildNode("specular").getAttribute("shininess")));
			tempMaterial->transparent = ParseColor (tempMaterialNode.getChildNode("transparent"));
			tempMaterial->reflective = ParseColor (tempMaterialNode.getChildNode("reflective"));
			tempMaterial->refraction_index = ParseColor (tempMaterialNode.getChildNode("refraction_index"));
			
			if (tempMaterial->texture != "")
			{
				if (!tempMaterial->LoadTexture ())
					return false;
			}

			m_MaterialList.push_back (tempMaterial);
		}
	}

	// Load the Objects
	printf ("Loading Objects...\n");
	tempNode = sceneXML.getChildNode("object_list");
	if (!tempNode.isEmpty ())
	{
		unsigned int numObjects = tempNode.nChildNode ();
		for (unsigned int n = 0; n < numObjects; n++)
		{
			XMLNode tempObjectNode = tempNode.getChildNode(n);
			if (tempObjectNode.isEmpty ())
				return false;
			if (!strcasecmp(tempObjectNode.getName (), "sphere"))
			{
				// Load a Sphere
				SceneSphere *tempSphere = new SceneSphere ();
				tempSphere->name = CHECK_ATTR(tempObjectNode.getAttribute("name"));
				tempSphere->material = GetMaterial(CHECK_ATTR(tempObjectNode.getAttribute("material")));
				tempSphere->radius = atof(CHECK_ATTR(tempObjectNode.getAttribute("radius")));
				tempSphere->scale = ParseXYZ (tempObjectNode.getChildNode("scale"));
				tempSphere->rotation = ParseXYZ (tempObjectNode.getChildNode("rotation"));
				tempSphere->position = ParseXYZ (tempObjectNode.getChildNode("position"));
				tempSphere->center = ParseXYZ (tempObjectNode.getChildNode("center"));
				tempSphere->physicalMaterial = (CHECK_ATTR(tempObjectNode.getChildNode("physicalMaterial").getAttribute("name")));
				m_ObjectList.push_back (tempSphere);
			}
			else if (!strcasecmp(tempObjectNode.getName (), "triangle"))
			{
				// Load a Triangle
				XMLNode vertexNode;
				SceneTriangle *tempTriangle = new SceneTriangle ();
				tempTriangle->name = CHECK_ATTR(tempObjectNode.getAttribute("name"));
				tempTriangle->scale = ParseXYZ (tempObjectNode.getChildNode("scale"));
				tempTriangle->rotation = ParseXYZ (tempObjectNode.getChildNode("rotation"));
				tempTriangle->position = ParseXYZ (tempObjectNode.getChildNode("position"));
				tempTriangle->physicalMaterial = (CHECK_ATTR(tempObjectNode.getChildNode("physicalMaterial").getAttribute("name")));
				
				// Load Vertex 0
				vertexNode = tempObjectNode.getChildNodeWithAttribute ("vertex", "index", "0");
				tempTriangle->material[0] = GetMaterial(CHECK_ATTR(vertexNode.getAttribute("material")));
				tempTriangle->vertex[0] = ParseXYZ (vertexNode.getChildNode("position"));
				tempTriangle->normal[0] = ParseXYZ (vertexNode.getChildNode("normal"));
				tempTriangle->u[0] = atof (CHECK_ATTR(vertexNode.getChildNode("texture").getAttribute("u")));
				tempTriangle->v[0] = atof (CHECK_ATTR(vertexNode.getChildNode("texture").getAttribute("v")));

				// Load Vertex 1
				vertexNode = tempObjectNode.getChildNodeWithAttribute ("vertex", "index", "1");
				tempTriangle->material[1] = GetMaterial(CHECK_ATTR(vertexNode.getAttribute("material")));
				tempTriangle->vertex[1] = ParseXYZ (vertexNode.getChildNode("position"));
				tempTriangle->normal[1] = ParseXYZ (vertexNode.getChildNode("normal"));
				tempTriangle->u[1] = atof (CHECK_ATTR(vertexNode.getChildNode("texture").getAttribute("u")));
				tempTriangle->v[1] = atof (CHECK_ATTR(vertexNode.getChildNode("texture").getAttribute("v")));

				// Load Vertex 2
				vertexNode = tempObjectNode.getChildNodeWithAttribute ("vertex", "index", "2");
				tempTriangle->material[2] = GetMaterial(CHECK_ATTR(vertexNode.getAttribute("material")));
				tempTriangle->vertex[2] = ParseXYZ (vertexNode.getChildNode("position"));
				tempTriangle->normal[2] = ParseXYZ (vertexNode.getChildNode("normal"));
				tempTriangle->u[2] = atof (CHECK_ATTR(vertexNode.getChildNode("texture").getAttribute("u")));
				tempTriangle->v[2] = atof (CHECK_ATTR(vertexNode.getChildNode("texture").getAttribute("v")));

				m_ObjectList.push_back (tempTriangle);
			}
			else if (!strcasecmp(tempObjectNode.getName (), "model"))
			{
				// Load a Model
				SceneModel *tempModel = new SceneModel ();
				tempModel->filename = CHECK_ATTR(tempObjectNode.getAttribute("filename"));
				if (tempModel->filename == "")
				{
					printf ("No Specified Model filename\n");
					return false;
				}
				tempModel->name = CHECK_ATTR(tempObjectNode.getAttribute("name"));
				std::string material = CHECK_ATTR(tempObjectNode.getAttribute("material"));
				tempModel->scale = ParseXYZ (tempObjectNode.getChildNode("scale"));
				tempModel->rotation = ParseXYZ (tempObjectNode.getChildNode("rotation"));
				tempModel->position = ParseXYZ (tempObjectNode.getChildNode("position"));
				tempModel->physicalMaterial = CHECK_ATTR(tempObjectNode.getChildNode("physicalMaterial").getAttribute("name"));
				
				SceneMaterial * materialPtr = GetMaterial(material);

				// Check the file format
				if (tempModel->filename.substr (tempModel->filename.length() - 4, 4) == ".3ds")
				{
					// Load the list of triangles from the .3ds
					C3DS sceneObj;
					if (!sceneObj.Create((char *)tempModel->filename.c_str()))
					{
						printf ("Error loading .3ds file\n");
						return false;
					}

					for (unsigned int obj = 0; obj < (unsigned int)sceneObj.m_iNumMeshs; obj++)
					{
						for (unsigned int n = 0; n < (unsigned int)sceneObj.m_pMeshs[obj].iNumFaces; n++)
						{
							SceneTriangle tempTriangle;

							Vector v1, v2, v3;
							// Vert 1
							v1.x = sceneObj.m_pMeshs[obj].pVerts[sceneObj.m_pMeshs[obj].pFaces[n].corner[0]].x;
							v1.y = sceneObj.m_pMeshs[obj].pVerts[sceneObj.m_pMeshs[obj].pFaces[n].corner[0]].y;
							v1.z = sceneObj.m_pMeshs[obj].pVerts[sceneObj.m_pMeshs[obj].pFaces[n].corner[0]].z;
							// Vert 2
							v2.x = sceneObj.m_pMeshs[obj].pVerts[sceneObj.m_pMeshs[obj].pFaces[n].corner[1]].x;
							v2.y = sceneObj.m_pMeshs[obj].pVerts[sceneObj.m_pMeshs[obj].pFaces[n].corner[1]].y;
							v2.z = sceneObj.m_pMeshs[obj].pVerts[sceneObj.m_pMeshs[obj].pFaces[n].corner[1]].z;
							// Vert 3
							v3.x = sceneObj.m_pMeshs[obj].pVerts[sceneObj.m_pMeshs[obj].pFaces[n].corner[2]].x;
							v3.y = sceneObj.m_pMeshs[obj].pVerts[sceneObj.m_pMeshs[obj].pFaces[n].corner[2]].y;
							v3.z = sceneObj.m_pMeshs[obj].pVerts[sceneObj.m_pMeshs[obj].pFaces[n].corner[2]].z;

							// Normal
							Vector normal = (v1 - v2).Cross(v3 - v2).Normalize();

							// Load Vertex 0
							tempTriangle.material[0] = materialPtr;
							tempTriangle.vertex[0] = v1;
							tempTriangle.normal[0] = normal;
							// Texture Coords

							if (sceneObj.m_pMeshs[obj].bTextCoords)
							{
								tempTriangle.u[0] = sceneObj.m_pMeshs[obj].pTexs[sceneObj.m_pMeshs[obj].pFaces[n].corner[0]].tu;
								tempTriangle.v[0] = sceneObj.m_pMeshs[obj].pTexs[sceneObj.m_pMeshs[obj].pFaces[n].corner[0]].tv;
							}
							else
							{
								tempTriangle.u[0] = 0.0f;
								tempTriangle.v[0] = 0.0f;
							}

							// Load Vertex 1
							tempTriangle.material[1] = materialPtr;
							tempTriangle.vertex[1] = v2;
							tempTriangle.normal[1] = normal;
							// Texture Coords
							if (sceneObj.m_pMeshs[obj].bTextCoords)
							{
								tempTriangle.u[1] = sceneObj.m_pMeshs[obj].pTexs[sceneObj.m_pMeshs[obj].pFaces[n].corner[1]].tu;
								tempTriangle.v[1] = sceneObj.m_pMeshs[obj].pTexs[sceneObj.m_pMeshs[obj].pFaces[n].corner[1]].tv;
							}
							else
							{
								tempTriangle.u[1] = 0.0f;
								tempTriangle.v[1] = 0.0f;
							}

							// Load Vertex 2
							tempTriangle.material[2] = materialPtr;
							tempTriangle.vertex[2] = v3;
							tempTriangle.normal[2] = normal;
							// Texture Coords
							if (sceneObj.m_pMeshs[obj].bTextCoords)
							{
								tempTriangle.u[2] = sceneObj.m_pMeshs[obj].pTexs[sceneObj.m_pMeshs[obj].pFaces[n].corner[2]].tu;
								tempTriangle.v[2] = sceneObj.m_pMeshs[obj].pTexs[sceneObj.m_pMeshs[obj].pFaces[n].corner[2]].tv;
							}
							else
							{
								tempTriangle.u[2] = 0.0f;
								tempTriangle.v[2] = 0.0f;
							}

							tempTriangle.physicalMaterial = tempModel->physicalMaterial;

							tempModel->triangleList.push_back (tempTriangle);
						}
					}

					sceneObj.Release();
				}
				else if (tempModel->filename.substr (tempModel->filename.length() - 4, 4) == ".obj")
				{
					// The following code is a modified version of code from the old RayTracer Code rt_trimesh.cpp
					char line[MAX_LINE_LEN];
					char command[MAX_LINE_LEN];
					int position;
					vector<Vector> vertices;
					vector<Vector> normals;

					std::ifstream infile (tempModel->filename.c_str());

					if (infile.fail() )
					{
						printf ("Error loading .obj file\n");
						return false;
					}

					while (infile.good ())
					{
						infile.getline (line, MAX_LINE_LEN);
						ParseOBJCommand (line, MAX_LINE_LEN, command, position);

						if (strcmp (command,"v")==0)
						{
							Vector pos = ParseOBJVector (&(line[position]));
							vertices.push_back (pos);
						}
						else if (strcmp (command,"vn")==0)
						{
							Vector norm = ParseOBJVector (&(line[position]));
							normals.push_back (norm);
						}
						else if (strcmp (command,"f")==0)
						{
							int num = 0; // number of edges
							int v_index[3]; // vertex index
							int n_index[3]; // normal index

							if (!ParseOBJCoords (&(line[position]), num, v_index, n_index))
							{
								printf ("Error parsing faces in .obj file\n");
								return false;
							}

							SceneTriangle tempTriangle;

							// Load Vertex 0
							tempTriangle.material[0] = materialPtr;
							tempTriangle.vertex[0] = vertices[v_index[0]];
							tempTriangle.normal[0] = normals[n_index[0]];
							// Texture Coords
							tempTriangle.u[0] = 0.0f;
							tempTriangle.v[0] = 0.0f;

							// Load Vertex 1
							tempTriangle.material[1] = materialPtr;
							tempTriangle.vertex[1] = vertices[v_index[1]];
							tempTriangle.normal[1] = normals[n_index[1]];
							// Texture Coords
							tempTriangle.u[1] = 0.0f;
							tempTriangle.v[1] = 0.0f;

							// Load Vertex 2
							tempTriangle.material[2] = materialPtr;
							tempTriangle.vertex[2] = vertices[v_index[2]];
							tempTriangle.normal[2] = normals[n_index[2]];
							// Texture Coords
							tempTriangle.u[2] = 0.0f;
							tempTriangle.v[2] = 0.0f;

							tempTriangle.physicalMaterial = tempModel->physicalMaterial;

							tempModel->triangleList.push_back (tempTriangle);
						}
						else
						{
							//printf ("Ignoring command <%s> in obj file\n", command);
						}
					}
					infile.close ();
				}
				else
				{
					printf ("Unsupported file format\n");
					return false;
				}

				m_ObjectList.push_back (tempModel);
			}
			else
			{
				printf ("Unrecognized Node <%s> in <object_list>\n", tempObjectNode.getName ());
				exit (255);
			}
		}
	}

	// Load the Camera
	printf ("Loading Camera...\n");
	tempNode = sceneXML.getChildNode("camera");
	if (tempNode.isEmpty ())
		return false;
	m_Camera.SetFOV (atof(CHECK_ATTR2(tempNode.getAttribute("fieldOfView"), "45.0")));
	m_Camera.SetNearClip (atof(CHECK_ATTR2(tempNode.getAttribute("nearClip"), "0.1")));
	m_Camera.SetFarClip (atof(CHECK_ATTR2(tempNode.getAttribute("farClip"), "100.0")));
	m_Camera.SetPosition (ParseXYZ (tempNode.getChildNode("position")));
	m_Camera.SetTarget (ParseXYZ (tempNode.getChildNode("target")));
	m_Camera.SetUp (ParseXYZ (tempNode.getChildNode("up")));

	printf ("Scene Loaded!\n");

	return true;
}

void Scene::ParseOBJCommand (char *line, int max, char *command, int &position)
{
	int i = 0;
	int j = 0;

	while (i<max && line[i]==' ') i++;
	while (i<max && line[i]!='\0' && line[i]!=' ')
	{
		command[j] = line[i];
		j++; i++;
	}

	command[j] = '\0';
	position = i;
}

Vector Scene::ParseOBJVector (char *str)
{
	int i = 0;
	float x,y,z;
	while (str[i]!='\0' && str[i]==' ') i++;
	x = atof (&(str[i]));
	while (str[i]!='\0' && str[i]!=' ') i++;
	y = atof (&(str[i]));
	while (str[i]!='\0' && str[i]==' ') i++;
	while (str[i]!='\0' && str[i]!=' ') i++;
	z = atof (&(str[i]));
	return Vector (x,y,z);
}

bool Scene::ParseOBJCoords (char *str, int &num, int v_index[3], int n_index[3])
{
	int i = 0;
	num = 0;

	while (str[i]!='\0' && str[i]==' ') i++;
	while (str[i]!='\0')
	{
		while (str[i]!='\0' && str[i]!=' ') i++;
		while (str[i]!='\0' && str[i]==' ') i++;
		num++;
	}
	if (num != 3)
		return false;

	i = 0;
	for (int j=0; j<num; j++)
	{
		while (str[i]==' ') i++;
		v_index[j] = atoi (&(str[i])) - 1;
		while (str[i]!='/' && str[i]!=' ') i++;
		if (str[i] != '/')
			return false;
		i++;
		while (str[i]!='/' && str[i]!=' ') i++;
		if (str[i] != '/')
			return false;
		i++;
		n_index[j] = atoi (&(str[i])) - 1;
		while(str[i]!='\0' && str[i]!=' ') i++;
	}

	return true;
}