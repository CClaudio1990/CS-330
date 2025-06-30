///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  MODIFIED BY: Christian Claudio
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
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
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
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
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	// Destroy created OpenGL textures
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
		m_textureIDs[i].ID = 0;
	}

	m_loadedTextures = 0;
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
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
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
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
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

	// Creating textures for objects with error handling for debugging
	bReturn = CreateGLTexture("textures/plane.jpg", "planeTexture");
	if (!bReturn) std::cout << "Failed to load plane.jpg" << std::endl;
	bReturn = CreateGLTexture("textures/red.jpg", "redTexture");
	if (!bReturn) std::cout << "Failed to load red.jpg" << std::endl;
	bReturn = CreateGLTexture("textures/circular-gold.jpg", "midTexture");
	if (!bReturn) std::cout << "Failed to load circular-gold.jpg" << std::endl;
	bReturn = CreateGLTexture("textures/seamless-gold.jpg", "topTexture");
	if (!bReturn) std::cout << "Failed to load seamless-gold.jpg" << std::endl;
	bReturn = CreateGLTexture("textures/cork.jpg", "corkTexture");
	if (!bReturn) std::cout << "Failed to load cork.jpg" << std::endl;
	if (!bReturn) std::cout << "Failed to load seamless-gold.jpg" << std::endl;
	bReturn = CreateGLTexture("textures/gold.jpg", "cupTexture");
	if (!bReturn) std::cout << "Failed to load gold.jpg" << std::endl;
	bReturn = CreateGLTexture("textures/stone.jpg", "bottleTexture");
	if (!bReturn) std::cout << "Failed to load stone.jpg" << std::endl;
	bReturn = CreateGLTexture("textures/copper.jpg", "knobTexture");
	if (!bReturn) std::cout << "Failed to load copper.jpg" << std::endl;
	bReturn = CreateGLTexture("textures/white.jpg", "whiteTexture");
	if (!bReturn) std::cout << "Failed to load white.jpg" << std::endl;
	bReturn = CreateGLTexture("textures/white2.jpg", "lidTexture");
	if (!bReturn) std::cout << "Failed to load white2.jpg" << std::endl;
	bReturn = CreateGLTexture("textures/grip.jpg", "gripTexture");
	if (!bReturn) std::cout << "Failed to load grip.jpg" << std::endl;
	bReturn = CreateGLTexture("textures/pot1.jpg", "pot1");
	if (!bReturn) std::cout << "Failed to load pot1.jpg" << std::endl;
	bReturn = CreateGLTexture("textures/pot2.jpg", "pot2");
	if (!bReturn) std::cout << "Failed to load pot2.jpg" << std::endl;
	bReturn = CreateGLTexture("textures/wall.jpg", "wall");
	if (!bReturn) std::cout << "Failed to load wall.jpg" << std::endl;

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
	// Object Materials

	OBJECT_MATERIAL glossMaterial;
	glossMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glossMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glossMaterial.shininess = 128.0f;
	glossMaterial.tag = "shinier";

	m_objectMaterials.push_back(glossMaterial);

	OBJECT_MATERIAL matteMaterial;
	matteMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.4f);
	matteMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	matteMaterial.shininess = 0.05f;
	matteMaterial.tag = "matte";

	m_objectMaterials.push_back(matteMaterial);

	OBJECT_MATERIAL goldMaterial;
	goldMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	goldMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	goldMaterial.shininess = 256.0f;
	goldMaterial.tag = "gold";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.1f, 0.2f, 0.3f);
	glassMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glassMaterial.shininess = 256.0f;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting - to use the default rendered 
	// lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// directional light for natural light
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.05f, -0.3f, -0.1f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// Point Lights
	m_pShaderManager->setVec3Value("pointLights[0].position", -4.0f, 8.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);
	
	m_pShaderManager->setVec3Value("pointLights[1].position", 4.0f, 8.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);
	
	m_pShaderManager->setVec3Value("pointLights[2].position", 3.8f, 5.5f, 4.0f);
	m_pShaderManager->setVec3Value("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("pointLights[2].specular", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);
	
	m_pShaderManager->setVec3Value("pointLights[3].position", 3.8f, 3.5f, 4.0f);
	m_pShaderManager->setVec3Value("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[3].diffuse", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("pointLights[3].specular", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setBoolValue("pointLights[3].bActive", true);
	
	m_pShaderManager->setVec3Value("pointLights[4].position", -3.2f, 6.0f, -4.0f);
	m_pShaderManager->setVec3Value("pointLights[4].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[4].diffuse", 0.9f, 0.9f, 0.9f);
	m_pShaderManager->setVec3Value("pointLights[4].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setBoolValue("pointLights[4].bActive", true);

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
	// Call LoadSceneTextures
	LoadSceneTextures();
	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();

	// Load all necessary meshes for the scene
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// Bind textures
	BindGLTextures();

	// Declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Tray
	scaleXYZ = glm::vec3(9.5f, 0.5f, 5.5f);
	positionXYZ = glm::vec3(-1.0f, -0.9f, 6.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("planeTexture");
	SetShaderMaterial("matte");
	m_basicMeshes->DrawBoxMesh();

	/*************************************************/
	// Sugar Container
	/*************************************************/
	// Bottom
	scaleXYZ = glm::vec3(1.5f, 1.7f, 1.5f);
	positionXYZ = glm::vec3(2.0f, 0.2f, 5.0f);
	SetTransformations(scaleXYZ, 0.0f, 45.0f, 0.0f, positionXYZ);
	SetShaderTexture("redTexture");
	SetShaderMaterial("shinier");
	m_basicMeshes->DrawBoxMesh();

	// Middle (Pyramid)
	scaleXYZ = glm::vec3(1.5f, 1.5f, 1.5f);
	positionXYZ = glm::vec3(2.0f, 1.8f, 5.0f);
	SetTransformations(scaleXYZ, 0.0f, 45.0f, 0.0f, positionXYZ);
	SetShaderTexture("midTexture");
	SetShaderMaterial("shinier");
	m_basicMeshes->DrawPyramid4Mesh();

	// Top Box
	scaleXYZ = glm::vec3(1.0f, 0.8f, 1.0f);
	positionXYZ = glm::vec3(2.0f, 1.9f, 5.0f);
	SetTransformations(scaleXYZ, 0.0f, 45.0f, 0.0f, positionXYZ);
	SetShaderTexture("topTexture");
	SetShaderMaterial("shinier");
	m_basicMeshes->DrawBoxMesh();

	// Cork on top
	scaleXYZ = glm::vec3(0.7f, 0.4f, 0.7f);
	positionXYZ = glm::vec3(2.0f, 2.3f, 5.0f);
	SetTransformations(scaleXYZ, 0.0f, 45.0f, 0.0f, positionXYZ);
	SetShaderTexture("corkTexture");
	SetShaderMaterial("matte");
	m_basicMeshes->DrawBoxMesh();

	/*************************************************/
	// Gold Cup
	/*************************************************/
	scaleXYZ = glm::vec3(0.7f, 1.2f, 0.7f);
	positionXYZ = glm::vec3(2.5f, -0.6f, 7.4f);
	SetTransformations(scaleXYZ, 0.0f, 45.0f, 0.0f, positionXYZ);
	SetShaderTexture("cupTexture");
	SetShaderMaterial("gold");
	m_basicMeshes->DrawCylinderMesh(false, true, true);
	/*************************************************/

	/*************************************************/
	// Coffee Pot
	/*************************************************/
	// Base
	scaleXYZ = glm::vec3(0.9f, 1.0f, 0.9f);
	positionXYZ = glm::vec3(-0.9f, -0.7f, 5.3f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("pot2");
	SetShaderMaterial("shinier");
	m_basicMeshes->DrawCylinderMesh(false, true, true);

	// Middle
	scaleXYZ = glm::vec3(0.9f, 2.8f, 0.9f);
	positionXYZ = glm::vec3(-0.9f, 0.2f, 5.3f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("pot2");
	SetShaderMaterial("shinier");
	m_basicMeshes->DrawTaperedCylinderMesh(false, false, true);

	// Top
	scaleXYZ = glm::vec3(0.9f, 1.0f, 0.9f);
	positionXYZ = glm::vec3(-0.9f, 3.7f, 5.3f);
	SetTransformations(scaleXYZ, 180.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("pot1");
	SetShaderMaterial("matte");
	m_basicMeshes->DrawTaperedCylinderMesh(false, false, true);

	// Handle Short
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.5f);
	positionXYZ = glm::vec3(-0.9f, 2.3f, 4.5f);
	SetTransformations(scaleXYZ, -78.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("gripTexture");
	SetShaderMaterial("matte");
	m_basicMeshes->DrawBoxMesh();
	
	// Handle Long
	scaleXYZ = glm::vec3(0.4f, 0.5f, 1.6f);
	positionXYZ = glm::vec3(-0.9f, 2.0f, 4.1f);
	SetTransformations(scaleXYZ, -78.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("gripTexture");
	SetShaderMaterial("matte");
	m_basicMeshes->DrawBoxMesh();
	/*************************************************/

	/*************************************************/
	// Bottle
	/*************************************************/
	scaleXYZ = glm::vec3(0.8f, 2.5f, 0.8f);
	positionXYZ = glm::vec3(-2.9f, -0.7f, 5.3f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("bottleTexture");
	SetShaderMaterial("shinier");
	m_basicMeshes->DrawCylinderMesh();

	// Middle
	scaleXYZ = glm::vec3(0.8f, 0.5f, 0.8f);
	positionXYZ = glm::vec3(-2.9f, 1.8f, 5.3f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("bottleTexture");
	SetShaderMaterial("shinier");
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Top
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);
	positionXYZ = glm::vec3(-2.9f, 2.3f, 5.3f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("bottleTexture");
	SetShaderMaterial("shinier");
	m_basicMeshes->DrawCylinderMesh(false, true, true);
	/*************************************************/

	/*************************************************/
	// Cone Object
	/*************************************************/
	scaleXYZ = glm::vec3(0.8f, 2.5f, 0.8f);
	positionXYZ = glm::vec3(-4.6f, 0.0f, 5.3f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("corkTexture");
	SetShaderMaterial("matte");
	m_basicMeshes->DrawConeMesh();

	// Bottom
	scaleXYZ = glm::vec3(0.8f, 0.8f, 0.8f);
	positionXYZ = glm::vec3(-4.6f, -0.02f, 5.3f);
	SetTransformations(scaleXYZ, 180.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("corkTexture");
	SetShaderMaterial("matte");
	m_basicMeshes->DrawHalfSphereMesh();
	/*************************************************/

	/*************************************************/
	// Container
	/*************************************************/
	// Container Base
	// Middle Box
	scaleXYZ = glm::vec3(3.8f, 0.7f, 1.5f);
	positionXYZ = glm::vec3(-1.5f, -0.5f, 7.5f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("whiteTexture");
	SetShaderMaterial("shinier");
	m_basicMeshes->DrawBoxMesh();

	// Edge Cylinder 1
	scaleXYZ = glm::vec3(0.77f, 0.7f, 0.77f);
	positionXYZ = glm::vec3(-3.2f, -0.85f, 7.5f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("whiteTexture");
	SetShaderMaterial("shinier");
	m_basicMeshes->DrawCylinderMesh();

	// Edge Cylinder 2
	scaleXYZ = glm::vec3(0.77f, 0.7f, 0.77f);
	positionXYZ = glm::vec3(0.2f, -0.85f, 7.5f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("whiteTexture");
	SetShaderMaterial("shinier");
	m_basicMeshes->DrawCylinderMesh();

	// Container Lid
	// Middle Box
	scaleXYZ = glm::vec3(3.8f, 0.3f, 1.58f);
	positionXYZ = glm::vec3(-1.5f, 0.0f, 7.5f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("lidTexture");
	SetShaderMaterial("shinier");
	m_basicMeshes->DrawBoxMesh();

	// Edge Cylinder 1
	scaleXYZ = glm::vec3(0.8f, 0.3f, 0.8f);
	positionXYZ = glm::vec3(-3.2f, -0.15f, 7.5f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("lidTexture");
	SetShaderMaterial("shinier");
	m_basicMeshes->DrawCylinderMesh();

	// Edge Cylinder 2
	scaleXYZ = glm::vec3(0.8f, 0.3f, 0.8f);
	positionXYZ = glm::vec3(0.2f, -0.15f, 7.5f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("lidTexture");
	SetShaderMaterial("shinier");
	m_basicMeshes->DrawCylinderMesh();

	// Knob on top
	scaleXYZ = glm::vec3(0.3f, 0.2f, 0.3f);
	positionXYZ = glm::vec3(-1.5f, 0.15f, 7.5f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("knobTexture");
	SetShaderMaterial("shinier");
	m_basicMeshes->DrawCylinderMesh();
	/*************************************************/

	/*************************************************/
	// Backdrop (Wall)
	/*************************************************/
	scaleXYZ = glm::vec3(11.0f, 0.5f, 7.0f);
	positionXYZ = glm::vec3(-1.0f, 5.45f, -0.97f);
	SetTransformations(scaleXYZ, 90.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wall");
	SetShaderMaterial("matte");
	m_basicMeshes->DrawPlaneMesh();
	/*************************************************/

	/*************************************************/
	// Base (Table)
	/*************************************************/
	scaleXYZ = glm::vec3(22.0f, 0.5f, 15.0f);
	positionXYZ = glm::vec3(-1.0f, -1.4f, 6.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("planeTexture");
	SetShaderMaterial("shinier");
	m_basicMeshes->DrawBoxMesh();
	/*************************************************/

	/*************************************************/
	// Glass Sphere
	/*************************************************/
	scaleXYZ = glm::vec3(0.8f, 0.8f, 0.8f);
	positionXYZ = glm::vec3(-5.1f, 0.0f, 7.3f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.1f, 0.2f, 0.3f, 0.8f);
	SetShaderMaterial("glass");
	m_basicMeshes->DrawSphereMesh();
	/*************************************************/
}
