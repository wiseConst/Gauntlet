#pragma once

// Everytime you want to build something with Legion, include this header.

// CORE
#include <Eclipse/Core/Core.h>
#include <Eclipse/Core/Application.h>
#include <Eclipse/Core/Log.h>
#include <Eclipse/Layer/Layer.h>
#include <Eclipse/Core/Timestep.h>

// INPUT
#include <Eclipse/Core/Input.h>
#include <Eclipse/Core/InputCodes.h>

// EVENT SYSTEM
#include <Eclipse/Event/Event.h>
#include <Eclipse/Event/KeyEvent.h>
#include <Eclipse/Event/MouseEvent.h>
#include <Eclipse/Event/WindowEvent.h>

// RENDERER
#include <Eclipse/Renderer/Renderer.h>
#include <Eclipse/Renderer/Renderer2D.h>
#include <Eclipse/Renderer/GraphicsContext.h>
#include <Eclipse/Renderer/Texture.h>
#include <Eclipse/Renderer/Buffer.h>
#include <Eclipse/Renderer/Camera/Camera.h>
#include <Eclipse/Renderer/Camera/OrthographicCamera.h>
#include <Eclipse/Renderer/Camera/PerspectiveCamera.h>

// UI
#include <Eclipse/ImGui/ImGuiLayer.h>

// Don't forget to #include <Legion/Core/Entrypoint.h> after this include.
