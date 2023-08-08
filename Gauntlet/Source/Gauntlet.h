#pragma once

// Everytime you want to build something with Legion, include this header.

// CORE
#include <Gauntlet/Core/Core.h>
#include <Gauntlet/Core/Application.h>
#include <Gauntlet/Core/Log.h>
#include <Gauntlet/Layer/Layer.h>
#include <Gauntlet/Core/Timestep.h>

// INPUT
#include <Gauntlet/Core/Input.h>
#include <Gauntlet/Core/InputCodes.h>

// EVENT SYSTEM
#include <Gauntlet/Event/Event.h>
#include <Gauntlet/Event/KeyEvent.h>
#include <Gauntlet/Event/MouseEvent.h>
#include <Gauntlet/Event/WindowEvent.h>

// RENDERER
#include <Gauntlet/Renderer/Renderer.h>
#include <Gauntlet/Renderer/Renderer2D.h>
#include <Gauntlet/Renderer/GraphicsContext.h>
#include <Gauntlet/Renderer/Texture.h>
#include <Gauntlet/Renderer/Buffer.h>
#include <Gauntlet/Renderer/Camera/Camera.h>
#include <Gauntlet/Renderer/Camera/OrthographicCamera.h>
#include <Gauntlet/Renderer/Camera/PerspectiveCamera.h>
#include <Gauntlet/Renderer/Framebuffer.h>
#include <Gauntlet/Renderer/Mesh.h>

// UI
#include <Gauntlet/ImGui/ImGuiLayer.h>

// Don't forget to #include <Legion/Core/Entrypoint.h> after this include.