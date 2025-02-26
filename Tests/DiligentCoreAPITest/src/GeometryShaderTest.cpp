/*
 *  Copyright 2019-2022 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#include "GPUTestingEnvironment.hpp"
#include "TestingSwapChainBase.hpp"

#include "gtest/gtest.h"

#include "InlineShaders/GeometryShaderTestHLSL.h"

namespace Diligent
{

namespace Testing
{

#if DILIGENT_D3D11_SUPPORTED
void GeometryShaderReferenceD3D11(ISwapChain* pSwapChain);
#endif

#if DILIGENT_D3D12_SUPPORTED
void GeometryShaderReferenceD3D12(ISwapChain* pSwapChain);
#endif

#if DILIGENT_GL_SUPPORTED || DILIGENT_GLES_SUPPORTED
void GeometryShaderReferenceGL(ISwapChain* pSwapChain);
#endif

#if DILIGENT_VULKAN_SUPPORTED
void GeometryShaderReferenceVk(ISwapChain* pSwapChain);
#endif

#if DILIGENT_METAL_SUPPORTED

#endif

} // namespace Testing

} // namespace Diligent

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

TEST(GeometryShaderTest, DrawTriangles)
{
    auto* const pEnv       = GPUTestingEnvironment::GetInstance();
    auto* const pDevice    = pEnv->GetDevice();
    const auto& DeviceInfo = pDevice->GetDeviceInfo();

    if (!DeviceInfo.Features.GeometryShaders)
    {
        GTEST_SKIP() << "Geometry shaders are not supported by this device";
    }

    if (!DeviceInfo.Features.SeparablePrograms)
    {
        GTEST_SKIP() << "Geometry shader test requires separable programs";
    }

    GPUTestingEnvironment::ScopedReset EnvironmentAutoReset;

    auto* pSwapChain = pEnv->GetSwapChain();
    auto* pContext   = pEnv->GetDeviceContext();

    RefCntAutoPtr<ITestingSwapChain> pTestingSwapChain(pSwapChain, IID_TestingSwapChain);
    if (pTestingSwapChain)
    {
        pContext->Flush();
        pContext->InvalidateState();

        switch (DeviceInfo.Type)
        {
#if DILIGENT_D3D11_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D11:
                GeometryShaderReferenceD3D11(pSwapChain);
                break;
#endif

#if DILIGENT_D3D12_SUPPORTED
            case RENDER_DEVICE_TYPE_D3D12:
                GeometryShaderReferenceD3D12(pSwapChain);
                break;
#endif

#if DILIGENT_GL_SUPPORTED || DILIGENT_GLES_SUPPORTED
            case RENDER_DEVICE_TYPE_GL:
            case RENDER_DEVICE_TYPE_GLES:
                GeometryShaderReferenceGL(pSwapChain);
                break;

#endif

#if DILIGENT_VULKAN_SUPPORTED
            case RENDER_DEVICE_TYPE_VULKAN:
                GeometryShaderReferenceVk(pSwapChain);
                break;

#endif

            default:
                LOG_ERROR_AND_THROW("Unsupported device type");
        }

        pTestingSwapChain->TakeSnapshot();
    }

    ITextureView* pRTVs[] = {pSwapChain->GetCurrentBackBufferRTV()};
    pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    float ClearColor[] = {0.f, 0.f, 0.f, 0.0f};
    pContext->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);


    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    auto& PSODesc          = PSOCreateInfo.PSODesc;
    auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Geometry shader test";

    PSODesc.PipelineType                          = PIPELINE_TYPE_GRAPHICS;
    GraphicsPipeline.NumRenderTargets             = 1;
    GraphicsPipeline.RTVFormats[0]                = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_POINT_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

    ShaderCreateInfo ShaderCI;
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    ShaderCI.ShaderCompiler = pEnv->GetDefaultCompiler(ShaderCI.SourceLanguage);

    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc       = {"Geometry shader test - VS", SHADER_TYPE_VERTEX, true};
        ShaderCI.EntryPoint = "main";
        ShaderCI.Source     = HLSL::GSTest_VS.c_str();
        pDevice->CreateShader(ShaderCI, &pVS);
        ASSERT_NE(pVS, nullptr);
    }

    RefCntAutoPtr<IShader> pGS;
    {
        ShaderCI.Desc       = {"Geometry shader test - GS", SHADER_TYPE_GEOMETRY, true};
        ShaderCI.EntryPoint = "main";
        ShaderCI.Source     = HLSL::GSTest_GS.c_str();
        pDevice->CreateShader(ShaderCI, &pGS);
        ASSERT_NE(pGS, nullptr);
    }

    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc       = {"Geometry shader test - PS", SHADER_TYPE_PIXEL, true};
        ShaderCI.EntryPoint = "main";
        ShaderCI.Source     = HLSL::GSTest_PS.c_str();
        pDevice->CreateShader(ShaderCI, &pPS);
        ASSERT_NE(pPS, nullptr);
    }

    PSODesc.Name = "Geometry shader test";

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pGS = pGS;
    PSOCreateInfo.pPS = pPS;
    RefCntAutoPtr<IPipelineState> pPSO;
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
    ASSERT_NE(pPSO, nullptr);

    pContext->SetPipelineState(pPSO);

    DrawAttribs drawAttrs(2, DRAW_FLAG_VERIFY_ALL);
    pContext->Draw(drawAttrs);

    pSwapChain->Present();
}

} // namespace
