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
#include "../../Graphics/Graphics.h"
#include "../../Graphics/ShaderVariation.h"

namespace Urho3D
{

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

            // Create needed constant buffers
            const unsigned* vsBufferSizes = shader->GetConstantBufferSizes();
            for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
            {
                if (vsBufferSizes[i])
                    constantBuffers_[shaderType][i] = graphics->GetOrCreateConstantBuffer(VS, i, vsBufferSizes[i]);
            }

            // Copy parameters. Add direct links to constant buffers.
            const HashMap<StringHash, ShaderParameter>& params = shader->GetParameters();
            for (HashMap<StringHash, ShaderParameter>::ConstIterator i = params.Begin(); i != params.End(); ++i)
            {
                parameters_[i->first_] = i->second_;
                parameters_[i->first_].bufferPtr_ = constantBuffers_[shaderType][i->second_.buffer_].Get();
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
};

}
