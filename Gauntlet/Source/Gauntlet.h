#pragma once

// Everytime you want to build something with Gauntlet, include this header.

// CORE
#include <Gauntlet/Core/Core.h>
#include <Gauntlet/Core/Application.h>
#include <Gauntlet/Core/Log.h>
#include <Gauntlet/Layer/Layer.h>
#include <Gauntlet/Core/Timer.h>
#include <Gauntlet/Core/Random.h>

// Math defines
#include <Gauntlet/Core/Math.h>

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
#include <Gauntlet/Renderer/Framebuffer.h>
#include <Gauntlet/Renderer/Texture.h>
#include <Gauntlet/Renderer/TextureCube.h>
#include <Gauntlet/Renderer/Material.h>
#include <Gauntlet/Renderer/Camera/Camera.h>
#include <Gauntlet/Renderer/Camera/OrthographicCamera.h>
#include <Gauntlet/Renderer/Camera/PerspectiveCamera.h>
#include <Gauntlet/Renderer/Mesh.h>

// ECS Stuff
#include <Gauntlet/Scene/Scene.h>
#include <Gauntlet/Scene/Entity.h>

// UI
#include <imgui/imgui.h>
#include <Gauntlet/ImGui/ImGuiLayer.h>

// MULTITHREADING
#include <Gauntlet/Core/JobSystem.h>
#include <Gauntlet/Core/Thread.h>

// Don't forget to #include <Legion/Core/Entrypoint.h> after this include.
