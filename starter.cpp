/*
  15-462 Computer Graphics I
  Assignment 3: Ray Tracer
  C++ starter code
  Author: rtark
  Oct 2007

  Name: [Name]
  AndrewID: [ID]
*/

#include <stdio.h>
#include <stdlib.h>
#include "pic.h"

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
#include "RayTrace.h"
#include "NormalRenderer.h"

/*
	!!!NOTE!!!

	The Width & Height have been reduced to help you debug
	more quickly by reducing the resolution to a quarter of
	the original size (640 x 480). Your code should work for
	any dimension, but more importantly this should be set back
	to the original resolution before you render your submissions.
*/
const int Scene::WINDOW_WIDTH = 512;// 320;
const int Scene::WINDOW_HEIGHT = 512;// 240;

bool Scene::supersample = false;
bool Scene::montecarlo = false;

/* --- Global State Variables --- */

/* - Menu State Identifier - */
int g_iMenuId;

/* - Mouse State Variables - */
int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

/* - RayTrace Variable for the RayTrace Image Generation - */
RayTrace g_RayTrace;

/* - NormalRenderer Variable for drawing with OpenGL calls instead of the RayTracer - */
NormalRenderer g_NormalRenderer;

/* - RayTrace Buffer - */
Vector g_ScreenBuffer[Scene::WINDOW_HEIGHT][Scene::WINDOW_WIDTH]; 

unsigned int g_X = 0, g_Y = 0;
bool g_bRayTrace = false;
bool g_bRenderNormal = true;

/*
	saveScreenshot - Writes a screenshot to the specified filename in JPEG
*/
void saveScreenshot (char *filename)
{
	Pic *in = NULL;

	if (filename == NULL)
		return;

	/* Allocate a picture buffer */
	in = pic_alloc (Scene::WINDOW_WIDTH, Scene::WINDOW_HEIGHT, 3, NULL);

	printf("File to save to: %s\n", filename);

	/* Loop over each row of the image and copy into the image */
	for (int i = Scene::WINDOW_HEIGHT - 1; i >= 0; i--)
	{
		glReadPixels(0, Scene::WINDOW_HEIGHT - 1 - i, Scene::WINDOW_WIDTH, 1, GL_RGB,
						GL_UNSIGNED_BYTE, &in->pix[i*in->m_width*in->m_channels]);
	}

	/* Output the file */
	if (WriteJPEG(filename, in ) )
	{
		printf("File saved Successfully\n");
	}
	else
	{
		printf("Error in Saving\n");
	}

	/* Free memory used by image */
	pic_free(in);
}

/*
	loadTexture - Loads a texture from a JPEG file to memory and returns the handle
		Note: pWidth and pHeight are pointers to return imageWidth and imageHeight
*/
GLuint loadTexture (char *filename, int *pWidth = NULL, int *pHeight = NULL)
{
	GLuint texIndex;

	if (filename == NULL)
		return 0;

	Pic *texture = ReadJPEG(filename );
	if (texture == NULL)
		return 0;

	glGenTextures (1, &texIndex);
	glBindTexture (GL_TEXTURE_2D, texIndex);

	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);

	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	if (pWidth != NULL)
		*pWidth = texture->m_width;
	if (pHeight != NULL)
		*pHeight = texture->m_height;

	switch (texture->m_channels)
	{
	case 1:
	default:
		gluBuild2DMipmaps (GL_TEXTURE_2D, 1, texture->m_width, texture->m_height,
					GL_R3_G3_B2, GL_UNSIGNED_BYTE, texture->pix);
		break;
	case 3:
		gluBuild2DMipmaps (GL_TEXTURE_2D, 3, texture->m_width, texture->m_height,
					GL_RGB, GL_UNSIGNED_BYTE, texture->pix);
		break;
	case 4:
		gluBuild2DMipmaps (GL_TEXTURE_2D, 4, texture->m_width, texture->m_height,
					GL_RGBA8, GL_UNSIGNED_BYTE, texture->pix);
		break;
	}

	pic_free(texture);

	return texIndex;
}

