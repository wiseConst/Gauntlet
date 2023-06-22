#pragma once

#include "RendererAPI.h"

namespace Eclipse
{

	class Renderer
	{
	public:
		Renderer() = delete;
		~Renderer() = delete;

		static void Init(RendererAPI::EAPI GraphicsAPI);
		static void Shutdown();

	};

}
