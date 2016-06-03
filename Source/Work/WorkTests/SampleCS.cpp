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
// Based on Microsoft's "Basic compute example" (MIT licensed):
// https://github.com/walbourn/directx-sdk-samples/tree/master/BasicCompute11
// https://code.msdn.microsoft.com/windowsdesktop/DirectCompute-Basic-Win32-7d5a7408
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

#include "SampleCS.h"

#include <Urho3D/DebugNew.h>
#include <Urho3D/IO/Log.h>
#include <cstdio>

// Path of the example data folder (relative to the bin folder)
#define RESOURCE_PATH "../../Source/Work/WorkTests/Data/CS"

// Execute the compute operation during the event command
//#define COMPUTE_ON_EVENT

// Use structured buffers instead of raw buffers
#define USE_STRUCTURED_BUFFERS

// Test a double-precision data type, this requires CS 5.0 hardware and driver support for double-precision shaders
#define TEST_DOUBLE

// The number of elements in a buffer to be tested
const unsigned NUM_ELEMENTS = 1024;

const String csDefines = String(
#ifdef USE_STRUCTURED_BUFFERS
        " USE_STRUCTURED_BUFFERS"
#endif
#ifdef TEST_DOUBLE
        " TEST_DOUBLE"
#endif
    );

struct BufType
{
    int i;
    float f;
#ifdef TEST_DOUBLE
    double d;
#endif
};
BufType g_vBuf0[NUM_ELEMENTS];
BufType g_vBuf1[NUM_ELEMENTS];


SampleCS::SampleCS(Context* context) :
    Sample(context),
    debugText_(0)
{
}

void SampleCS::Start()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    cache->AddResourceDir(RESOURCE_PATH);

    // Execute base class startup
    Sample::Start();

    Init();

    CreateText();

    // Setup the viewport
    SetupViewport();

    // Hook up to the custom command event
    SubscribeToEvents();
}

void SampleCS::Init()
{
    // Buffers initialization
    for ( int i = 0; i < NUM_ELEMENTS; ++i )
    {
        g_vBuf0[i].i = (0+i);
        g_vBuf0[i].f = (float)(0+i);
#ifdef TEST_DOUBLE
        g_vBuf0[i].d = (double)(0+i);
#endif

        g_vBuf1[i].i = (0+i);
        g_vBuf1[i].f = (float)(0+i);
#ifdef TEST_DOUBLE
        g_vBuf1[i].d = (double)(0+i);
#endif
    }

#ifdef USE_STRUCTURED_BUFFERS
    bufferIn0_ = new ShaderBuffer(context_, ShaderBuffer::BUFFER_STRUCTURED | ShaderBuffer::BUFFER_READ);
    bufferIn1_ = new ShaderBuffer(context_, ShaderBuffer::BUFFER_STRUCTURED | ShaderBuffer::BUFFER_READ);
    bufferOut_ = new ShaderBuffer(context_, ShaderBuffer::BUFFER_STRUCTURED | ShaderBuffer::BUFFER_WRITE);
#else
    bufferIn0_ = new ShaderBuffer(context_, ShaderBuffer::BUFFER_RAW | ShaderBuffer::BUFFER_READ);
    bufferIn1_ = new ShaderBuffer(context_, ShaderBuffer::BUFFER_RAW | ShaderBuffer::BUFFER_READ);
    bufferOut_ = new ShaderBuffer(context_, ShaderBuffer::BUFFER_RAW | ShaderBuffer::BUFFER_WRITE);
#endif
    bufferIn0_->SetSize(sizeof(BufType), NUM_ELEMENTS, &g_vBuf0[0]);
    bufferIn1_->SetSize(sizeof(BufType), NUM_ELEMENTS, &g_vBuf1[0]);
    bufferOut_->SetSize(sizeof(BufType), NUM_ELEMENTS, 0);

    Graphics* graphics = GetSubsystem<Graphics>();
    graphics->AddShaderBuffer("Buffer0", bufferIn0_);
    graphics->AddShaderBuffer("Buffer1", bufferIn1_);
    graphics->AddShaderBuffer("BufferOut", bufferOut_);
}

