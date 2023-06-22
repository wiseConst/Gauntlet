#pragma once

#include "Eclipse/Core/Core.h"
#include "Eclipse/Renderer/GraphicsContext.h"

#ifdef ELS_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <volk/volk.h>

namespace Eclipse
{

	class VulkanDevice;

	class VulkanContext final : public GraphicsContext
	{
	public:
		VulkanContext(Scoped<Window>& InWindow);
		
		virtual void BeginRender() final override;
        virtual void EndRender() final override;
		virtual void Destroy() final override;

	private:
		VkInstance m_Instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		Scoped<VulkanDevice> m_Device;

		void CreateInstance();
        void CreateDebugMessenger();
		void CreateSurface();

		bool CheckVulkanAPISupport() const;
        bool CheckValidationLayerSupport();
        const std::vector<const char*> GetRequiredExtensions();
	};

}