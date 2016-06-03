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
// Based on "Compute Shader Filters" by Marco Alamia:
// http://www.codinglabs.net/tutorial_compute_shaders_filters.aspx
//

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Profiler.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/BillboardSet.h>
#include <Urho3D/Graphics/Camera.h>
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
#include <Urho3D/Graphics/ShaderBuffer.h>
#include <Urho3D/Graphics/View.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>

#include "SampleCS2.h"

#include <Urho3D/DebugNew.h>
#include <Urho3D/IO/Log.h>

// Path of the example data folder (relative to the bin folder)
#define RESOURCE_PATH "../../Source/Work/WorkTests/Data/CS2"

SampleCS2::SampleCS2(Context* context) :
    Sample(context),
    drawDebug_(false),
    debugText_(0),
    paramatersDirty_(false)
{
}

void SampleCS2::Start()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    cache->AddResourceDir(RESOURCE_PATH);

    // Execute base class startup
    Sample::Start();

    CreateLog();

    // Create the scene content
    CreateScene();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update and render post-update events
    SubscribeToEvents();
}

void SampleCS2::CreateScene()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    scene_ = new Scene(context_);

    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    // Also create a DebugRenderer component so that we can draw debug geometry
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();

    // Create the camera. Limit far clip distance to match the fog
    cameraNode_ = scene_->CreateChild("Camera");
    Camera* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(300.0f);
    cameraNode_->SetPosition(Vector3(0.0f, 1.0f, 0.0f));

    // Create a Zone component for ambient lighting & fog control
    Node* zoneNode = scene_->CreateChild("Zone");
    Zone* zone = zoneNode->CreateComponent<Zone>();
    zone->SetAmbientColor(Color(0.7f, 0.7f, 0.7f));
    zone->SetBoundingBox(BoundingBox(-2000.0f, 2000.0f));
    zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
    zone->SetFogStart(400.0f);
    zone->SetFogEnd(500.0f);

    // Create a directional light without shadows
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.3f, -0.5f, 0.425f));
    Light* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetSpecularIntensity(0.5f);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));

    {
        Node* floorNode = scene_->CreateChild("Floor");
        floorNode->SetScale(Vector3(20.0f, 1.0f, 20.f));
        StaticModel* floorObject = floorNode->CreateComponent<StaticModel>();
        floorObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        floorObject->SetMaterial(cache->GetResource<Material>("Materials/Terrain.xml"));

        Node* boxNode = scene_->CreateChild("Box");
        boxNode->SetPosition(Vector3(0.0f, 1.0f, 5.0f));
        boxNode->SetScale(1.0f);
        StaticModel* boxObject = boxNode->CreateComponent<StaticModel>();
        boxObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        boxObject->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
        boxObject->SetCastShadows(true);
        boxObject->SetOccluder(true);

        Node* boxNode2 = boxNode->Clone();
        boxNode2->SetPosition(Vector3(1.5f, 1.0f, 5.0f));
        boxNode2->SetRotation(Quaternion(0.0f, 45.0f, 0.0f));
    }

}

void SampleCS2::SetupViewport()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));

    SharedPtr<RenderPath> effectRenderPath = viewport->GetRenderPath()->Clone();
    effectRenderPath->Append(cache->GetResource<XMLFile>("PostProcess/Compute2.xml"));
    viewport->SetRenderPath(effectRenderPath);

    Renderer* renderer = GetSubsystem<Renderer>();
    renderer->SetViewport(0, viewport);
}

void SampleCS2::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(SampleCS2, HandleUpdate));

    // Subscribe HandlePostRenderUpdate() function for processing the post-render update event,
    // during which we request debug geometry
    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(SampleCS2, HandlePostRenderUpdate));

    SubscribeToEvent(E_COMMANDEVENT, URHO3D_HANDLER(SampleCS2, HandleCommandEvent));
}

void SampleCS2::CreateLog()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    UI* ui = GetSubsystem<UI>();

    debugText_ = ui->GetRoot()->CreateChild<Text>();
    debugText_->SetPosition(20, 20);
    debugText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
}

void SampleCS2::Log(const String& text, bool clear)
{
    if (!debugText_)
        return;
    if (clear)
        debugText_->SetText(text);
    else
        debugText_->SetText(debugText_->GetText() + "\n" + text);
}

void SampleCS2::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    Input* input = GetSubsystem<Input>();

    // Movement speed as world units per second
    const float MOVE_SPEED = 1.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    IntVector2 mouseMove = input->GetMouseMove();
    yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
    pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
    pitch_ = Clamp(pitch_, -90.0f, 90.0f);

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown('W'))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input->GetKeyDown('S'))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input->GetKeyDown('A'))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown('D'))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);

    // Toggle debug geometry with space
    if (input->GetKeyPress(KEY_SPACE))
        drawDebug_ = !drawDebug_;

    // Change circles color
    if (input->GetKeyPress('C'))
    {
        paramatersDirty_ = true;
        circleColor_ = Color(Random(1.0f), Random(1.0f), Random(1.0f));
    }
}

