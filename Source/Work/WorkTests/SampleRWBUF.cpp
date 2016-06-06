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

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Profiler.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/BillboardSet.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/RenderPath.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/ShaderBuffer.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>

#include "SampleRWBUF.h"

#include <Urho3D/DebugNew.h>
#include <Urho3D/IO/Log.h>
#include <cstdio>

// Path of the example data folder (relative to the bin folder)
#define RESOURCE_PATH "../../Source/Work/WorkTests/Data/RWBUF"

#define DIVS 7

SampleRWBUF::SampleRWBUF(Context* context) :
    Sample(context)
{
}

void SampleRWBUF::Start()
{
    GetSubsystem<ResourceCache>()->AddResourceDir(RESOURCE_PATH);

    // Execute base class startup
    Sample::Start();

    Init();

    // Setup the viewport
    SetupViewport();
}

void SampleRWBUF::Init()
{
    dataBuffer_ = new ShaderBuffer(context_, ShaderBuffer::BUFFER_STRUCTURED |
                                   ShaderBuffer::BUFFER_READ | ShaderBuffer::BUFFER_WRITE);

    Graphics* graphics = GetSubsystem<Graphics>();
    graphics->AddShaderBuffer("DataBuffer", dataBuffer_);

    struct Data
    {
        unsigned color;
    };

    unsigned elemSize = sizeof(Data);
    unsigned elemCount = DIVS * DIVS;
    dataBuffer_->SetSize(elemSize, elemCount);

    // Clear the buffer, so that InterlockedMax can work
    for (unsigned i=0; i<elemCount; ++i)
    {
        Data v;
        v.color = 0;
        dataBuffer_->SetElements(i, 1, &v);
    }
    dataBuffer_->Apply();
}

void SampleRWBUF::SetupViewport()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    XMLFile* renderPathXml = cache->GetResource<XMLFile>("RenderPaths/RWBUF.xml");
    RenderPath* renderPath = new RenderPath();
    renderPath->Load(renderPathXml);

    RenderPathCommand* writeCmd = renderPath->GetCommand("WriteBuffer");
    if (writeCmd)
    {
        writeCmd->pixelShaderDefines_ += " DIVS="+String(DIVS);
    }
    RenderPathCommand* readCmd = renderPath->GetCommand("ReadBuffer");
    if (readCmd)
    {
        readCmd->pixelShaderDefines_ += " DIVS="+String(DIVS);
    }

    SharedPtr<Viewport> viewport(new Viewport(context_, 0, 0, renderPath));

    Renderer* renderer = GetSubsystem<Renderer>();
    renderer->SetViewport(0, viewport);
}
