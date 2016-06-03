//
// Copyright (c) 2008-2016 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../../Precompiled.h"

#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/ShaderBuffer.h"
#include "../../IO/Log.h"

#include "../../DebugNew.h"

namespace Urho3D
{

ShaderBuffer::ShaderBuffer(Context* context, unsigned usage) :
    Object(context),
    GPUObject(GetSubsystem<Graphics>()),
    usage_(usage),
    count_(0),
    size_(0),
    dirty_(true),
    unorderedAccessView_(0),
    shaderResourceView_(0)
{
}

ShaderBuffer::~ShaderBuffer()
{
    Release();
}

void ShaderBuffer::Release()
{
    URHO3D_SAFE_RELEASE(object_);
    URHO3D_SAFE_RELEASE(shaderResourceView_);
    URHO3D_SAFE_RELEASE(unorderedAccessView_);

    sourceBuffer_.Reset();
    shadowData_.Reset();
    count_ = 0;
    size_ = 0;
}

void* ShaderBuffer::GetBuffer()
{
    dirty_ = true;
    return shadowData_ ? shadowData_.Get() : 0;
}

bool ShaderBuffer::SetSize(ShaderBuffer* source)
{
    if (!source)
    {
        URHO3D_LOGERROR("Can not create a buffer from null");
        return false;
    }

    Release();

    sourceBuffer_ = source;

    count_ = source->GetElementCount();
    size_ = source->GetElementSize();
    dirty_ = true;
    shadowData_.Reset();

    return CreateBufferAndViews(0);
}

bool ShaderBuffer::SetSize(unsigned size, unsigned count, const void* data)
{
    if (!count || !size)
    {
        URHO3D_LOGERROR("Can not create zero-sized buffer");
        return false;
    }

    Release();

    // Round up to next 16 bytes
    unsigned width = size * count;
    width += 15;
    width &= 0xfffffff0;
    count = width / size;

    count_ = count;
    size_ = size;
    dirty_ = true;
    shadowData_ = new unsigned char[size_ * count_];

    return CreateBufferAndViews(data);
}

bool ShaderBuffer::CreateBufferAndViews(const void* data)
{
    if (!graphics_)
        return false;
    ID3D11Device* device = graphics_->GetImpl()->GetDevice();

    // Buffer description
    D3D11_BUFFER_DESC bufferDesc;
    memset(&bufferDesc, 0, sizeof bufferDesc);
    bufferDesc.ByteWidth = size_ * count_;
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.CPUAccessFlags = 0;

    // Constant buffer
    if (usage_ & BUFFER_CONSTANT)
    {
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    }
    // Structured buffer
    else if (usage_ & BUFFER_STRUCTURED)
    {
        bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
        bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        bufferDesc.StructureByteStride = size_;
    }
    // Raw buffer
    else if (usage_ & BUFFER_RAW)
    {
        bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_INDEX_BUFFER | D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    }
    // Cpu read buffer
    else if (usage_ & BUFFER_CPU_READ)
    {
        if (sourceBuffer_)
        {
            ID3D11Buffer* sourceBuffer = (ID3D11Buffer*)sourceBuffer_->GetGPUObject();
            sourceBuffer->GetDesc(&bufferDesc);
        }
        bufferDesc.BindFlags = 0;
        bufferDesc.MiscFlags = 0;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        bufferDesc.Usage = D3D11_USAGE_STAGING;
    }
    else
    {
        URHO3D_LOGERROR("Buffer type not specified");
        return false;
    }

    HRESULT hr;
    if (data)
    {
        memcpy(shadowData_.Get(), data, size_ * count_);
        D3D11_SUBRESOURCE_DATA initData;
        initData.pSysMem = shadowData_.Get();
        hr = device->CreateBuffer(&bufferDesc, &initData, (ID3D11Buffer**)&object_);
    }
    else
    {
        if (shadowData_)
            memset(shadowData_.Get(), 0, size_ * count_);
        hr = device->CreateBuffer(&bufferDesc, 0, (ID3D11Buffer**)&object_);
    }

    if (FAILED(hr))
    {
        URHO3D_SAFE_RELEASE(object_);
        URHO3D_LOGD3DERROR("Failed to create buffer", hr);
        return false;
    }

    // Readable buffer, create a shader resource view (SRV) to bind input resources to shaders
    if (usage_ & BUFFER_READ)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        memset(&srvDesc, 0, sizeof srvDesc);

        // Shader resource view for structured buffer
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
        srvDesc.BufferEx.FirstElement = 0;
        srvDesc.BufferEx.NumElements = count_;

        // Shader resource view for raw buffer
        if (usage_ & BUFFER_RAW)
        {
            srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            srvDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
            srvDesc.BufferEx.NumElements = bufferDesc.ByteWidth / 4;
        }

        hr = device->CreateShaderResourceView((ID3D11Buffer*)object_, &srvDesc, (ID3D11ShaderResourceView**)&shaderResourceView_);
        if (FAILED(hr))
        {
            URHO3D_SAFE_RELEASE(shaderResourceView_);
            URHO3D_LOGD3DERROR("Failed to create shader resource view for buffer", hr);
            return false;
        }
    }