void SampleCS2::AnimateScene(float timeStep)
{
    // Get the light and billboard scene nodes
    PODVector<Node*> lightNodes;
    scene_->GetChildrenWithComponent<Light>(lightNodes);

    const float LIGHT_ROTATION_SPEED = 20.0f;

    // Rotate the lights around the world Y-axis
    for (unsigned i = 0; i < lightNodes.Size(); ++i)
        lightNodes[i]->Rotate(Quaternion(0.0f, LIGHT_ROTATION_SPEED * timeStep, 0.0f), TS_WORLD);
}

void SampleCS2::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera and animate the scene, scale movement with time step
    MoveCamera(timeStep);
    AnimateScene(timeStep);
}

void SampleCS2::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    // If draw debug mode is enabled, draw viewport debug geometry. This time use depth test, as otherwise the result becomes
    // hard to interpret due to large object count
    if (drawDebug_)
        GetSubsystem<Renderer>()->DrawDebugGeometry(true);
}

void SampleCS2::HandleCommandEvent(StringHash eventType, VariantMap& eventData)
{
    Log("[Compute shader sample 2]", true);

    Graphics* graphics = GetSubsystem<Graphics>();
    Renderer* renderer = GetSubsystem<Renderer>();
    RenderPath* renderPath = renderer->GetViewport(0)->GetRenderPath();
    unsigned cmdIndex = eventData[CommandEvent::P_COMMANDINDEX].GetUInt();
    RenderPathCommand* cmd = renderPath->GetCommand(cmdIndex);
    if (!cmd)
        return;
    Log("compute command found");

    // Get the viewport texture
    View* view = renderer->GetViewport(0)->GetView();
    Texture2D* viewportTexture = dynamic_cast<Texture2D*>(view->GetCurrentViewportTexture());
    if (!viewportTexture)
        return;

    unsigned width = viewportTexture->GetLevelWidth(0);
    unsigned height = viewportTexture->GetLevelHeight(0);
    unsigned dataSize = viewportTexture->GetDataSize(width, height);
    Log("viewport texture found " + String(width) + " x "+ String(height));

    // Create buffers if necessary
    if (!bufferIn0_)
    {
        bufferIn0_ = new ShaderBuffer(context_, ShaderBuffer::BUFFER_STRUCTURED | ShaderBuffer::BUFFER_READ);
        bufferOut_ = new ShaderBuffer(context_, ShaderBuffer::BUFFER_STRUCTURED | ShaderBuffer::BUFFER_WRITE);
        bufferRead_ = new ShaderBuffer(context_, ShaderBuffer::BUFFER_CPU_READ);

        // We suppose the viewport texture is 4 bytes per pixel
        unsigned elemSize = 4;
        unsigned elemCount = dataSize / elemSize;
        bufferIn0_->SetSize(elemSize, elemCount);
        bufferOut_->SetSize(elemSize, elemCount);
        bufferRead_->SetSize(bufferOut_);

        graphics->AddShaderBuffer("Buffer0", bufferIn0_);
        graphics->AddShaderBuffer("BufferOut", bufferOut_);
    }

    // Copy the viewport texture in the input buffer
    viewportTexture->GetData(0, bufferIn0_->GetBuffer());

    // Push the input buffer to the GPU for the compute shader to read it
    bufferIn0_->Apply();

    // Load/compile the compute shader
    ShaderVariation* cs = graphics->GetShader(CS, cmd->computeShaderName_, cmd->computeShaderDefines_);
    if (!cs)
        return;
    Log("shader ok");

    // Set compute shader and buffers
    graphics->SetComputeShader(cs);

    if (paramatersDirty_)
    {
        paramatersDirty_ = false;
        graphics->SetShaderParameter("CircleColor", circleColor_);
    }

    // Execute the compute shader
    {
        URHO3D_PROFILE(Compute);
        graphics->Compute(32, 24, 1);
    }
    Log("computed");

    // Copy the shader output buffer to the read buffer
    bufferRead_->Apply();

    // Map the read buffer to enable CPU access (and deny GPU access)
    void* p = bufferRead_->Map();
    if (!p)
        return;
    Log("output buffer mapped");

    // Copy the output buffer to the viewport texture
    if (!viewportTexture->SetData(0, 0, 0, width, height, p))
        return;
    Log("output texture loaded");

    // Disable CPU access (and enable GPU access)
    bufferRead_->Unmap();
}