void SampleCS::SetupViewport()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    RenderPath* renderPath = new RenderPath();
    XMLFile* xml = cache->GetResource<XMLFile>("RenderPaths/Compute.xml");
    renderPath->Load(xml);

    SharedPtr<Viewport> viewport(new Viewport(context_, 0, 0, renderPath));

    Renderer* renderer = GetSubsystem<Renderer>();
    renderer->SetViewport(0, viewport);

    RenderPathCommand* cmd = renderPath->GetCommand("basic");
    if (cmd)
#ifdef COMPUTE_ON_EVENT
        cmd->enabled_ = false;
#else
        cmd->computeShaderDefines_ = csDefines;
#endif
}

void SampleCS::SubscribeToEvents()
{
    SubscribeToEvent(E_COMMANDEVENT, URHO3D_HANDLER(SampleCS, HandleCommandEvent));
}

void SampleCS::CreateText()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    UI* ui = GetSubsystem<UI>();

    debugText_ = ui->GetRoot()->CreateChild<Text>();
    debugText_->SetPosition(20, 20);
    debugText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    PrintText(String());
}

void SampleCS::PrintText(const String& text)
{
    if (text.Empty())
        debugText_->SetText("[Compute shader sample]");
    else
        debugText_->SetText(debugText_->GetText() + "\n" + text);
}

void SampleCS::HandleCommandEvent(StringHash eventType, VariantMap& eventData)
{
    PrintText(String());

    unsigned commandIndex = eventData[CommandEvent::P_COMMANDINDEX].GetUInt();

    // Get some info from the command
    Renderer* renderer = GetSubsystem<Renderer>();
    RenderPath* renderPath = renderer->GetViewport(0)->GetRenderPath();
    RenderPathCommand* cmd = renderPath->GetCommand(commandIndex);
    if (!cmd)
        return;
    PrintText("command index: " + String(commandIndex) + ", tag: " + cmd->tag_);

#ifdef COMPUTE_ON_EVENT
    // Get the shader, compile it if necessary
    Graphics* graphics = GetSubsystem<Graphics>();
    ShaderVariation* cs = graphics->GetShader(CS, "BasicCompute", csDefines);
    if (!cs)
        return;
    PrintText("shader ok");

    // Set compute shader and buffers
    graphics->SetComputeShader(cs);

    // Execute the compute shader
    {
        URHO3D_PROFILE(Compute);
        graphics->Compute(NUM_ELEMENTS, 1, 1);
    }
    PrintText("computed");
#endif

    // Read back the result from GPU, verify its correctness against result computed by CPU
    {
        ShaderBuffer debugBuffer(context_, ShaderBuffer::BUFFER_CPU_READ);
        debugBuffer.SetSize(bufferOut_);
        debugBuffer.Apply();
        BufType* p = (BufType*)debugBuffer.Map();
        if (!p)
            return;

        // Verify that if Compute Shader has done right
        bool success = true;
        for (unsigned i = 0; i < NUM_ELEMENTS; ++i)
        {
            if ( (p[i].i != g_vBuf0[i].i + g_vBuf1[i].i)
                 || (p[i].f != g_vBuf0[i].f + g_vBuf1[i].f)
#ifdef TEST_DOUBLE
                 || (p[i].d != g_vBuf0[i].d + g_vBuf1[i].d)
#endif
               )
            {
                char text[512];
                sprintf(text, "%3d + %3d = CPU %3d, CS %3d",
                        g_vBuf0[i].i, g_vBuf1[i].i, g_vBuf0[i].i + g_vBuf1[i].i, p[i].i);
                PrintText(text);
                sprintf(text, "%3.1f + %3.1f = CPU %3.1f, CS %3.1f",
                        g_vBuf0[i].f, g_vBuf1[i].f, g_vBuf0[i].f + g_vBuf1[i].f, p[i].f);
                PrintText(text);
                success = false;
                break;
            }
        }
        if (success)
            PrintText("verify succeeded");
        else
            PrintText("verify failure");

        debugBuffer.Unmap();
    }
}
