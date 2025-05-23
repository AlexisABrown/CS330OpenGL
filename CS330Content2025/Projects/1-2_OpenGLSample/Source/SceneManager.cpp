///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes. It 
// handles textures, materials, lighting configurations, and object rendering.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load, bind, and manage textures in OpenGL.
// - Define materials and lighting properties for 3D objects.
// - Manage transformations and shader configurations.
// - Render complex 3D scenes using basic meshes.
//
// NOTE: This implementation leverages external libraries like `stb_image` for 
// texture loading and GLM for matrix and vector operations.
///////////////////////////////////////////////////////////////////////////////


#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

// declaration of global variables and defines
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	// create the shape meshes object
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// free the allocated objects
	m_pShaderManager = NULL;
	if (NULL != m_basicMeshes)
	{
		delete m_basicMeshes;
		m_basicMeshes = NULL;
	}

	// free the allocated OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL, 
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename, 
		&width, 
		&height, 
		&colorChannels, 
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the 
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}
	
	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL &material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** The code in the methods BELOW is for preparing and     ***/
/*** rendering the 3D replicated scenes.                    ***/
/**************************************************************/

/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"textures/rusticwood.jpg",
		"table");

	bReturn = CreateGLTexture(
		"textures/cheese_wheel.jpg",
		"cheese_wheel_side");

	bReturn = CreateGLTexture(
		"textures/cheese_top.jpg",
		"cheese_wheel_top");

	bReturn = CreateGLTexture(
		"textures/breadcrust.jpg",
		"breadcrust");

	bReturn = CreateGLTexture(
		"textures/backdrop.jpg",
		"backdrop");

	bReturn = CreateGLTexture(
		"textures/knife_handle.jpg",
		"knifehandle");

	bReturn = CreateGLTexture(
		"textures/stainless.jpg",
		"stainless");

	bReturn = CreateGLTexture(
		"textures/cheddar.jpg",
		"cheddar");

	bReturn = CreateGLTexture(
		"textures/circular-brushed-gold-texture.jpg",
		"knifescrew");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	goldMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.6f);
	goldMaterial.shininess = 52.0;
	goldMaterial.tag = "metal";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	woodMaterial.shininess = 0.1;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	glassMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glassMaterial.shininess = 95.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL plateMaterial;
	plateMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	plateMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	plateMaterial.shininess = 30.0;
	plateMaterial.tag = "plate";

	m_objectMaterials.push_back(plateMaterial);

	OBJECT_MATERIAL cheeseMaterial;
	cheeseMaterial.diffuseColor = glm::vec3(0.6f, 0.5f, 0.3f);
	cheeseMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	cheeseMaterial.shininess = 0.1;
	cheeseMaterial.tag = "cheese";

	m_objectMaterials.push_back(cheeseMaterial);

	OBJECT_MATERIAL breadMaterial;
	breadMaterial.diffuseColor = glm::vec3(0.7f, 0.6f, 0.5f);
	breadMaterial.specularColor = glm::vec3(0.02f, 0.02f, 0.02f);
	breadMaterial.shininess = 0.001;
	breadMaterial.tag = "bread";

	m_objectMaterials.push_back(breadMaterial);

	OBJECT_MATERIAL darkBreadMaterial;
	darkBreadMaterial.diffuseColor = glm::vec3(0.5f, 0.4f, 0.3f);
	darkBreadMaterial.specularColor = glm::vec3(0.01f, 0.01f, 0.01f);
	darkBreadMaterial.shininess = 0.001;
	darkBreadMaterial.tag = "darkbread";

	m_objectMaterials.push_back(darkBreadMaterial);

	OBJECT_MATERIAL backdropMaterial;
	backdropMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.9f);
	backdropMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	backdropMaterial.shininess = 2.0;
	backdropMaterial.tag = "backdrop";

	m_objectMaterials.push_back(backdropMaterial);

	OBJECT_MATERIAL grapeMaterial;
	grapeMaterial.diffuseColor = glm::vec3(0.4f, 0.2f, 0.4f);
	grapeMaterial.specularColor = glm::vec3(0.1f, 0.05f, 0.1f);
	grapeMaterial.shininess = 0.55;
	grapeMaterial.tag = "grape";

	m_objectMaterials.push_back(grapeMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light 
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable lighting in the shader
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Directional light setup
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.05f, -0.3f, -0.1f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// Point light 1
	m_pShaderManager->setVec3Value("pointLights[0].position", -4.0f, 8.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// Point light 2
	m_pShaderManager->setVec3Value("pointLights[1].position", 4.0f, 8.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("pointLights[1].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[1].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[1].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	// Point light 3
	m_pShaderManager->setVec3Value("pointLights[2].position", 3.8f, 5.5f, 4.0f);
	m_pShaderManager->setVec3Value("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("pointLights[2].specular", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setFloatValue("pointLights[2].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[2].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[2].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);

	// Point light 4
	m_pShaderManager->setVec3Value("pointLights[3].position", 3.8f, 3.5f, 4.0f);
	m_pShaderManager->setVec3Value("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[3].diffuse", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("pointLights[3].specular", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setFloatValue("pointLights[3].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[3].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[3].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[3].bActive", true);

	// Point light 5
	m_pShaderManager->setVec3Value("pointLights[4].position", -3.2f, 6.0f, -4.0f);
	m_pShaderManager->setVec3Value("pointLights[4].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[4].diffuse", 0.9f, 0.9f, 0.9f);
	m_pShaderManager->setVec3Value("pointLights[4].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("pointLights[4].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[4].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[4].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[4].bActive", true);

	// Spotlight setup
	m_pShaderManager->setVec3Value("spotLight.ambient", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("spotLight.specular", 0.7f, 0.7f, 0.7f);
	m_pShaderManager->setFloatValue("spotLight.constant", 1.0f);
	m_pShaderManager->setFloatValue("spotLight.linear", 0.09f);
	m_pShaderManager->setFloatValue("spotLight.quadratic", 0.032f);
	m_pShaderManager->setFloatValue("spotLight.cutOff", glm::cos(glm::radians(42.5f)));
	m_pShaderManager->setFloatValue("spotLight.outerCutOff", glm::cos(glm::radians(48.0f)));
	m_pShaderManager->setBoolValue("spotLight.bActive", true);
}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the texture image files for the textures applied
	// to objects in the 3D scene
	LoadSceneTextures();
	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	RenderTable();
	RenderBackdrop();
	RenderCheeseWheel();
	RenderBreadLoaf();
	RenderWineBottle();
	RenderWineGlass();
	RenderGrapes();
	RenderPlateAndKnife();
}

/***********************************************************
 *  RenderTable()
 *
 *  This method is called to render the shapes for the table
 *  object.
 ***********************************************************/
void SceneManager::RenderTable()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, .6f, 8.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.2f, -0.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("table");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawBoxMesh();
}

/***********************************************************
 *  RenderBackdrop()
 *
 *  This method is called to render the shapes for the scene
 *  backdrop object.
 ***********************************************************/
void SceneManager::RenderBackdrop()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 20.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 15.0f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("backdrop");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("backdrop");

	// draw the mesh with transformation values - this plane is used for the backdrop
	m_basicMeshes->DrawPlaneMesh();
}

/***********************************************************
 *  RenderCheeseWheel()
 *
 *  This method is called to render the shapes for the cheese
 *  wheel object.
 ***********************************************************/
void SceneManager::RenderCheeseWheel()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 1.2f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 0.5f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("cheese_wheel_side");
	SetTextureUVScale(5.0, 1.0);
	SetShaderMaterial("cheese");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawCylinderMesh(false, false, true);

	SetShaderTexture("cheese_wheel_top");
	SetTextureUVScale(1.0, 1.0);

	m_basicMeshes->DrawCylinderMesh(true, false, false);
}

/***********************************************************
 *  RenderBreadLoaf()
 *
 *  This method is called to render the shapes for the loaf
 *  of bread object.
 ***********************************************************/
void SceneManager::RenderBreadLoaf()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 1.0f, .9f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.5f, 1.2f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("breadcrust");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("bread");

	m_basicMeshes->DrawHalfSphereMesh();

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, .6f, .9f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.5f, 1.2f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("breadcrust");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("darkbread");

	m_basicMeshes->DrawHalfSphereMesh();
}

/***********************************************************
 *  RenderWineGlass()
 *
 *  This method is called to render the shapes for the wine
 *  glass object.
 ***********************************************************/
void SceneManager::RenderWineGlass()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.8f, 0.06f, .8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.54f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.7, .7, .8, 0.3);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawCylinderMesh();

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.4f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.6f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 0.3);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawTaperedCylinderMesh(false, false, true);

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 1.5f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 1.0f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.7, .7, .8, 0.3);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawCylinderMesh(false, false, true);

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.4f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 2.896f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.7, .7, .8, 0.3);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawTaperedCylinderMesh(false, false, true);

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.8f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.68f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.3, 0.1, 0.4, 0.8);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawHalfSphereMesh();

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.99f, 1.5f, .99f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.68f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.7, .7, .8, 0.3);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawTaperedCylinderMesh(false, false, true);
}

