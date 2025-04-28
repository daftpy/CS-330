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
 *  LoadSceneTextures()
 *
 *  This methods loads the texture files from the assets
 *  folder into memory for rendering in the 3d scene
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	// Load the texture for the desk that supports the scene
	CreateGLTexture("assets/desk_texture.jpg", "desk");

	// Load the texture for the metal cork stopper
	CreateGLTexture("assets/cork_stopper.jpg", "cork_stopper");

	// Load the texture for the black rubber
	CreateGLTexture("assets/black_rubber.jpg", "rubber");

	// Load the book fabric texture for the book covers
	CreateGLTexture("assets/book_fabric.jpg", "book_fabric");

	// Load the paper texture for the book pages
	CreateGLTexture("assets/paper.jpg", "paper");

	// Load the wood chest texture
	CreateGLTexture("assets/chest.jpg", "chest");

	// Load the leather texture for the traps on the chest
	CreateGLTexture("assets/leather.jpg", "leather");

	// Load the wood chest texture for the top of the chest
	CreateGLTexture("assets/chest_top.jpg", "chest_top");

	// Load the leather texture with the seam
	CreateGLTexture("assets/leather_seam.jpg", "leather_seam");

	// Load the dark metal texture for the lock
	CreateGLTexture("assets/metal_dark.jpg", "metal_dark");

	// Load the texture for the metal mug
	CreateGLTexture("assets/metal_mug.jpg", "metal_mug");

	// Load the texture for the metal mug body with logo
	CreateGLTexture("assets/metal_mug_body.jpg", "metal_mug_body");

	// Load the tile texture for the backdrop
	CreateGLTexture("assets/tile.jpg", "tile_wall");

	// After the textures are loaded into memory, bind them
	// to the texture slots
	BindGLTextures();
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
	// Define and load the materials
	DefineObjectMaterials();

	// Define and load the lights
	SetupSceneLights();

	// load the textures for the scene
	LoadSceneTextures();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();

	m_basicMeshes->LoadConeMesh();

	m_basicMeshes->LoadSphereMesh();

	m_basicMeshes->LoadTorusMesh(0.05);

	m_basicMeshes->LoadExtraTorusMesh1();

	m_basicMeshes->LoadExtraTorusMesh2(0.061);

	m_basicMeshes->LoadBoxMesh();

	m_basicMeshes->LoadCylinderMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// Render the platform for the scene to sit upon
	RenderPlatform();

	// Render the cork stopper
	RenderCorkStopper();

	// Render the book
	RenderBook();

	// Render the candle
	RenderCandle();

	// Render the chest
	RenderChest();

	// Render the mug
	RenderMug();
}

/***********************************************************
 *  RenderPlatform()
 *
 *  This method is used for rendering the plane the scene
 *  sits upon. It transforms, adds a texture, and draws the
 *  the plane.
 ***********************************************************/
