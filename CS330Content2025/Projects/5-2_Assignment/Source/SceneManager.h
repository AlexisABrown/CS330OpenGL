///////////////////////////////////////////////////////////////////////////////
// scenemanager.h
// ==============
// Defines the `SceneManager` class, which handles the preparation and rendering 
// of 3D scenes. This includes managing textures, materials, lighting, and 
// transformation settings for objects in the scene.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load and manage textures for OpenGL rendering.
// - Define and manage materials and lighting for 3D objects.
// - Handle transformations and shader configurations.
// - Provide methods to render various objects within the 3D scene.
//
// NOTE: This class integrates with the `ShaderManager` and `ShapeMeshes` 
// classes to dynamically prepare and render 3D graphics.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ShaderManager.h"
#include "ShapeMeshes.h"

#include <string>
#include <vector>

/***********************************************************
 *  SceneManager
 *
 *  This class contains the code for preparing and rendering
 *  3D scenes, including the shader settings.
 ***********************************************************/
class SceneManager
{
public:
	// constructor
	SceneManager(ShaderManager *pShaderManager);
	// destructor
	~SceneManager();

	// properties for loaded texture access
	struct TEXTURE_INFO
	{
		std::string tag;
		uint32_t ID;
	};

	// properties for object materials
	struct OBJECT_MATERIAL
	{
		glm::vec3 diffuseColor;
		glm::vec3 specularColor;
		float shininess;
		std::string tag;
	};

private:
	// pointer to shader manager object
	ShaderManager* m_pShaderManager;
	// pointer to basic shapes object
	ShapeMeshes *m_basicMeshes;
	// the number of loaded textures
	int m_loadedTextures;
	// loaded textures info
	TEXTURE_INFO m_textureIDs[16];
	// defined object materials
	std::vector<OBJECT_MATERIAL> m_objectMaterials;

	// methods for managing OpenGL textures
	bool CreateGLTexture(const char* filename, std::string tag);
	void BindGLTextures();
	void DestroyGLTextures();
	int FindTextureID(std::string tag);
	int FindTextureSlot(std::string tag);

	// set the transformation values 
	// into the transform buffer
	void SetTransformations(
		glm::vec3 scaleXYZ,
		float XrotationDegrees,
		float YrotationDegrees,
		float ZrotationDegrees,
		glm::vec3 positionXYZ);

	// set the color values into the shader
	void SetShaderColor(
		float redColorValue,
		float greenColorValue,
		float blueColorValue,
		float alphaValue);

	// set the texture data into the shader
	void SetShaderTexture(
		std::string textureTag);

	// set the texture UV scale into the shader
	void SetTextureUVScale(
		float u, float v);

public:

	/*** The following methods are for the students to ***/
	/*** customize for their own 3D scene              ***/
	void PrepareScene();
	void RenderScene();

	// loads textures from image files
	void LoadSceneTextures();
};