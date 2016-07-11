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

#pragma once

#include "../../Container/HashMap.h"
#include "../../Graphics/ConstantBuffer.h"
#include "../../Graphics/ShaderBuffer.h"
#include "../../Graphics/Graphics.h"
#include "../../Graphics/ShaderVariation.h"
#include "../../IO/Log.h"

namespace Urho3D
{

/// A pair of a buffer and the slot where it is bound.
typedef Pair<unsigned, SharedPtr<ShaderBuffer> > SlotBufferPair;
/// A pair of a texture and the slot where it is bound.
typedef Pair<unsigned, WeakPtr<Texture> > SlotTexturePair;

/// Combined information for specific vertex and pixel shaders.
class URHO3D_API ShaderProgram : public RefCounted
{
public:
    /// Construct.
    ShaderProgram(Graphics* graphics, ShaderVariation* vertexShader, ShaderVariation* pixelShader,
                  ShaderVariation* geometryShader, ShaderVariation* computeShader)
    {
        ShaderVariation* shaders[MAX_SHADER_TYPE];
        shaders[VS] = vertexShader;
        shaders[PS] = pixelShader;
        shaders[GS] = geometryShader;
        shaders[CS] = computeShader;

        for (unsigned shaderType = 0; shaderType < MAX_SHADER_TYPE; ++shaderType)
        {
            ShaderVariation* shader = shaders[shaderType];
            if (!shader)
                continue;

            // Shader resources.
            HashMap<StringHash, ShaderResource> resources = shader->GetResources();
            for (HashMap<StringHash, ShaderResource>::ConstIterator i = resources.Begin(); i != resources.End(); ++i)
            {
                StringHash resourceHash = i->first_;
                const ShaderResource& resource = i->second_;
                if (resource.type_ == SR_CBV)
                {
                    // Create needed constant buffers
                    constantBuffers_[shaderType][resource.bindSlot_] =
                            graphics->GetOrCreateConstantBuffer((ShaderType)shaderType, resourceHash, &resource);

                    // Copy parameters. Add direct links to constant buffers.
                    const HashMap<StringHash, ShaderParameter>& params = shader->GetParameters();
                    for (HashMap<StringHash, ShaderParameter>::ConstIterator j = params.Begin(); j != params.End(); ++j)
                    {
                        parameters_[j->first_] = j->second_;
                        parameters_[j->first_].bufferPtr_ = constantBuffers_[shaderType][j->second_.buffer_].Get();
                    }
                }
                else if (resource.type_ == SR_SRV)
                {
                    // Get the structured buffer, it should be already created by the user
                    ShaderBuffer* buffer = graphics->GetShaderBuffer(resourceHash);
                    if (!buffer)
                        URHO3D_LOGERROR("SRV buffer " + resource.name_ + " not defined");
                    else if (resource.size_ && buffer->GetElementSize() != resource.size_)
                        URHO3D_LOGERROR("SRV buffer " + resource.name_ + " requires elements of size " + String(resource.size_) +
                                 " but it was created with size " + String(buffer->GetElementSize()) );
                    else if ((buffer->GetUsage() & ShaderBuffer::BUFFER_READ) == 0)
                        URHO3D_LOGERROR("SRV buffer " + resource.name_ + " needs the BUFFER_READ flag");
                    else
                        resourceViewBuffers_.Push(MakePair(resource.bindSlot_, SharedPtr<ShaderBuffer>(buffer)));
                }
                else if (resource.type_ == SR_UAV_STRUCTURED)
                {
                    // Get the unordered buffer, it should be already created by the user
                    ShaderBuffer* buffer = graphics->GetShaderBuffer(resourceHash);
                    if (!buffer)
                        URHO3D_LOGERROR("UAV buffer " + resource.name_ + " not defined");
                    else if (resource.size_ && buffer->GetElementSize() != resource.size_)
                        URHO3D_LOGERROR("UAV buffer " + resource.name_ + " requires elements of size " + String(resource.size_) +
                                 " but it was created with size " + String(buffer->GetElementSize()) );
                    else if ((buffer->GetUsage() & ShaderBuffer::BUFFER_WRITE) == 0)
                        URHO3D_LOGERROR("UAV buffer " + resource.name_ + " needs the BUFFER_WRITE flag");
                    else
                        accessViewBuffers_.Push(MakePair(resource.bindSlot_, SharedPtr<ShaderBuffer>(buffer)));
                }
                else if (resource.type_ == SR_UAV_TYPED)
                {
                    // Get the texture where compute shaders can write to
                    Texture* texture = graphics->GetComputeTarget(resourceHash, resource.bindSlot_);
                    if (!texture)
                        URHO3D_LOGERROR("UAV texture " + resource.name_ + " not found");
                    else if (texture->GetUsage() != TEXTURE_COMPUTETARGET)
                        URHO3D_LOGERROR("UAV texture " + resource.name_ + " needs COMPUTETARGET usage");
                    else
                        accessViewTextures_.Push(MakePair(resource.bindSlot_, WeakPtr<Texture>(texture)));
                }

            }
        }

        // Optimize shader parameter lookup by rehashing to next power of two
        parameters_.Rehash(NextPowerOfTwo(parameters_.Size()));

    }

    /// Destruct.
    ~ShaderProgram()
    {
    }

    /// Combined parameters from all the shaders.
    HashMap<StringHash, ShaderParameter> parameters_;
    /// Shader constant buffers.
    SharedPtr<ConstantBuffer> constantBuffers_[MAX_SHADER_TYPE][MAX_SHADER_PARAMETER_GROUPS];
    /// Input buffers accessed by a shader resource views (SRV).
    Vector< SlotBufferPair > resourceViewBuffers_;
    /// Output buffers accessed by a unordered access views (UAV).
    Vector< SlotBufferPair > accessViewBuffers_;
    /// Output textures accessed by a unordered access views (UAV).
    Vector< SlotTexturePair > accessViewTextures_;
};

}
