/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

#include "GraphicsUtilities.h"

#if DILIGENT_D3D12_SUPPORTED
#    include <d3d12.h>
#    include "RenderDeviceD3D12.h"

#    include "RefCntAutoPtr.hpp"
#    include "DebugUtilities.hpp"
#endif

namespace Diligent
{

bool GetRenderDeviceD3D12MaxShaderVersion(IRenderDevice* pDevice, ShaderVersion& Version)
{
    Version = {};
#if DILIGENT_D3D12_SUPPORTED
    if (pDevice == nullptr)
    {
        UNEXPECTED("pDevice must not be null");
        return false;
    }

    if (pDevice->GetDeviceInfo().Type != RENDER_DEVICE_TYPE_D3D12)
    {
        return false;
    }

    RefCntAutoPtr<IRenderDeviceD3D12> pDeviceD3D12{pDevice, IID_RenderDeviceD3D12};
    if (!pDeviceD3D12)
    {
        UNEXPECTED("Failed to query the IRenderDeviceD3D12 interface");
        return false;
    }

    Version = pDeviceD3D12->GetMaxShaderVersion();
    return true;
#else
    return false;
#endif
}

} // namespace Diligent


extern "C"
{
    bool Diligent_GetRenderDeviceD3D12MaxShaderVersion(Diligent::IRenderDevice* pDevice, Diligent::ShaderVersion& Version)
    {
        return Diligent::GetRenderDeviceD3D12MaxShaderVersion(pDevice, Version);
    }
}
