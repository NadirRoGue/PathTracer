#pragma once

#include <string>

#include "pic.h"
#include "Utils.h"

class SceneMaterial
{
	Pic *tex;
public:
	std::string name;
	std::string texture;
	Vector diffuse;
	Vector specular;
	float shininess;
	Vector transparent;
	Vector reflective;
	Vector refraction_index;

	// -- Constructors & Destructors --
	SceneMaterial(void) : tex(NULL)
	{}

	~SceneMaterial(void)
	{
		if (tex)
		{
			//pic_free (tex);
		}
	}

	// -- Utility Functions --
	// - LoadTexture - Loads a Texture from a filename
	bool LoadTexture(void)
	{
		/*tex = jpeg_read ((char *)texture.c_str (), NULL);
		if (tex == NULL)
		return false;
		*/
		return true;
	}

	// -- Accessor Functions --
	// - GetTextureColor - Returns the texture color at coordinates (u,v)
	Vector GetTextureColor(float u, float v)
	{
		if (tex)
		{
			int textureX = (int)(u * tex->m_width);
			int textureY = (int)(v * tex->m_height);

			return Vector(tex->Pixel(textureX, textureY, 0),
				tex->Pixel(textureX, textureY, 1),
				tex->Pixel(textureX, textureY, 2)
			);
		}

		return Vector(1.0f, 1.0f, 1.0f);
	}
};