/***********************************************************
 *  RenderWineBottle()
 *
 *  This method is called to render the shapes for the wine
 *  bottle object.
 ***********************************************************/
void SceneManager::RenderWineBottle()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	
	/*** Set needed transformations before drawing the basic mesh ***/
	
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.9f, .3f, .9f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.8f, 0.9f, -2.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.07, 0.2, .08, .95);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values - this half sphere is used for the bottom of the bottle
	m_basicMeshes->DrawHalfSphereMesh();
	
	/*** Set needed transformations before drawing the basic mesh ***/
	
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.9f, 4.0f, .9f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.8f, .9f, -2.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// draw the mesh with transformation values - this cylinder is used for the main bottle section
	m_basicMeshes->DrawCylinderMesh(false, false, true);
	
	/*** Set needed transformations before drawing the basic mesh ***/
	
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.905f, .9f, .905f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -6.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.8f, 4.9f, -2.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values - this half spere is used for the rounded bottle top
	m_basicMeshes->DrawHalfSphereMesh();
	
	/*** Set needed transformations before drawing the basic mesh ***/
	
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.3f, 2.0f, .3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.8f, 5.6f, -2.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values - this cylinder is used for the bottle top
	m_basicMeshes->DrawCylinderMesh(false, false, true);
	
	/*** Set needed transformations before drawing the basic mesh ***/
	
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.32f, .32f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.8f, 7.4f, -2.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values - this torus is used on the bottle top
	m_basicMeshes->DrawTorusMesh();
	
	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.28f, .28f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.8f, 7.6f, -2.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values - this torus is used for the rim on top of the bottle
	m_basicMeshes->DrawTorusMesh();
}

