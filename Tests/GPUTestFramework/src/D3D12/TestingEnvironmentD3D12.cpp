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

#include "D3D12/TestingEnvironmentD3D12.hpp"
#include "RenderDeviceD3D12.h"
#include "DeviceContextD3D12.h"
#include "dxc/dxcapi.h"

namespace Diligent
{

namespace Testing
{

void CreateTestingSwapChainD3D12(IRenderDevice*       pDevice,
                                 IDeviceContext*      pContext,
                                 const SwapChainDesc& SCDesc,
                                 ISwapChain**         ppSwapChain);

TestingEnvironmentD3D12::TestingEnvironmentD3D12(const CreateInfo&    CI,
                                                 const SwapChainDesc& SCDesc) :
    GPUTestingEnvironment{CI, SCDesc},
    m_WaitForGPUEventHandle{CreateEvent(nullptr, false, false, nullptr)},
    m_pDxCompiler{CreateDXCompiler(DXCompilerTarget::Direct3D12, 0, nullptr)}
{
    RefCntAutoPtr<IRenderDeviceD3D12> pRenderDeviceD3D12{m_pDevice, IID_RenderDeviceD3D12};
    m_pd3d12Device = pRenderDeviceD3D12->GetD3D12Device();
    auto hr =
        m_pd3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(m_pd3d12CmdAllocator),
                                               reinterpret_cast<void**>(static_cast<ID3D12CommandAllocator**>(&m_pd3d12CmdAllocator)));
    VERIFY_EXPR(SUCCEEDED(hr));

    hr = m_pd3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(m_pd3d12Fence),
                                     reinterpret_cast<void**>(static_cast<ID3D12Fence**>(&m_pd3d12Fence)));
    VERIFY_EXPR(SUCCEEDED(hr));

    if (m_pSwapChain == nullptr)
    {
        CreateTestingSwapChainD3D12(m_pDevice, GetDeviceContext(), SCDesc, &m_pSwapChain);
    }

    // As of Windows version 2004 (build 19041), there is a bug in D3D12 WARP rasterizer:
    // Shader resource array indexing always references array element 0 when shaders are compiled
    // with shader model 5.1:
    //      AllCorrect *= CheckValue(g_Tex2DArr_Static[0].SampleLevel(g_Sampler, UV.xy, 0.0), Tex2DArr_Static_Ref0); // OK
    //      AllCorrect *= CheckValue(g_Tex2DArr_Static[1].SampleLevel(g_Sampler, UV.xy, 0.0), Tex2DArr_Static_Ref1); // FAIL - g_Tex2DArr_Static[0] is sampled
    // The shaders work OK when using shader model 5.0 with old compiler.
    // TODO: this should be fixed in the next Windows release - verify.
    m_NeedWARPResourceArrayIndexingBugWorkaround = GetAdapterType() == ADAPTER_TYPE_SOFTWARE;
}

TestingEnvironmentD3D12::~TestingEnvironmentD3D12()
{
    CloseHandle(m_WaitForGPUEventHandle);
}

CComPtr<ID3D12GraphicsCommandList> TestingEnvironmentD3D12::CreateGraphicsCommandList()
{
    CComPtr<ID3D12GraphicsCommandList> pd3d12CmdList;
    m_pd3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3d12CmdAllocator, nullptr, __uuidof(ID3D12GraphicsCommandList4),
                                      reinterpret_cast<void**>(static_cast<ID3D12GraphicsCommandList**>(&pd3d12CmdList)));
    return pd3d12CmdList;
}

void TestingEnvironmentD3D12::IdleCommandQueue(ID3D12CommandQueue* pd3d12Queue)
{
    Uint64 LastSignaledFenceValue = m_NextFenceValue;
    ++m_NextFenceValue;

    pd3d12Queue->Signal(m_pd3d12Fence, LastSignaledFenceValue);

    if (m_pd3d12Fence->GetCompletedValue() < LastSignaledFenceValue)
    {
        m_pd3d12Fence->SetEventOnCompletion(LastSignaledFenceValue, m_WaitForGPUEventHandle);
        WaitForSingleObject(m_WaitForGPUEventHandle, INFINITE);
        VERIFY(m_pd3d12Fence->GetCompletedValue() == LastSignaledFenceValue, "Unexpected signaled fence value");
    }
}

