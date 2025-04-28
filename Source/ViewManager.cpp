///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with
	// the 3D scene
	Camera* g_pCamera = nullptr;

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// time between current frame and last frame
	float gDeltaTime = 0.0f;
	float gLastFrame = 0.0f;

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false;
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager* pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();

	// Referenece view. Elevated, pulled back, and a very slight tilt.
	g_pCamera->Position = glm::vec3(0.0f, 8.0f, 12.0f);
	g_pCamera->Front = glm::vec3(-0.1f, -1.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	// free up allocated memory
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// tell GLFW to capture all mouse events
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// this callback is used to receive mouse moving events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);

	// this callback uses the scrollwheel to adjust the speed of the camera
	glfwSetScrollCallback(window, &ViewManager::MouseScrollCallback);

	// enable blending for supporting tranparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	// Record the first mouse pos so subsequent mouse moves
	// can be calculated correctly using the x and y offsets.
	if (gFirstMouse)
	{
		gLastX = xMousePos;
		gLastY = yMousePos;
		gFirstMouse = false;
	}

	// Calculate the x and y offsets
	float xOffset = xMousePos - gLastX;
	float yOffset = gLastY - yMousePos; // Reverse y coords because the axis goes from bottom to top

	// Set the current positions into the last position vars
	gLastX = xMousePos;
	gLastY = yMousePos;

	// Move the camera according to the offsets
	g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

/***********************************************************
 *  MouseScrollCallback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse wheel is scrolled within the active GLFW window.
 ***********************************************************/
void ViewManager::MouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
	if (g_pCamera)
	{
		// Adjust thee speed based on the scroll wheel
		g_pCamera->MovementSpeed += yOffset * 0.1f;

		// Set a floor for the speed
		if (g_pCamera->MovementSpeed < 1.0f)
		{
			g_pCamera->MovementSpeed = 1.0f;
		}

		// Set a speed limit to keep it controllable
		if (g_pCamera->MovementSpeed > 50.0f)
		{
			g_pCamera->MovementSpeed = 50.0f;
		}
	}
}

/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	// If the camera is null, return early
	if (g_pCamera == NULL)
	{
		return;
	}

	// Forward and backward movement
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
	}

	// Side to side movement, left/right
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
	}

	// Up and down movement
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(UP, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(DOWN, gDeltaTime);
	}

	// Enable perspective projection
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
	{
		// reset the camera pos
		g_pCamera->Position = glm::vec3(0.0f, 8.0f, 12.0f);
		g_pCamera->Front = glm::vec3(-0.1f, -1.5f, -2.0f);
		g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
		g_pCamera->Zoom = 80;

		bOrthographicProjection = false;
	}
	// Enable orthographic and cycle views
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS && !m_oKeyPressed)
	{
		m_oKeyPressed = true; // Prevent accidental switching

		// If ortho is engaged, cycle
		if (bOrthographicProjection)
		{
			switch (m_currentOrthoView)
			{
			case ORTHO_FRONT:
				// Switch to side view
				g_pCamera->Position = glm::vec3(15.0f, 8.0f, 0.0f);
				g_pCamera->Front = glm::vec3(-1.0f, 0.0f, 0.0f);
				g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);

				// Set the camera state
				m_currentOrthoView = ORTHO_SIDE;
				break;

			case ORTHO_SIDE:
				// Switch to top view
				g_pCamera->Position = glm::vec3(0.0f, 10.0f, 0.0f);
				g_pCamera->Front = glm::vec3(0.0f, -1.0f, 0.0f);
				g_pCamera->Up = glm::vec3(0.0f, 0.0f, -1.0f);

				// Set the camera state
				m_currentOrthoView = ORTHO_TOP;
				break;

			case ORTHO_TOP:
				// Switch to front position
				g_pCamera->Position = glm::vec3(1.0f, 8.0f, 10.0f);
				g_pCamera->Front = glm::vec3(0.0f, 0.0f, -1.0f);
				g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);

				// Set the camera state
				m_currentOrthoView = ORTHO_FRONT;
				break;

				// Unknown, break
			default:
				break;
			}
		}
		// If ortho is not engaged, engage ortho
		else
		{
			// Switch to front position
			g_pCamera->Position = glm::vec3(1.0f, 8.0f, 10.0f);
			g_pCamera->Front = glm::vec3(0.0f, 0.0f, -1.0f);
			g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);

			// Set the camera state
			m_currentOrthoView = ORTHO_FRONT;

			// set orthographic projection to true
			bOrthographicProjection = true;
		}
	}
	// On O key release, reset state
	else if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_RELEASE) {
		m_oKeyPressed = false; // Reset the state when released
	}
}

/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the 
	// event queue
	ProcessKeyboardEvents();

	// get the current view matrix from the camera
	view = g_pCamera->GetViewMatrix();

	// If ortho is not enabled, set a perspective projection
	if (!bOrthographicProjection)
	{
		// define the current projection matrix
		projection = glm::perspective(glm::radians(g_pCamera->Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}
	// Else, use an orthographic projection
	else
	{
		projection = glm::ortho(
			-10.0f,
			10.0f,
			-10.0f,
			10.0f,
			0.1f,
			100.0f
		);
	}
	// if the shader manager object is valid
	if (NULL != m_pShaderManager)
	{
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ViewName, view);
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		// set the view position of the camera into the shader for proper rendering
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);

		// Update the spotlight position to follow the camera
		m_pShaderManager->setVec3Value("spotLight.position", g_pCamera->Position);
		m_pShaderManager->setVec3Value("spotLight.direction", g_pCamera->Front);
	}
}
