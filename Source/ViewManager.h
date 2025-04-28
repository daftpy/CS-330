///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ShaderManager.h"
#include "camera.h"

// GLFW library
#include "GLFW/glfw3.h" 

class ViewManager
{
public:
	// constructor
	ViewManager(
		ShaderManager* pShaderManager);
	// destructor
	~ViewManager();

	// mouse position callback for mouse interaction with the 3D scene
	static void Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos);

	// scroll wheel callback to adjust the speed of camera movement
	static void MouseScrollCallback(GLFWwindow* window, double xYoffset, double yOffset);

	// A call back to help switch between ortho view positions
	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

private:
	// pointer to shader manager object
	ShaderManager* m_pShaderManager;
	// active OpenGL display window
	GLFWwindow* m_pWindow;

	// process keyboard events for interaction with the 3D scene
	void ProcessKeyboardEvents();

	// An enumeration for the different ortho view states
	enum OrthoView { ORTHO_FRONT = 0, ORTHO_SIDE, ORTHO_TOP };
	OrthoView m_currentOrthoView = ORTHO_FRONT; // Ortho view state (start front)

	// Holds the pressed state for the 'O' key
	bool m_oKeyPressed{ false };

public:
	// create the initial OpenGL display window
	GLFWwindow* CreateDisplayWindow(const char* windowTitle);

	// prepare the conversion from 3D object display to 2D scene display
	void PrepareSceneView();
};