/*
	myinit - Function to add your initialization code
*/
void myinit()
{
	// Default to these camera settings
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity ();
	glOrtho(0, Scene::WINDOW_WIDTH, 0, Scene::WINDOW_HEIGHT, 1, -1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Set the Scene Variable for the NormalRenderer
	g_NormalRenderer.SetScene (&g_RayTrace.m_Scene);

	glClearColor(0, 0, 0, 0);
}

/*
  display - Function to modify with your ray-tracing rendering code 
*/
void display()
{
	if (g_bRenderNormal)
	{
		g_NormalRenderer.RenderScene ();
	}
	else
	{
		// Set up the camera to render pixel-by-pixel
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity ();
		glOrtho(0, Scene::WINDOW_WIDTH, 0, Scene::WINDOW_HEIGHT, 1, -1);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBegin(GL_POINTS);
		{
			for (int y = 0; y < Scene::WINDOW_HEIGHT; y++)
			{
				for (int x = 0; x < Scene::WINDOW_WIDTH; x++)
				{
					glColor3f(g_ScreenBuffer[y][x].x, g_ScreenBuffer[y][x].y, g_ScreenBuffer[y][x].z);
					glVertex2i(x, y);
				}
			}
		}

		glEnd();
	}

	glFlush();

	glutSwapBuffers ();
}

/*
	menufunc - Menu Event-handler
*/
void menufunc(int value)
{
	switch (value)
	{
	case 0:
		// Render Normal
		g_bRayTrace = false;
		g_bRenderNormal = true;
		glutPostRedisplay ();
		break;
	case 1:
		// Start the Ray Tracing
		g_bRayTrace = true;
		g_bRenderNormal = false;
//		g_RayTrace.preRenderInit();
      Scene::supersample = false;
      Scene::montecarlo = false;
		break;
	case 2:
		// Start the Ray Tracing with supersampling
		g_bRayTrace = true;
		g_bRenderNormal = false;
//		g_RayTrace.preRenderInit();
      Scene::supersample = true;
      Scene::montecarlo = false;
		break;
   case 3:
		// Start the Ray Tracing with Monte Carlo
		g_bRayTrace = true;
		g_bRenderNormal = false;
//		g_RayTrace.preRenderInit();
      Scene::supersample = false;
      Scene::montecarlo = true;
		break;
	case 4:
		// Quit Program
		exit(0);
		break;
	}

	g_X = 0;
	g_Y = 0;
}

/*
	doIdle - The idle-function that can be used to update the screen
*/
void doIdle()
{
	if (g_bRayTrace)
	{
		g_RayTrace.Render();
		Vector ** buffer = g_RayTrace.getBuffer();
		
		for (int i = 0; i < Scene::WINDOW_HEIGHT; i++)
		{
			
			for (int j = 0; j < Scene::WINDOW_WIDTH; j++)
			{
				memcpy(&g_ScreenBuffer[i], buffer[i], sizeof(Vector) * Scene::WINDOW_WIDTH);
			}
		}

		g_bRayTrace = false;
		glutPostRedisplay();
		/*
		g_ScreenBuffer[g_Y][g_X] = g_RayTrace.CalculatePixel (g_X, g_Y);

		// Move to the next pixel
		g_X++;
		if (g_X >= Scene::WINDOW_WIDTH)
		{
			// Move to the next row
			g_X = 0;
			g_Y++;

			//You can uncomment the next line 
			//to see the raytrace "in-action"
			//glutPostRedisplay();
		}

		// Check for the end of the screen
		if (g_Y >= Scene::WINDOW_HEIGHT)
		{
			g_bRayTrace = false;
			glutPostRedisplay ();
		}*/
	}
	else
	{
		glutPostRedisplay ();
	}
}

/*
	mousedrag - converts mouse drags into information about rotation/translation/scaling
*/
void mousedrag(int x, int y)
{
	int vMouseDelta[2] = {x - g_vMousePos[0], y - g_vMousePos[1]};

	g_vMousePos[0] = x;
	g_vMousePos[1] = y;
}

/*
	mouseidle - Idle mouse movement callback function
*/
void mouseidle(int x, int y)
{
	g_vMousePos[0] = x;
	g_vMousePos[1] = y;
}

/*
	mousebutton - Sets the global mouse states according to the actions
*/
void mousebutton(int button, int state, int x, int y)
{
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		g_iLeftMouseButton = (state==GLUT_DOWN);
		break;
	case GLUT_MIDDLE_BUTTON:
		g_iMiddleMouseButton = (state==GLUT_DOWN);
		break;
	case GLUT_RIGHT_BUTTON:
		g_iRightMouseButton = (state==GLUT_DOWN);
		break;
	}

	g_vMousePos[0] = x;
	g_vMousePos[1] = y;
}

/*
	processkeys - processes the keyboard keys
*/
void processkeys(unsigned char key, int x, int y)
{
	Vector dir = g_RayTrace.m_Scene.m_Camera.GetTarget () - g_RayTrace.m_Scene.m_Camera.GetPosition ();
	dir.Normalize ();
	Vector right = dir.Cross (g_RayTrace.m_Scene.m_Camera.GetUp ());
	float camSpeed = 0.1f;

	switch (key)
	{
	case 27:
		// Escape Key
		exit(0);

	case 'w':
		// - Move Forward -
		g_RayTrace.m_Scene.m_Camera.SetPosition (g_RayTrace.m_Scene.m_Camera.GetPosition () + dir * camSpeed);
		g_RayTrace.m_Scene.m_Camera.SetTarget (g_RayTrace.m_Scene.m_Camera.GetTarget () + dir * camSpeed);
		break;

	case 's':
		// - Move Backward -
		g_RayTrace.m_Scene.m_Camera.SetPosition (g_RayTrace.m_Scene.m_Camera.GetPosition () - dir * camSpeed);
		g_RayTrace.m_Scene.m_Camera.SetTarget (g_RayTrace.m_Scene.m_Camera.GetTarget () - dir * camSpeed);
		break;

	case 'a':
		// - Strafe Left -
		g_RayTrace.m_Scene.m_Camera.SetPosition (g_RayTrace.m_Scene.m_Camera.GetPosition () - right * camSpeed);
		g_RayTrace.m_Scene.m_Camera.SetTarget (g_RayTrace.m_Scene.m_Camera.GetTarget () - right * camSpeed);
		break;

	case 'd':
		// - Strafe Right -
		g_RayTrace.m_Scene.m_Camera.SetPosition (g_RayTrace.m_Scene.m_Camera.GetPosition () + right * camSpeed);
		g_RayTrace.m_Scene.m_Camera.SetTarget (g_RayTrace.m_Scene.m_Camera.GetTarget () + right * camSpeed);
		break;
	}
}