/***********************************************************
 *  RenderGrapes()
 *
 *  This method is called to render the shapes for the grapes
 *  object.
 ***********************************************************/
void SceneManager::RenderGrapes()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.23f, 0.21f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.3f, 0.7f, 1.1f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.2, 0.1, .4, 1.0);
	SetShaderMaterial("grape");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawSphereMesh();

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.23f, 0.21f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.6f, 0.7f, 1.4f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.2, 0.1, .4, 1.0);
	SetShaderMaterial("grape");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawSphereMesh();

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.23f, 0.21f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.1f, 0.7f, 1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.2, 0.1, .4, 1.0);
	SetShaderMaterial("grape");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawSphereMesh();

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.22f, 0.19f, 0.18f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.3f, 0.96f, 1.28f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.2, 0.1, .4, 1.0);
	SetShaderMaterial("grape");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawSphereMesh();

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.23f, 0.21f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.9f, 0.7f, 1.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.2, 0.1, .4, 1.0);
	SetShaderMaterial("grape");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawSphereMesh();

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.21f, 0.19f, 0.17f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.5f, 0.7f, 1.4f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.2, 0.1, .4, 1.0);
	SetShaderMaterial("grape");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawSphereMesh();

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.22f, 0.19f, 0.17f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.76f, 0.95f, 1.44f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.2, 0.1, .4, 1.0);
	SetShaderMaterial("grape");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawSphereMesh();

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.21f, 0.19f, 0.17f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.7f, 0.7f, 1.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.2, 0.1, .4, 1.0);
	SetShaderMaterial("grape");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawSphereMesh();

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.18f, 0.16f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.30f, .70f, 1.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.2, 0.1, .4, 1.0);
	SetShaderMaterial("grape");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawSphereMesh();

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.02f, 0.90f, 0.02f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 15.0f;
	ZrotationDegrees = 100.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 0.85f, 1.14f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.2, 0.4, .2, 1.0);
	SetShaderMaterial("grape");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawCylinderMesh();
}

/***********************************************************
 *  RenderPlateAndKnife()
 *
 *  This method is called to render the shapes for the plate
 *  and knife objects.
 ***********************************************************/
void SceneManager::RenderPlateAndKnife()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.46f, 0.08f, 0.46f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.7f, 0.55f, 1.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1.0);
	SetShaderMaterial("plate");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.06f, 0.1f, 1.06f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.7f, .71f, 1.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1.0);
	SetShaderMaterial("plate");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawHalfSphereMesh();
	/******************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.3f, 0.18f, 0.20f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 20.0f;
	ZrotationDegrees = 4.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.2f, .64f, 1.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("knifehandle");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.2f, 2.0f, 0.01f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 110.0f;
	ZrotationDegrees = 4.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.2f, .75f, 1.395f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("stainless");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawPyramid4Mesh();
	/******************************************************************/
	
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(.6f, 0.25f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 8.0f;
	YrotationDegrees = -140.0f;
	ZrotationDegrees = -6.4f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.1f, .785f, 2.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("cheddar");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("cheese");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawPrismMesh();
	/******************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.05f, 0.186f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 4.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-.7f, 0.584f, 1.73f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("knifescrew");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawCylinderMesh(true, true, false);
	/******************************************************************/
}