void TestingEnvironmentD3D12::ExecuteCommandList(ID3D12CommandList* pCmdList, bool WaitForIdle)
{
    RefCntAutoPtr<ICommandQueueD3D12> pQueueD3D12{GetDeviceContext()->LockCommandQueue(), IID_CommandQueueD3D12};

    auto* pd3d12Queue = pQueueD3D12->GetD3D12CommandQueue();

    ID3D12CommandList* pCmdLists[] = {pCmdList};
    pd3d12Queue->ExecuteCommandLists(_countof(pCmdLists), pCmdLists);
    if (WaitForIdle)
        IdleCommandQueue(pd3d12Queue);

    GetDeviceContext()->UnlockCommandQueue();
}

GPUTestingEnvironment* CreateTestingEnvironmentD3D12(const GPUTestingEnvironment::CreateInfo& CI,
                                                     const SwapChainDesc&                     SCDesc)
{
    return new TestingEnvironmentD3D12{CI, SCDesc};
}

HRESULT TestingEnvironmentD3D12::CompileDXILShader(const std::string& Source,
                                                   LPCWSTR            strFunctionName,
                                                   const DxcDefine*   Defines,
                                                   Uint32             DefinesCount,
                                                   LPCWSTR            profile,
                                                   ID3DBlob**         ppBlobOut)
{
    const wchar_t* pArgs[] =
        {
            L"-Zpc", // Matrices in column-major order
            L"-WX",  // Warnings as errors
            L"-Od"   // Disable optimization
        };

    CComPtr<IDxcBlob> errors;

    IDXCompiler::CompileAttribs CA;
    CA.Source                     = Source.c_str();
    CA.SourceLength               = static_cast<Uint32>(Source.length());
    CA.EntryPoint                 = strFunctionName;
    CA.Profile                    = profile;
    CA.pDefines                   = Defines;
    CA.DefinesCount               = DefinesCount;
    CA.pArgs                      = pArgs;
    CA.ArgsCount                  = _countof(pArgs);
    CA.pShaderSourceStreamFactory = nullptr;
    CA.ppBlobOut                  = reinterpret_cast<IDxcBlob**>(ppBlobOut);
    CA.ppCompilerOutput           = &errors;
    if (!m_pDxCompiler->Compile(CA))
    {
        const char* CompilerMsg = errors ? static_cast<const char*>(errors->GetBufferPointer()) : nullptr;

        LOG_INFO_MESSAGE("Failed to compile DXIL shader :\n", (CompilerMsg != nullptr ? CompilerMsg : "<no compiler log available>"));
        return E_FAIL;
    }
    return S_OK;
}

#if !DILIGENT_D3D11_SUPPORTED
HRESULT CompileD3DShader(const std::string&      Source,
                         LPCSTR                  strFunctionName,
                         const D3D_SHADER_MACRO* pDefines,
                         LPCSTR                  profile,
                         ID3DBlob**              ppBlobOut)
{
    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#    if defined(DILIGENT_DEBUG)
    // Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows
    // the shaders to be optimized and to run exactly the way they will run in
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#    else
    // Warning: do not use this flag as it causes shader compiler to fail the compilation and
    // report strange errors:
    // dwShaderFlags |= D3D10_SHADER_OPTIMIZATION_LEVEL3;
#    endif

    CComPtr<ID3DBlob> pCompilerOutput;

    auto hr = D3DCompile(Source.c_str(), Source.length(), NULL, pDefines, nullptr, strFunctionName, profile, dwShaderFlags, 0, ppBlobOut, &pCompilerOutput);

    const char* CompilerMsg = pCompilerOutput ? reinterpret_cast<const char*>(pCompilerOutput->GetBufferPointer()) : nullptr;
    if (FAILED(hr))
    {
        LOG_ERROR_AND_THROW("Failed to compile D3D shader :\n", (CompilerMsg != nullptr ? CompilerMsg : "<no compiler log available>"));
    }
    else if (CompilerMsg != nullptr)
    {
        LOG_INFO_MESSAGE("Shader compiler output:\n", CompilerMsg);
    }

    return hr;
}
#endif // !DILIGENT_D3D11_SUPPORTED

} // namespace Testing

} // namespace Diligent
