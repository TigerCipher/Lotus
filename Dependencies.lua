
-- VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
-- IncludeDir["Box2D"] = "%{wks.location}/vendor/deps/box2d/include"
-- IncludeDir["entt"] = "%{wks.location}/vendor/deps/entt/include"
-- IncludeDir["Glad"] = "%{wks.location}/vendor/deps/Glad/include"
-- IncludeDir["GLFW"] = "%{wks.location}/vendor/deps/glfw/include"
-- IncludeDir["glm"] = "%{wks.location}/vendor/deps/glm"
-- IncludeDir["ImGui"] = "%{wks.location}/vendor/deps/imgui"
-- IncludeDir["ImGuizmo"] = "%{wks.location}/vendor/deps/ImGuizmo"
-- IncludeDir["shaderc"] = "%{wks.location}/vendor/deps/shaderc/include"
-- IncludeDir["spdlog"] = "%{wks.location}/vendor/deps/spdlog/include"
-- IncludeDir["SPIRV_Cross"] = "%{wks.location}/vendor/deps/SPIRV-Cross"
-- IncludeDir["stb_image"] = "%{wks.location}/vendor/deps/stb_image"
-- IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
-- IncludeDir["yaml_cpp"] = "%{wks.location}/vendor/deps/yaml-cpp/include"
-- IncludeDir["zlib"] = "%{wks.location}/vendor/deps/zlib"

LibraryDir = {}

-- LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"
-- LibraryDir["VulkanSDK_Debug"] = "%{wks.location}/vendor/deps/VulkanSDK/Lib"
-- LibraryDir["VulkanSDK_DebugDLL"] = "%{wks.location}/vendor/deps/VulkanSDK/Bin"

Library = {}
-- Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
-- Library["VulkanUtils"] = "%{LibraryDir.VulkanSDK}/VkLayer_utils.lib"

-- Library["ShaderC_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/shaderc_sharedd.lib"
-- Library["SPIRV_Cross_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/spirv-cross-cored.lib"
-- Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/spirv-cross-glsld.lib"
-- Library["SPIRV_Tools_Debug"] = "%{LibraryDir.VulkanSDK_Debug}/SPIRV-Toolsd.lib"

-- Library["ShaderC_Release"] = "%{LibraryDir.VulkanSDK}/shaderc_shared.lib"
-- Library["SPIRV_Cross_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-core.lib"
-- Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsl.lib"