void SceneManager::RenderPlatform()
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
	scaleXYZ = glm::vec3(35.0f, 1.0f, 12.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply the desk texture
	SetShaderTexture("desk");
	SetShaderMaterial("desk");
	// Tile across the plane to avoid stretching and keep detail
	// 10x across and 4x down
	SetTextureUVScale(10.0f, 4.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// The platform backdrop behiind the platform
	positionXYZ = glm::vec3(0.0, 2.0, -10.0f);

	// Scale similarly to the platform
	scaleXYZ = glm::vec3(35.0f, 1.0f, 12.0f);

	// Rotate it to act as a wall
	XrotationDegrees = 90.0f;

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply the desk texture
	SetShaderTexture("tile_wall");
	SetShaderMaterial("metal");
	// Tile across the plane to avoid stretching and keep detail
	// 10x across and 4x down
	SetTextureUVScale(10.0f, 4.0f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
}

/***********************************************************
 *  RenderCorkStopper()
 *
 *  This method is used for rendering the metal cork stopper
 *  It transforms, textures, and draws two torus' and a cone
 *  to represent the cork stopper.
 ***********************************************************/
void SceneManager::RenderCorkStopper()
{
	// Render torus 1
	// Position vector for torus 1
	glm::vec3 positionXYZ = glm::vec3(2.0f, 0.35f, 0.0f);

	// Create the transform variables
	glm::vec3 scaleXYZ = glm::vec3(0.3, 0.3, 0.3); // Set the scale
	float XrotationDegrees = 90.0f; // Rotate 90 deg to fit over the cone
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply the dark rubber texture
	SetShaderTexture("rubber");

	// Apply the rubber material
	SetShaderMaterial("rubber");

	// Slightly zoom on the texture to balance with the scaled-down torus size
	SetTextureUVScale(0.64f, 0.64f);

	// Draw the mesh
	m_basicMeshes->DrawExtraTorusMesh1();

	// Render torus 2
	positionXYZ = glm::vec3(2.0f, 0.65f, 0.0f); // Set the new position
	scaleXYZ = glm::vec3(0.24f, 0.24f, 0.24f); // Set the new scale
	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees, // Use the same rotation as the previous torus
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply the dark rubber textur
	SetShaderTexture("rubber");

	// Apply the rubber material
	SetShaderMaterial("rubber");

	// Slightly zoom on the texture to balance with the scaled-down torus size
	SetTextureUVScale(0.55f, 0.55f);

	// Draw the mesh
	m_basicMeshes->DrawExtraTorusMesh1();

	// Render cone
	positionXYZ = glm::vec3(2.0f, 0.0f, 0.0f); // Set the new position
	scaleXYZ = glm::vec3(0.4f, 2.0f, 0.4f); // Set the new scale
	XrotationDegrees = 0.0f; // Reset the X axis rotation

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Set the metal cork stopper texture
	SetShaderTexture("cork_stopper");

	// Set the metal material to the cork stopper
	SetShaderMaterial("metal");

	// Tile the texture along the height of the cone to avoid stretching
	// 2.0f works well, 4.0f does not look as good
	SetTextureUVScale(1.0f, 2.0f);

	// Draw the cone
	m_basicMeshes->DrawConeMesh();
}

/***********************************************************
 *  RenderBook()
 *
 *  This method is used for rendering the book. It
 *  transforms, textures, and draws four different shaped
 *  boxes to represents all of the pages and sides of a book.
 ***********************************************************/
void SceneManager::RenderBook()
{
	// Set the books central position
	glm::vec3 bookPos = glm::vec3(7.0f, 0.6f, 0.0f);

	// Rotation applied to the whole book
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = -25.0f;
	float ZrotationDegrees = 0.0f;

	// Declare the reusable variables for transformation
	glm::vec3 scaleXYZ;
	glm::vec3 positionXYZ;

	/*********************** Back Cover ****************************/
	scaleXYZ = glm::vec3(6.0f, 0.2f, 9.0f); // Set the scale of the cover
	positionXYZ = bookPos + glm::vec3(0.0f, -0.5f, 0.0f); // offset down

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Set the book fabric texture
	SetShaderTexture("book_fabric");

	// Set the book fabric material
	SetShaderMaterial("book_fabric");

	// Tile the texture 1x across and 3x down
	SetTextureUVScale(1.0f, 3.0f);

	// Draw the back cover
	m_basicMeshes->DrawBoxMesh();// Set the book fabric texture

	/*********************** Front Cover ***************************/
	scaleXYZ = glm::vec3(6.0f, 0.2f, 9.0f); // Match the scale of the back cover
	positionXYZ = bookPos + glm::vec3(0.0f, 0.5f, 0.0f); // offset up

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Set the book fabric texture
	SetShaderTexture("book_fabric");

	// Set the book fabric material
	SetShaderMaterial("book_fabric");

	// Tile the texture to the same ratio as the shape (1:3)
	// 1x across and 3x down
	SetTextureUVScale(1.0f, 3.0f);

	// Draw the front cover
	m_basicMeshes->DrawBoxMesh();


	/*********************** Binding *******************************/
	// The binding of the book must have its rotation offset in order to
	// rotate accurately with the other pieces of the book

	// Set the scale
	scaleXYZ = glm::vec3(0.2f, 1.2f, 9.0f);

	// Offset in local space before rotation
	glm::vec3 localOffset = glm::vec3(-3.0f, 0.0f, 0.0f);

	// Convert to radians
	float radians = glm::radians(YrotationDegrees);

	// Rotate around y using glm::rotate
	glm::vec3 rotatedOffset = glm::vec3(glm::rotate(radians, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(localOffset, 1.0f));

	// Add to the book pos
	positionXYZ = bookPos + rotatedOffset; // left side of covers

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Set the book fabric texture
	SetShaderTexture("book_fabric");

	// Set the book fabric material
	SetShaderMaterial("book_fabric");

	// Tile the texture to the same ratio as the shape (1:3)
	// Tile the texture 1x across and 3x down
	SetTextureUVScale(1.0f, 3.0f);

	// Draw the binding
	m_basicMeshes->DrawBoxMesh();


	/*********************** Pages *********************************/
	scaleXYZ = glm::vec3(5.8f, 0.8f, 8.8f);
	positionXYZ = bookPos; // centered

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Set the book fabric texture
	SetShaderTexture("paper");

	// Set the paper material
	SetShaderMaterial("paper");

	// Tile the texture to the same ratio as the shape (1:3)
	// Tile the texture 1x across and 3x down
	SetTextureUVScale(1.0f, 3.0f);

	// Draw the pages
	m_basicMeshes->DrawBoxMesh();
}

/***********************************************************
 *  RenderCancdle()
 *
 *  This method is used for rendering the candle. It
 *  transforms, textures, and draws four different shaped
 *  cylinders and tori to represents the jar, threads, and contents.
 ***********************************************************/
void SceneManager::RenderCandle()
{
	// Set a default location for all the pieces to build from
	glm::vec3 candlePos = glm::vec3(7.0f, 0.6f, 0.0f);

	/*********************** Candle Lower Body *********************************/
	// Set the candle position
	glm::vec3 positionXYZ = candlePos + glm::vec3(0.0f, 0.6f, 0.0f); // Position it on top of the book

	// Set the candle scale
	glm::vec3 scaleXYZ = glm::vec3(1.1f, 2.0f, 1.1f);

	// Set the candle to a default rotation
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Set a glass like color and material
	SetShaderColor(1.0f, 0.1f, 0.3f, 1.0f);
	SetShaderMaterial("glass");

	m_basicMeshes->DrawCylinderMesh();

	/*********************** Candle Lower Glass *********************************/
	// Set the candle position
	positionXYZ = candlePos + glm::vec3(0.0f, 0.6f, 0.0f); // Position it on top of the book

	// Set the candle scale
	scaleXYZ = glm::vec3(1.2f, 0.3f, 1.2f);

	// Set the candle to a default rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// A glass color and material
	SetShaderColor(0.3, 0.3, 0.4, 0.6f);
	SetShaderMaterial("glass");

	// Draw the lower candle glass
	m_basicMeshes->DrawCylinderMesh(true, false, true);

	/*********************** Candle Label *********************************/
	// Set the candle position
	positionXYZ = candlePos + glm::vec3(0.0f, 0.9f, 0.0f); // Position it on top of the book

	// Set the candle scale
	scaleXYZ = glm::vec3(1.2f, 1.4f, 1.2f);

	// Set the candle to a default rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Set a paper texture and material to match the candle
	SetShaderTexture("paper");
	SetShaderMaterial("paper");

	// Follow the dimensions of the cylinder
	SetTextureUVScale(1.2f, 1.4f);

	// Draw the label portion
	m_basicMeshes->DrawCylinderMesh(false, false, true);

	/*********************** Candle Wick *********************************/
	// Set the candle position
	positionXYZ = candlePos + glm::vec3(0.0f, 2.6f, 0.0f); // Position it on top of the book

	// Set the candle scale
	scaleXYZ = glm::vec3(0.05f, 0.4f, 0.05f);

	// Set the candle to a default rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply the dark rubber texture to simulate a burned wick
	SetShaderTexture("rubber");

	// Apply a book_fabric material, similar in properties to a wick
	SetShaderMaterial("book_fabric");

	// Default texture uv scale
	SetTextureUVScale(1.0f, 1.0f);

	// Draw the wick
	m_basicMeshes->DrawCylinderMesh(true, false, true);

	/*********************** Candle Upper Glass *********************************/
	// Set the candle position
	positionXYZ = candlePos + glm::vec3(0.0f, 2.3f, 0.0f); // Position it on top of the book

	// Set the candle scale
	scaleXYZ = glm::vec3(1.2f, 0.8f, 1.2f);

	// Set the candle to a default rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Set a glass color and material
	SetShaderColor(0.3, 0.3, 0.4, 0.6f);
	SetShaderMaterial("glass");

	// Draw the uppper glass
	m_basicMeshes->DrawCylinderMesh(true, false, true);

	/*********************** Candle Neck *********************************/
	positionXYZ = candlePos + glm::vec3(0.0f, 3.1f, 0.0f); // Position it on top of the book

	// Set the candle scale
	scaleXYZ = glm::vec3(1.0f, 0.4f, 1.0f);

	// Set the candle to a default rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// A glass color and material
	SetShaderColor(0.3, 0.3, 0.4, 0.6f);
	SetShaderMaterial("glass");

	// Draw the candle neck
	m_basicMeshes->DrawCylinderMesh(false, false, true);

	/*********************** Candle Threads *********************************/
	// Render torus 1
	// Position vector for torus 1
	positionXYZ = candlePos + glm::vec3(0.0f, 3.2f, 0.0f); // Position it around the neck of the candle

	// Create the transform variables
	scaleXYZ = glm::vec3(1.05, 1.05, 0.3); // Set the scale
	XrotationDegrees = 90.0f; // Rotate 90 deg to fit over the neck
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply a glass color and material
	SetShaderColor(0.3, 0.3, 0.4, 0.8f);
	SetShaderMaterial("glass");

	// Draw the mesh
	m_basicMeshes->DrawTorusMesh();

	// Render torus 1
	// Position vector for torus 1
	positionXYZ = candlePos + glm::vec3(0.0f, 3.3f, 0.0f); // Position it on top of the book

	// Create the transform variables
	scaleXYZ = glm::vec3(1.05, 1.05, 0.3); // Set the scale
	XrotationDegrees = 90.0f; // Rotate 90 deg to fit over the cone
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// A glass color and material
	SetShaderColor(0.3, 0.3, 0.4, 0.8f);
	SetShaderMaterial("glass");

	// Draw the threads
	m_basicMeshes->DrawTorusMesh();
}

/***********************************************************
 *  RenderMug()
 *
 *  This method is used for rendering the mug. It
 *  transforms, textures, and draws different shaped cylinders
 *  for the mug body, boxes for the handle, and a torus for the rim.
 ***********************************************************/
void SceneManager::RenderMug()
{
	// The location for all the mug pieces to build from
	glm::vec3 mugPos = glm::vec3(-7.0f, 0.0f, 3.0f);

	// Set the mug to a default rotation
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;

	/*********************** Outter body *********************************/
	glm::vec3 positionXYZ = mugPos;

	YrotationDegrees = 145.0f;

	glm::vec3 scaleXYZ = glm::vec3(1.6f, 3.4f, 1.6f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees, // Use the same rotation as last time
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Set the metal material and mug texture with the logo
	SetShaderTexture("metal_mug_body");
	SetShaderMaterial("metal");
	// Set a default uv scale, no tiling for the logo
	SetTextureUVScale(1.0f, 1.0f);

	// Draw the outter body
	m_basicMeshes->DrawCylinderMesh(false, false, true);

	/*********************** Inner body *********************************/
	positionXYZ = mugPos + glm::vec3(0.0f, 0.4f, 0.0f); // Position above the mug base

	// Set the scale to fit inside the body of the mug
	scaleXYZ = glm::vec3(1.4f, 3.0f, 1.4f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Set the metal material and mug texture
	SetShaderTexture("metal_mug");
	SetShaderMaterial("metal");
	// Set a default uv scale
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawCylinderMesh(false, true, true);

	/*********************** Rim Body *********************************/
	positionXYZ = mugPos + glm::vec3(0.0f, 3.4f, 0.0f);

	scaleXYZ = glm::vec3(1.6f, 0.2f, 1.6f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Use the metal texture for the rim
	SetShaderTexture("cork_stopper");
	SetShaderMaterial("metal");
	// Set a default uv scale
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawCylinderMesh(false, false, true);

	/*********************** Rim top *********************************/
	positionXYZ = mugPos + glm::vec3(0.0f, 3.6f, 0.0f);


	// Create the transform variables
	scaleXYZ = glm::vec3(1.5, 1.5, 1.1); // Set the scale
	XrotationDegrees = 90.0f; // Rotate 90 deg to fit over the cone
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply a metal material and texture
	SetShaderTexture("cork_stopper");
	SetShaderMaterial("metal");

	// Use a default uv scale
	SetTextureUVScale(1.0f, 1.0f);

	// Draw the mesh
	m_basicMeshes->DrawExtraTorusMesh2();

	/*********************** Handle top *********************************/
	positionXYZ = mugPos + glm::vec3(-1.5f, 2.9f, 1.5f);

	// Create the transform variables
	scaleXYZ = glm::vec3(0.5, 0.1, 1.3); // Set the scale
	XrotationDegrees = 0.0f; // Rotate parallel to floor
	YrotationDegrees = -45.0f;
	ZrotationDegrees = 0.0f;

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply a metal material and mug texture
	SetShaderTexture("metal_mug");
	SetShaderMaterial("metal");

	// Use a default uv scale
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	/*********************** Handle bottom *********************************/
	positionXYZ = mugPos + glm::vec3(-1.5f, 0.8f, 1.5f);

	// Create the transform variables
	scaleXYZ = glm::vec3(0.5, 0.1, 1.3); // Set the scale
	XrotationDegrees = 0.0f;
	YrotationDegrees = -45.0f;
	ZrotationDegrees = 0.0f;

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply the metal material and mug texture
	SetShaderTexture("metal_mug");
	SetShaderMaterial("metal");

	// Use a default uv scale
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	/*********************** Handle side *********************************/
	positionXYZ = mugPos + glm::vec3(-1.925f, 1.85f, 1.925f);

	// Create the transform variables
	scaleXYZ = glm::vec3(0.5f, 2.0f, 0.1f); // Set the scale
	XrotationDegrees = 0.0f; // Rotate parallel to floor
	YrotationDegrees = -45.0f;
	ZrotationDegrees = 0.0f;

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply the metal material and mug texture
	SetShaderTexture("metal_mug");
	SetShaderMaterial("metal");

	// Set a default uv scale
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();
}

/***********************************************************
 *  RenderChest()
 *
 *  This method is used for rendering the Chest. It
 *  transforms, textures, and draws different shaped boxes
 *  to represent the top and bottom of the chest, as well as
 *  the decorative leather straps, and metal latch.
 ***********************************************************/
void SceneManager::RenderChest()
{
	// Set a base chest position
	glm::vec3 chestPos = glm::vec3(-3.0f, 0.0f, -2.0f);

	// Set a base rotation for the chest
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 15.0f;
	float ZrotationDegrees = 0.0f;

	/*********************** Bottom chest *********************************/
	glm::vec3 positionXYZ = chestPos + glm::vec3(0.0f, 1.75f, 0.0f); // Offset up

	glm::vec3 scaleXYZ = glm::vec3(9.0f, 3.5f, 3.5f); // Set the scale

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply the chest texture
	SetShaderTexture("chest");
	SetShaderMaterial("desk");

	// Tile the texture along the x axis
	SetTextureUVScale(4.5f, 1.0f);

	// Draw the bottom of the chest
	m_basicMeshes->DrawBoxMesh();

	/*********************** Top chest *********************************/
	positionXYZ = chestPos + glm::vec3(0.0f, 4.5f, 0.0f); // Offset uup

	scaleXYZ = glm::vec3(9.0f, 2.0f, 3.5f); // Set the scale

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Below the top section of the chest has each face textured individually.
	// This will allow a texture with a seam to emphasize the two seperate sections
	// of the chest

	// Apply the chest texture with the seam
	SetShaderTexture("chest_top");
	SetShaderMaterial("desk"); // Wood material
	SetTextureUVScale(2.0f, 1.0f); // Tile along the x axis
	// Draw the left of the chest top
	m_basicMeshes->DrawBoxMeshSide(m_basicMeshes->box_left);

	// Apply the chest texture with the seam
	SetShaderTexture("chest_top");
	SetShaderMaterial("desk"); // Wood material
	SetTextureUVScale(2.0f, 1.0f); // Scale to tile along the x axis
	// Draw the right of the chest top
	m_basicMeshes->DrawBoxMeshSide(m_basicMeshes->box_right);

	// Apply the chest texture with the seam
	SetShaderTexture("chest_top");
	SetShaderMaterial("desk"); // Wooden material
	SetTextureUVScale(4.5f, 1.0f); // Tile along the x to follow the shape
	// Draw the front of the chest top
	m_basicMeshes->DrawBoxMeshSide(m_basicMeshes->box_front);

	// Apply the chest texture with the seam
	SetShaderTexture("chest_top");
	SetShaderMaterial("desk"); // Wooden texture
	SetTextureUVScale(4.5f, 1.0f); // Tile along the x axis
	// Draw the back of the chest top
	m_basicMeshes->DrawBoxMeshSide(m_basicMeshes->box_back);

	// Apply the texture without the seam. The top has no seam
	SetShaderTexture("chest");
	SetShaderMaterial("desk"); // Wooden texture
	SetTextureUVScale(4.5f, 1.0f); // Tile along the x
	// Draw the top of the chest top.
	m_basicMeshes->DrawBoxMeshSide(m_basicMeshes->box_top);

	/*********************** Front strip 1 - bottom *********************************/

	// Offset in local space before rotation
	glm::vec3 localOffset = glm::vec3(-2.0f, 1.75f, 1.8f); // Offset up, to the left, and backward

	// Convert to radians
	float radians = glm::radians(YrotationDegrees);

	// Rotate around the y using glm::rotate
	glm::vec3 rotatedOffset = glm::vec3(glm::rotate(radians, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(localOffset, 1.0f));

	positionXYZ = chestPos + rotatedOffset; // Get the offset position

	// A long thin strip
	scaleXYZ = glm::vec3(0.75f, 3.5f, 0.1f); // Set the scale

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply the leather texture to the strip
	SetShaderTexture("leather");
	SetShaderMaterial("leather");

	// Tile along the y and zoom on the x axis
	SetTextureUVScale(0.75f, 5.5f);

	// Draw the strip
	m_basicMeshes->DrawBoxMesh();

	/*********************** Front strip 1 - top *********************************/
	// Offset in local space before rotation
	localOffset = glm::vec3(-2.0f, 4.5f, 1.8f); // Offset to the left, up and forward

	// Rotate around the y using glm::rotate
	rotatedOffset = glm::vec3(glm::rotate(radians, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(localOffset, 1.0f));

	positionXYZ = chestPos + rotatedOffset; // Get the offset position

	// A long thin strip
	scaleXYZ = glm::vec3(0.75f, 2.0f, 0.1f); // Set the scale

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);


	// Apply the chest texture with the seam
	SetShaderTexture("leather_seam");
	SetShaderMaterial("leather"); // Wood material
	SetTextureUVScale(2.0f, 1.0f); // Tile along the x axis
	// Draw the left of the chest top
	m_basicMeshes->DrawBoxMeshSide(m_basicMeshes->box_left);

	// Apply the chest texture with the seam
	SetShaderTexture("leather_seam");
	SetShaderMaterial("leather"); // Wood material
	SetTextureUVScale(2.0f, 1.0f); // Scale to tile along the x axis
	// Draw the right of the chest top
	m_basicMeshes->DrawBoxMeshSide(m_basicMeshes->box_right);

	// Apply the chest texture with the seam
	SetShaderTexture("leather_seam");
	SetShaderMaterial("leather"); // Wooden material
	SetTextureUVScale(4.5f, 1.0f); // Tile along the x to follow the shape
	// Draw the front of the chest top
	m_basicMeshes->DrawBoxMeshSide(m_basicMeshes->box_front);

	// Apply the chest texture with the seam
	SetShaderTexture("leather");
	SetShaderMaterial("leather"); // Wooden texture
	SetTextureUVScale(4.5f, 1.0f); // Tile along the x axis
	// Draw the back of the chest top
	m_basicMeshes->DrawBoxMeshSide(m_basicMeshes->box_back);

	// Apply the texture without the seam. The top has no seam
	SetShaderTexture("leather");
	SetShaderMaterial("leather"); // Wooden texture
	SetTextureUVScale(4.5f, 1.0f); // Tile along the x
	// Draw the top of the chest top.
	m_basicMeshes->DrawBoxMeshSide(m_basicMeshes->box_top);

	/*********************** Front strip 2 - bottom *********************************/

	// Offset in local space before rotation
	localOffset = glm::vec3(2.0f, 1.75f, 1.8f); // Offset right, up, and forward

	// Rotate around the y using glm::rotate
	rotatedOffset = glm::vec3(glm::rotate(radians, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(localOffset, 1.0f));

	positionXYZ = chestPos + rotatedOffset; // Set the offset position

	// A long thin strip
	scaleXYZ = glm::vec3(0.75f, 3.5f, 0.1f); // Set the scale

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply the leather texture to the strip
	SetShaderTexture("leather");
	SetShaderMaterial("leather");

	// Tile along the y and zoom on the x axis
	SetTextureUVScale(0.75f, 5.5f);

	// Draw the strip
	m_basicMeshes->DrawBoxMesh();

	/*********************** Front strip 2 - top *********************************/
	// Offset in local space before rotation
	localOffset = glm::vec3(2.0f, 4.5f, 1.8f);

	// Rotate around the y using glm::rotate
	rotatedOffset = glm::vec3(glm::rotate(radians, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(localOffset, 1.0f));

	positionXYZ = chestPos + rotatedOffset; // Set the offset position

	// A long thin strip
	scaleXYZ = glm::vec3(0.75f, 2.0f, 0.1f); // Set the scale

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply the chest texture with the seam
	SetShaderTexture("leather_seam");
	SetShaderMaterial("leather"); // Wood material
	SetTextureUVScale(2.0f, 1.0f); // Tile along the x axis
	// Draw the left of the chest top
	m_basicMeshes->DrawBoxMeshSide(m_basicMeshes->box_left);

	// Apply the chest texture with the seam
	SetShaderTexture("leather_seam");
	SetShaderMaterial("leather"); // Wood material
	SetTextureUVScale(2.0f, 1.0f); // Scale to tile along the x axis
	// Draw the right of the chest top
	m_basicMeshes->DrawBoxMeshSide(m_basicMeshes->box_right);

	// Apply the chest texture with the seam
	SetShaderTexture("leather_seam");
	SetShaderMaterial("leather"); // Wooden material
	SetTextureUVScale(4.5f, 1.0f); // Tile along the x to follow the shape
	// Draw the front of the chest top
	m_basicMeshes->DrawBoxMeshSide(m_basicMeshes->box_front);

	// Apply the chest texture with the seam
	SetShaderTexture("leather");
	SetShaderMaterial("leather"); // Wooden texture
	SetTextureUVScale(4.5f, 1.0f); // Tile along the x axis
	// Draw the back of the chest top
	m_basicMeshes->DrawBoxMeshSide(m_basicMeshes->box_back);

	// Apply the texture without the seam. The top has no seam
	SetShaderTexture("leather");
	SetShaderMaterial("leather"); // Wooden texture
	SetTextureUVScale(4.5f, 1.0f); // Tile along the x
	// Draw the top of the chest top.
	m_basicMeshes->DrawBoxMeshSide(m_basicMeshes->box_top);

	/*********************** Top strip 1 *********************************/
	localOffset = glm::vec3(2.0f, 5.55f, 0.05f); // Position above the chest

	// Rotate around the y using glm::rotate
	rotatedOffset = glm::vec3(glm::rotate(radians, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(localOffset, 1.0f));

	positionXYZ = chestPos + rotatedOffset; // Set the offset position

	// A long thin strip
	scaleXYZ = glm::vec3(0.75f, 0.1f, 3.6f); // Set the scale

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply the leather texture to the strip
	SetShaderTexture("leather");
	SetShaderMaterial("leather");

	// Tile along the y and zoom on the x axis
	SetTextureUVScale(0.75f, 3.6f);

	// Draw the top thin strip
	m_basicMeshes->DrawBoxMesh();

	/*********************** Top strip 2 *********************************/
	localOffset = glm::vec3(-2.0f, 5.55f, 0.05f); // Position above the chest

	// Rotate around the y using glm::rotate
	rotatedOffset = glm::vec3(glm::rotate(radians, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(localOffset, 1.0f));

	positionXYZ = chestPos + rotatedOffset;

	// A long thin strip
	scaleXYZ = glm::vec3(0.75f, 0.1f, 3.6f); // Set the scale

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply the leather texture to the strip
	SetShaderTexture("leather");
	SetShaderMaterial("leather");

	// Tile along the y and zoom on the x axis
	SetTextureUVScale(0.75f, 3.6f);

	// Draw the top thin strip
	m_basicMeshes->DrawBoxMesh();

	/*********************** Top Metal plate *********************************/
	localOffset = glm::vec3(0.0f, 3.75f, 1.8f);

	// Rotate around the y using glm::rotate
	rotatedOffset = glm::vec3(glm::rotate(radians, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(localOffset, 1.0f));

	positionXYZ = chestPos + rotatedOffset; // Set the offset position

	// A small locker, wider than tall
	scaleXYZ = glm::vec3(1.75f, 0.3f, 0.1f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply the metal material and texture
	SetShaderTexture("metal_dark");
	SetShaderMaterial("metal");

	// Scale the texture along the shape
	SetTextureUVScale(1.75f, 0.3f);

	m_basicMeshes->DrawBoxMesh();

	/*********************** Bottom Metal plate *********************************/
	localOffset = glm::vec3(0.0f, 3.25f, 1.8f);

	// Rotate around the y using glm::rotate
	rotatedOffset = glm::vec3(glm::rotate(radians, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(localOffset, 1.0f));

	positionXYZ = chestPos + rotatedOffset; // Set the offset position

	// A small locker, wider than tall
	scaleXYZ = glm::vec3(1.75f, 0.3f, 0.1f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply the metal material and texture
	SetShaderTexture("metal_dark");
	SetShaderMaterial("metal");

	// Scale the texture along the shape
	SetTextureUVScale(1.75f, 0.3f);

	// Draw the bottom metal plate
	m_basicMeshes->DrawBoxMesh();

	/*********************** Lock - part 1 *********************************/
	localOffset = glm::vec3(0.0f, 3.75f, 1.9f);

	// Rotate around the y using glm::rotate
	rotatedOffset = glm::vec3(glm::rotate(radians, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(localOffset, 1.0f));

	positionXYZ = chestPos + rotatedOffset; // Combine the chair pos and rotated offset

	// A small locker, wider than tall
	scaleXYZ = glm::vec3(0.5f, 0.1f, 0.1f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply the metal material and texture
	SetShaderTexture("cork_stopper");
	SetShaderMaterial("metal");

	// Set a default texture scale
	SetTextureUVScale(1.0f, 1.0f);

	// Draw the first part of the lock
	m_basicMeshes->DrawBoxMesh();

	/*********************** Lock - part 2 *********************************/
	localOffset = glm::vec3(0.125f, 3.25f, 1.9f);

	// Rotate around the y using glm::rotate
	rotatedOffset = glm::vec3(glm::rotate(radians, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(localOffset, 1.0f));

	positionXYZ = chestPos + rotatedOffset; // Combine the chir pos and rotated offset

	// A small locker, wider than tall
	scaleXYZ = glm::vec3(0.25f, 0.1f, 0.1f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply the metal material and texture
	SetShaderTexture("cork_stopper");
	SetShaderMaterial("metal");

	// Set a default texture scale
	SetTextureUVScale(1.0f, 1.0f);

	// Draw the second part of the lock
	m_basicMeshes->DrawBoxMesh();

	/*********************** Lock - part 3 *********************************/
	localOffset = glm::vec3(0.2f, 3.5f, 1.9f);

	// Rotate around the y using glm::rotate
	rotatedOffset = glm::vec3(glm::rotate(radians, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(localOffset, 1.0f));

	positionXYZ = chestPos + rotatedOffset;

	// A small locker, wider than tall
	scaleXYZ = glm::vec3(0.1f, 0.4f, 0.1f);

	// Apply the transformations
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Apply the metal material and texture
	SetShaderTexture("cork_stopper");
	SetShaderMaterial("metal");

	// Set a default texture scale
	SetTextureUVScale(1.0f, 1.0f);

	// Draw the latch part of the lock
	m_basicMeshes->DrawBoxMesh();
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Tell the shaders to render the 3D scene with custom lighting
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Point light: warm main light
	m_pShaderManager->setVec3Value("pointLights[0].position", glm::vec3(5.0f, 12.0f, 8.0f)); // Positioned in front and above the scene
	m_pShaderManager->setVec3Value("pointLights[0].ambient", glm::vec3(0.28f, 0.25f, 0.26f)); // Warm ambient color
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", glm::vec3(0.35f)); // Mild diffuse strength
	m_pShaderManager->setVec3Value("pointLights[0].specular", glm::vec3(0.3f)); // Very mild specular highlight
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true); // Activate point light

	// Directional light: soft side fill
	m_pShaderManager->setVec3Value("directionalLight.direction", glm::vec3(-0.3f, -1.0f, -0.4f)); // Light coming in diagnolly
	m_pShaderManager->setVec3Value("directionalLight.ambient", glm::vec3(0.20f, 0.18f, 0.17f)); // Dim ambient fill
	m_pShaderManager->setVec3Value("directionalLight.diffuse", glm::vec3(0.2f, 0.2f, 0.2f)); // Soft diffuse light
	m_pShaderManager->setVec3Value("directionalLight.specular", glm::vec3(0.4f)); // Moderate specular strength
	m_pShaderManager->setBoolValue("directionalLight.bActive", true); // Activate directional light
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// The material for the desk/plane, which should reflect the light
	OBJECT_MATERIAL deskSurfaceMat;
	deskSurfaceMat.tag = "desk";
	deskSurfaceMat.ambientColor = glm::vec3(0.25f); // A slight ambient color
	deskSurfaceMat.ambientStrength = 0.15f; // A mild ambient strength to have the wood stand out
	deskSurfaceMat.diffuseColor = glm::vec3(0.55f); // Moderate diffuse reflection for natural wood lighting
	deskSurfaceMat.specularColor = glm::vec3(0.1f); // Low specular to avoid glossy highlights
	deskSurfaceMat.shininess = 0.05f; // Matte finish, less shiny

	// Add the desk material
	m_objectMaterials.push_back(deskSurfaceMat);

	// The material for the rubber rings on the cork stopper
	OBJECT_MATERIAL rubberMat;
	rubberMat.tag = "rubber";
	rubberMat.ambientColor = glm::vec3(0.05f); // Nearly black base color
	rubberMat.ambientStrength = 0.2f; // Little ambient light
	rubberMat.diffuseColor = glm::vec3(0.1f); // Very low surface color under light
	rubberMat.specularColor = glm::vec3(0.05f); // Very low to no reflections
	rubberMat.shininess = 0.02f; // Extremely matte finish

	// Add the rubber material
	m_objectMaterials.push_back(rubberMat);

	// The material for the metal of the cork stopper
	OBJECT_MATERIAL metalMat;
	metalMat.tag = "metal";
	metalMat.ambientColor = glm::vec3(0.2f); // Cool base color
	metalMat.ambientStrength = 0.1f; // Low ambient light
	metalMat.diffuseColor = glm::vec3(0.4f); // Medium diffuse
	metalMat.specularColor = glm::vec3(0.9f); // Strong reflections
	metalMat.shininess = 64.0f; // Sharp highlights

	// Add the metal mat
	m_objectMaterials.push_back(metalMat);

	// The material for the cloth book cover
	OBJECT_MATERIAL bookFabricMat;
	bookFabricMat.tag = "book_fabric";
	bookFabricMat.ambientColor = glm::vec3(0.15f); // Mild ambient tone
	bookFabricMat.ambientStrength = 0.2f; // Low ambient strength
	bookFabricMat.diffuseColor = glm::vec3(0.3f); // Mild diffusion
	bookFabricMat.specularColor = glm::vec3(0.05f); // Low reflection for cloth
	bookFabricMat.shininess = 0.1f; // Cloth is not shiny

	// Add the book fabric material
	m_objectMaterials.push_back(bookFabricMat);

	// The material for the paper pages of the book
	OBJECT_MATERIAL paperMat;
	paperMat.tag = "paper";
	paperMat.ambientColor = glm::vec3(0.2f);
	paperMat.ambientStrength = 0.3f; // Catches slightly more ambient light than the fabric
	paperMat.diffuseColor = glm::vec3(0.8f); // Brighter under direct light
	paperMat.specularColor = glm::vec3(0.1f); // Paper does not reflect much
	paperMat.shininess = 0.05f; // Paper is not very shiny

	// Add the paper mat
	m_objectMaterials.push_back(paperMat);

	// The material for glass in the scene
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f); // Meedium ambient color
	glassMaterial.ambientStrength = 0.3f; // Mild amient strength
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f); // Mild diffuse color
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f); // Medium specular color for reflections
	glassMaterial.shininess = 85.0; // Very shiny
	glassMaterial.tag = "glass";

	// Add the glass mat
	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL leatherMat;
	leatherMat.tag = "leather";
	leatherMat.ambientColor = glm::vec3(0.22f, 0.14f, 0.10f); // Warm brown base tone
	leatherMat.ambientStrength = 0.25f; // Leather picks up a bit more ambient
	leatherMat.diffuseColor = glm::vec3(0.45f, 0.28f, 0.18f); // Brown diffuse color
	leatherMat.specularColor = glm::vec3(0.08f, 0.05f, 0.04f); // Mostly matte, but catch some light
	leatherMat.shininess = 1.0f; // Broad but subdued highlight

	// Add the leather mat
	m_objectMaterials.push_back(leatherMat);

	OBJECT_MATERIAL tileMat;
	tileMat.tag = "tile";
	tileMat.ambientColor = glm::vec3(0.25f, 0.25f, 0.45f); // Homey soft blue
	tileMat.ambientStrength = 0.25; // Mild ambient pick up 
	tileMat.diffuseColor = glm::vec3(0.4f, 0.5f, 0.6f); // Richer under diirecct light
	tileMat.specularColor = glm::vec3(0.1f, 0.15f, 0.2f);
	tileMat.shininess = 6.0f; // Soft highlights, but does shine

	// Add the tile mat
	m_objectMaterials.push_back(tileMat);
}