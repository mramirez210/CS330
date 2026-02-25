///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>
#include "ViewManager.h"


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
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
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
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
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

	modelView = translation * rotationX * rotationY * rotationZ * scale;

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
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}
/**************************************************************/
/* Setting up the lighting */
void SceneManager::SetupSceneLights()
{
	if (m_pShaderManager != nullptr)
	{
		m_pShaderManager->setVec3Value("dirLight.direction", glm::vec3(-0.5f, -0.8f, 0.8f));
		m_pShaderManager->setVec3Value("dirLight.ambient", glm::vec3(0.3f, 0.3f, 0.3f));
		m_pShaderManager->setVec3Value("dirLight.diffuse", glm::vec3(0.7f, 0.7f, 0.7f));

		m_pShaderManager->setVec3Value("spotLight.position", glm::vec3(5.5f, 4.0f, 0.5f));
		m_pShaderManager->setVec3Value("spotLight.direction", glm::vec3(-0.8f, -1.0f, -0.2f));

		m_pShaderManager->setVec3Value("spotLight.ambient", glm::vec3(0.1f, 0.1f, 0.1f));
		m_pShaderManager->setVec3Value("spotLight.diffuse", glm::vec3(1.0f, 0.95f, 0.8f)); // Warm bulb color
		m_pShaderManager->setVec3Value("spotLight.specular", glm::vec3(1.0f, 1.0f, 1.0f));

		m_pShaderManager->setFloatValue("spotLight.constant", 1.0f);
		m_pShaderManager->setFloatValue("spotLight.linear", 0.045f);
		m_pShaderManager->setFloatValue("spotLight.quadratic", 0.0075f);

		m_pShaderManager->setFloatValue("spotLight.cutOff", glm::cos(glm::radians(15.0f)));
		m_pShaderManager->setFloatValue("spotLight.outerCutOff", glm::cos(glm::radians(25.0f)));
	}
}
/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	LoadSceneTextures();
	LoadSceneMaterials();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadPyramid3Mesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadCylinderMesh();
}
void SceneManager::LoadSceneMaterials()
{

	OBJECT_MATERIAL marble;
	marble.tag = "marble";
	marble.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f); 
	marble.ambientStrength = 0.4f;
	marble.diffuseColor = glm::vec3(0.9f, 0.9f, 0.9f); 
	marble.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	marble.shininess = 16.0f;
	m_objectMaterials.push_back(marble);

	OBJECT_MATERIAL gold;
	gold.tag = "gold";
	gold.ambientColor = glm::vec3(0.25f, 0.20f, 0.07f);
	gold.ambientStrength = 0.3f;
	gold.diffuseColor = glm::vec3(0.8f, 0.65f, 0.25f);  
	gold.specularColor = glm::vec3(0.65f, 0.55f, 0.35f);
	gold.shininess = 51.2f;
	m_objectMaterials.push_back(gold);

	OBJECT_MATERIAL granite;
	granite.tag = "granite";
	granite.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	granite.ambientStrength = 0.35f;
	granite.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f);
	granite.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	granite.shininess = 8.0f;
	m_objectMaterials.push_back(granite);

	OBJECT_MATERIAL wall;
	wall.tag = "wall";
	wall.ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);
	wall.ambientStrength = 0.5f;
	wall.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	wall.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	wall.shininess = 1.0f;
	m_objectMaterials.push_back(wall);

	OBJECT_MATERIAL lamp;
	lamp.tag = "lamp";
	lamp.ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);
	lamp.ambientStrength = 0.5f;
	lamp.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	lamp.specularColor = glm::vec3(0.2f, 0.2f, 0.2f); // Low shine
	lamp.shininess = 2.0f;
	m_objectMaterials.push_back(lamp);
}
/**************************************************************/
/* LoadingScreenTexture() */
void SceneManager::LoadSceneTextures() {
	bool bReturn = false;
	bReturn = CreateGLTexture(
		"textures/wood.jpg", "wood");
	bReturn = CreateGLTexture(
		"textures/wall.jpg", "wall");
	bReturn = CreateGLTexture(
		"textures/pot.jpg", "pot");
	bReturn = CreateGLTexture(
		"textures/leaf.jpg", "leaf");
	bReturn = CreateGLTexture(
		"textures/lamp.jpg", "lamp");
	bReturn = CreateGLTexture(
		"textures/marble.jpg", "marble");
	bReturn = CreateGLTexture(
		"textures/granite.jpg", "granite");
	bReturn = CreateGLTexture(
		"textures/gold.jpg", "gold");
	BindGLTextures();
}
/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	SetupSceneLights();
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

		// The back wall
		scaleXYZ = glm::vec3(40.0f, 1.0f, 40.0f);
		positionXYZ = glm::vec3(0.0f, 4.0f, -10.0f);
		SetTransformations(scaleXYZ, -90.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderTexture("wall");
		m_pShaderManager->setIntValue("bUseTexture", true);
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderMaterial("wall");
		m_basicMeshes->DrawPlaneMesh();

		// Desk Surface
		SetTransformations(glm::vec3(20.0f, 1.0f, 20.0f), 0.0f, 0.0f, 0.0f, glm::vec3(0.0f, -0.5f, 0.0f));
		SetShaderMaterial("wall"); //
		SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
		m_basicMeshes->DrawPlaneMesh();

		// Lamp base
		scaleXYZ = glm::vec3(1.5f, 0.2f, 1.5f);
		positionXYZ = glm::vec3(5.0f, 0.0f, 0.0f); // Base position
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderMaterial("granite"); // Using granite so it's not invisible white
		SetShaderColor(0.85f, 0.85f, 0.85f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();

		//Lamp Neck
		scaleXYZ = glm::vec3(0.05f, 4.0f, 0.05f);
		positionXYZ = glm::vec3(6.0f, 0.0f, 0.0f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderMaterial("lamp");
		m_basicMeshes->DrawCylinderMesh();

		// Lamp Shade
		scaleXYZ = glm::vec3(1.2f, 1.5f, 1.2f);
		positionXYZ = glm::vec3(5.5f, 3.8f, 0.0f);
		SetTransformations(scaleXYZ, -45.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderMaterial("lamp");
		m_basicMeshes->DrawTaperedCylinderMesh();

		// Lamp Bulb
		scaleXYZ = glm::vec3(0.2f, 0.2f, 0.2f);
		positionXYZ = glm::vec3(5.5f, 3.6f, 0.0f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderColor(1.0f, 1.0f, 0.0f, 1.0f); // Bright Yellow
		m_pShaderManager->setIntValue("bUseTexture", false);
		m_basicMeshes->DrawSphereMesh();

		//Lamp Joint
		scaleXYZ = glm::vec3(0.15f, 0.3f, 0.15f);
		positionXYZ = glm::vec3(6.0f, 4.0f, -0.2f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 90.0f, positionXYZ);
		SetShaderMaterial("gold"); //
		m_basicMeshes->DrawCylinderMesh();

		//Clock 
		scaleXYZ = glm::vec3(1.6f, 0.05f, 1.6f);
		positionXYZ = glm::vec3(-2.0f, 7.0f, -4.95f); 
		SetTransformations(scaleXYZ, 90.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderMaterial("marble");
		m_pShaderManager->setIntValue("bUseTexture", false);
		SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();
		//Clock Face
		scaleXYZ = glm::vec3(1.5f, 0.1f, 1.5f);
		positionXYZ = glm::vec3(-2.0f, 7.0f, -4.9f); 
		SetTransformations(scaleXYZ, 90.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderTexture("wood");
		m_pShaderManager->setIntValue("bUseTexture", true);
		m_basicMeshes->DrawCylinderMesh();

		// Pot
		scaleXYZ = glm::vec3(1.2f, 1.0f, 1.2f);
		positionXYZ = glm::vec3(2.0f, 0.5f, 0.0f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderMaterial("granite"); //
		SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);
		m_basicMeshes->DrawSphereMesh();

		// Leaves
		m_pShaderManager->setIntValue("bUseTexture", true);
		SetShaderTexture("leaf");

		int leafCount = 10;
		for (int i = 0; i < leafCount; i++) {
			float randomHeight = 1.5f + (static_cast<float>(i % 3) * 0.2f);
			scaleXYZ = glm::vec3(0.12f, randomHeight, 0.4f);

			positionXYZ = glm::vec3(2.0f, 1.3f, 0.0f);

			float yRotation = i * (360.0f / leafCount);
			float xTilt = 20.0f + (i * 3.0f);
			float zLean = (i % 2 == 0) ? 5.0f : -5.0f;

			SetTransformations(scaleXYZ, xTilt, yRotation, zLean, positionXYZ);
			m_basicMeshes->DrawTaperedCylinderMesh();
		}
	}