    // Writeable buffer, create a unordered resource view (UAV) to bind output resources to compute shaders,
    // CS4.0/4.1 can have only one output resource bound to a shader at a time, CS5.0 doesn't have this limitation
    if (usage_ & BUFFER_WRITE)
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
        memset(&uavDesc, 0, sizeof uavDesc);

        // Unordered access view for structured buffer
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.Flags = 0;
        uavDesc.Buffer.NumElements = count_;

        // Unordered access view for raw buffer
        if (usage_ & BUFFER_RAW)
        {
            // Format must be DXGI_FORMAT_R32_TYPELESS, when creating Raw Unordered Access View
            uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
            uavDesc.Buffer.NumElements = bufferDesc.ByteWidth / 4;
        }

        device->CreateUnorderedAccessView((ID3D11Buffer*)object_, &uavDesc, (ID3D11UnorderedAccessView**)&unorderedAccessView_);
        if (!unorderedAccessView_)
        {
            URHO3D_LOGERROR("Failed to create unordered access view");
            return false;
        }
    }

    return true;
}

void ShaderBuffer::SetElements(unsigned offset, unsigned count, const void* data)
{
    if (offset + count > count_)
        return; // Would overflow the buffer

    memcpy(&shadowData_[offset * size_], data, count * size_);
    dirty_ = true;
}

void ShaderBuffer::SetBufferData(unsigned offset, unsigned size, const void* data)
{
    if (offset + size > GetBufferSize())
        return; // Would overflow the buffer

    memcpy(&shadowData_[offset], data, size);
    dirty_ = true;
}

void ShaderBuffer::Apply()
{
    if (!object_ || !graphics_)
        return;
    GraphicsImpl* impl = graphics_->GetImpl();

    if ((usage_ & BUFFER_CPU_READ) != 0 && sourceBuffer_)
    {
        ID3D11Buffer* sourceBuffer = (ID3D11Buffer*)sourceBuffer_->GetGPUObject();
        impl->GetDeviceContext()->CopyResource((ID3D11Buffer*)object_, sourceBuffer);
    }
    else if (dirty_ && object_)
    {
        impl->GetDeviceContext()->UpdateSubresource((ID3D11Buffer*)object_, 0, 0, shadowData_.Get(), 0, 0);
        dirty_ = false;
    }
}

void* ShaderBuffer::Map()
{
    if (!object_ || !graphics_)
        return 0;
    ID3D11DeviceContext* context = graphics_->GetImpl()->GetDeviceContext();
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    if (context->Map((ID3D11Buffer*)object_, 0, D3D11_MAP_READ, 0, &mappedResource) != S_OK)
        return 0;
    return mappedResource.pData;
}

void ShaderBuffer::Unmap()
{
    if (!object_ || !graphics_)
        return;
    ID3D11DeviceContext* context = graphics_->GetImpl()->GetDeviceContext();
    context->Unmap((ID3D11Buffer*)object_, 0);
}

}//namespace
