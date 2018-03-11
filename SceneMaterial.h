#pragma once

#include <string>

#include "pic.h"
#include "stb/stb_image.h"
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
			pic_free(tex);
		}
	}

	// -- Utility Functions --
	// - LoadTexture - Loads a Texture from a filename
	bool LoadTexture(void)
	{
		tex = ReadJPEG ((char *)texture.c_str ());
		if (tex == NULL)
			return false;

		return true;
	}

	// -- Accessor Functions --
	// - GetTextureColor - Returns the texture color at coordinates (u,v)
	Vector GetTextureColor(float u, float v)
	{
		if (tex)
		{
			int textureX = (int)(u * tex->m_width); // ... 3 hours lost wondering why tf the texture mapping wasnt working properly
			int textureY = (int)(v * tex->m_height);// only had to swap m_height and m_width for textureX and textureY 
													// tex->Pixel (ROW, COLUMN, CHANNEL)

			return Vector(tex->Pixel(textureY, textureX, 0),
				tex->Pixel(textureY, textureX, 1),
				tex->Pixel(textureY, textureX, 2)
			);
		}

		return Vector(1.0f, 1.0f, 1.0f);
	}
};
