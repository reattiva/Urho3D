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
#include <Urho3D/Graphics/RenderPath.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/DebugNew.h>
#include <Urho3D/IO/Log.h>

#include "SampleScene.h"

// Path of the example data folder (relative to the bin folder)
#define RESOURCE_PATH "../../Source/Work/WorkTests/Data/Scene"

SampleScene::SampleScene(Context* context) :
    Sample(context),
    text_(0),
    baseCommand_(0)
{
}

void SampleScene::Start()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    cache->AddResourceDir(RESOURCE_PATH);

    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update and render post-update events
    SubscribeToEvents();
}

void SampleScene::CreateScene()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);

    scene_->CreateComponent<Octree>();

    cameraNode_ = scene_->CreateChild("Camera");
    cameraNode_->CreateComponent<Camera>();
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));

    Node* zoneNode = scene_->CreateChild("Zone");
    Zone* zone = zoneNode->CreateComponent<Zone>();
    zone->SetAmbientColor(Color(0.6f, 0.6f, 0.6f));
    zone->SetBoundingBox(BoundingBox(-2000.0f, 2000.0f));
    zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
    zone->SetFogStart(400.0f);
    zone->SetFogEnd(500.0f);

    Node* planeNode = scene_->CreateChild("Plane");
    planeNode->SetScale(Vector3(100.0f, 1.0f, 100.0f));
    StaticModel* planeObject = planeNode->CreateComponent<StaticModel>();
    planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    planeObject->SetMaterial(cache->GetResource<Material>("Materials/NinjaSnowWar/CloudPlane.xml"));

    Material* crateMaterial = cache->GetResource<Material>("Materials/NinjaSnowWar/SnowCrate.xml");
    Technique* technique = crateMaterial->GetTechnique(0);
    if (technique)
    {
        for (unsigned i=0; i < technique->GetNumPasses(); ++i)
        {
            Pass* pass = technique->GetPass(i);
            if (pass && pass->GetName() == "base")
            {
                basePass_ = pass;
                basePass_->SetPixelShader("SceneParam");
                break;
            }
        }
    }

    const unsigned NUM_OBJECTS = 400;
    for (unsigned i = 0; i < NUM_OBJECTS; ++i)
    {
        Node* crateNode = scene_->CreateChild("Crate");
        crateNode->SetScale(0.5f + Random(2.0f));
        float height = 0.5f;//i > NUM_OBJECTS/2 ? 0.5f : 0.51f;
        crateNode->SetPosition(Vector3(Random(90.0f) - 45.0f, crateNode->GetScale().x_ * height, Random(90.0f) - 45.0f));
        crateNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
        StaticModel* crateObject = crateNode->CreateComponent<StaticModel>();
        crateObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        crateObject->SetMaterial(crateMaterial);
    }
}

void SampleScene::SetupViewport()
{
    Renderer* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);

    RenderPath* renderPath = viewport->GetRenderPath();
    if (renderPath)
    {
        for (unsigned i=0; i < renderPath->GetNumCommands(); ++i)
        {
            RenderPathCommand* command = renderPath->GetCommand(i);
            if (command && command->pass_ == "base")
            {
                baseCommand_ = command;
                break;
            }
        }
    }
}

void SampleScene::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(SampleScene, HandleUpdate));
}

void SampleScene::MoveCamera(float timeStep)
{
    static unsigned iParam = 0;
    static unsigned iDepth = 0;
    static unsigned iCull = 0;
    static bool updateText = true;

    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    Input* input = GetSubsystem<Input>();

    // Movement speed as world units per second
    const float MOVE_SPEED = 10.0f;
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

    // Change scene pass shader parameter
    if (baseCommand_ && input->GetKeyPress(KEY_Z))
    {
        const static Vector4 COLORS[4] = { Vector4(0.0f, 0.0f, 0.0f, 0.0f), Vector4(1.0f, 0.0f, 0.0f, 0.0f),
            Vector4(0.0f, 1.0f, 0.0f, 0.0f), Vector4(0.0f, 0.0f, 1.0f, 0.0f) };
        updateText = true;
        iParam = ++iParam % 4;
        baseCommand_->SetShaderParameter("SceneColor", COLORS[iParam]);
    }

    // Change depth test mode
    if (basePass_ && input->GetKeyPress(KEY_X))
    {
        updateText = true;
        iDepth = ++iDepth % MAX_COMPAREMODES;
        basePass_->SetDepthTestMode((CompareMode)iDepth);

        Node* planeNode = scene_->GetChild("Plane");
        if (planeNode)
            planeNode->SetEnabled(false);//iDepth == CMP_LESS);
    }

    // Change per-pass cull mode
    if (basePass_ && input->GetKeyPress(KEY_C))
    {
        updateText = true;
        iCull = ++iCull % MAX_CULLMODES;
        basePass_->SetCullMode((CullMode)iCull);
    }

    if (updateText)
    {
        updateText = false;
        Log();
        Log("[Z] Scene pass shader parameter: " + String(iParam));
        Log("[X] Depth test mode: " + String(iDepth));
        Log("[C] Cull mode: " + String(iCull));
    }
}

void SampleScene::Log(const String& text)
{
    if (!text_)
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();
        UI* ui = GetSubsystem<UI>();

        text_ = ui->GetRoot()->CreateChild<Text>();
        text_->SetPosition(20, 20);
        text_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
        text_->SetColor(Color(1.0f, 1.0f, 1.0f));
    }

    if (text.Empty())
        text_->SetText(text);
    else
        text_->SetText(text_->GetText() + "\n" + text);
}

void SampleScene::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera and animate the scene, scale movement with time step
    MoveCamera(timeStep);
}