/*
	processarrowkeys - processes the keyboard arrow keys
*/
void processarrowkeys(int key, int x, int y)
{
	Vector dir = g_RayTrace.m_Scene.m_Camera.GetTarget () - g_RayTrace.m_Scene.m_Camera.GetPosition ();
	float tar_posDist = dir.Magnitude ();
	dir.Normalize ();
	Vector right = dir.Cross (g_RayTrace.m_Scene.m_Camera.GetUp ());
	Vector tempVec;
	float turnSpeed = 0.01f;

	switch (key)
	{
	case GLUT_KEY_LEFT:
		// - Turn Left -
		// Simulate a Left-Turn by finding a position just a little to the left of the direction
		tempVec = dir - right * turnSpeed;
		tempVec.Normalize ();
		g_RayTrace.m_Scene.m_Camera.SetTarget (g_RayTrace.m_Scene.m_Camera.GetPosition () + tempVec * tar_posDist);
		//g_RayTrace.m_Scene.m_Camera.SetUp(right.Cross(tempVec)); // THE UP VECTOR WASNT BEING COMPUTED, CAUSING THE RAY TO BE PERTURBED...
		break;

	case GLUT_KEY_RIGHT:
		// - Turn Right -
		// Simulate a Right-Turn by finding a position just a little to the right of the direction
		tempVec = dir + right * turnSpeed;
		tempVec.Normalize ();
		g_RayTrace.m_Scene.m_Camera.SetTarget (g_RayTrace.m_Scene.m_Camera.GetPosition () + tempVec * tar_posDist);
		//g_RayTrace.m_Scene.m_Camera.SetUp(right.Cross(tempVec)); // THE UP VECTOR WASNT BEING COMPUTED, CAUSING THE RAY TO BE PERTURBED...
		break;

	case GLUT_KEY_UP:
		// - Turn Up -
		// Simulate an Up-Turn by finding a position just a little above the direction
		tempVec = dir + g_RayTrace.m_Scene.m_Camera.GetUp () * turnSpeed;
		tempVec.Normalize ();
		g_RayTrace.m_Scene.m_Camera.SetTarget (g_RayTrace.m_Scene.m_Camera.GetPosition () + tempVec * tar_posDist);
		g_RayTrace.m_Scene.m_Camera.SetUp (right.Cross(tempVec));
		break;

	case GLUT_KEY_DOWN:
		// - Turn Down -
		// Simulate a Down-Turn by finding a position just a little below the direction
		tempVec = dir - g_RayTrace.m_Scene.m_Camera.GetUp () * turnSpeed;
		tempVec.Normalize ();
		g_RayTrace.m_Scene.m_Camera.SetTarget (g_RayTrace.m_Scene.m_Camera.GetPosition () + tempVec * tar_posDist);
		g_RayTrace.m_Scene.m_Camera.SetUp (right.Cross(tempVec));
		break;
	}
}

/*
	main - The Main Function
*/
int main (int argc, char ** argv)
{
	if (argc < 2)
	{
		printf ("usage: %s scenefile\n", argv[0]);
		exit(1);
	}

	if (!g_RayTrace.m_Scene.Load (argv[1]))
	{
		printf ("failed to load scene\n");
		exit(1);
	}

	printf ("Right-click and choose Render to begin Ray-tracing...\n");

	glutInit(&argc,argv);

	/* create a window */
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(Scene::WINDOW_WIDTH, Scene::WINDOW_HEIGHT);

	glutCreateWindow("Trazador de rayos");

	/* tells glut to use a particular display function to redraw */
	glutDisplayFunc(display);

	/* create a right mouse button menu */
	g_iMenuId = glutCreateMenu(menufunc);
	glutSetMenu(g_iMenuId);
	glutAddMenuEntry("Render Normal",0);
	glutAddMenuEntry("Render Ray Tracing",1);
	glutAddMenuEntry("Render Ray Tracing Supersampling",2);
	glutAddMenuEntry("Render Ray Tracing Monte Carlo",3);
	glutAddMenuEntry("Quit",4);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	/* replace with any animate code */
	glutIdleFunc(doIdle);

	/* callback for mouse drags */
	glutMotionFunc(mousedrag);
	/* callback for idle mouse movement */
	glutPassiveMotionFunc(mouseidle);
	/* callback for mouse button changes */
	glutMouseFunc(mousebutton);
	/* callback for keyboard changes */
	glutKeyboardFunc(processkeys);
	/* callback for arrow key changes */
	glutSpecialFunc(processarrowkeys);

	/* do initialization */
	myinit();

	glutMainLoop();

	return 0;
}
