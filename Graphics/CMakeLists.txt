cmake_minimum_required (VERSION 3.6...3.24.2)

set(DLL_REL_SUFFIX _${ARCH}r)
set(DLL_DBG_SUFFIX _${ARCH}d)

add_library(Diligent-GraphicsEngineInterface INTERFACE)
target_include_directories(Diligent-GraphicsEngineInterface
INTERFACE
    GraphicsEngine/interface
)
target_link_libraries(Diligent-GraphicsEngineInterface
INTERFACE
    Diligent-Primitives
    Diligent-PublicBuildSettings
)

add_subdirectory(GraphicsAccessories)
add_subdirectory(GraphicsEngine)

if(DILIGENT_GL_SUPPORTED OR DILIGENT_GLES_SUPPORTED OR DILIGENT_VULKAN_SUPPORTED)
    add_subdirectory(HLSL2GLSLConverterLib)
endif()

add_subdirectory(ShaderTools)

if(DILIGENT_D3D12_SUPPORTED OR DILIGENT_VULKAN_SUPPORTED OR DILIGENT_METAL_SUPPORTED)
    add_subdirectory(GraphicsEngineNextGenBase)
endif()

if(DILIGENT_D3D11_SUPPORTED OR DILIGENT_D3D12_SUPPORTED)
    add_subdirectory(GraphicsEngineD3DBase)
endif()

if(DILIGENT_D3D11_SUPPORTED)
    add_subdirectory(GraphicsEngineD3D11)
endif()

if(DILIGENT_D3D12_SUPPORTED)
    add_subdirectory(GraphicsEngineD3D12)
endif()

if(DILIGENT_VULKAN_SUPPORTED)
    add_subdirectory(GraphicsEngineVulkan)
endif()

if(DILIGENT_GL_SUPPORTED OR DILIGENT_GLES_SUPPORTED)
    add_subdirectory(GraphicsEngineOpenGL)
endif()


if(DILIGENT_ARCHIVER_SUPPORTED)
    add_subdirectory(Archiver)
endif()

add_subdirectory(GraphicsTools)
