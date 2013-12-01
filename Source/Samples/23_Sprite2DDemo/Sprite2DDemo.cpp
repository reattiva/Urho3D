//
// Copyright (c) 2008-2013 the Urho3D project.
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

#include "Camera.h"
#include "CoreEvents.h"
#include "Engine.h"
#include "Graphics.h"
#include "Octree.h"
#include "Renderer.h"
#include "ResourceCache.h"
#include "Scene.h"
#include "Sprite2D.h"
#include "Sprite2DDemo.h"
#include "SpriteSheet.h"

#include "DebugNew.h"

DEFINE_APPLICATION_MAIN(Sprite2DDemo)

Sprite2DDemo::Sprite2DDemo(Context* context) :
Sample(context)
{    
}

void Sprite2DDemo::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();
}

void Sprite2DDemo::CreateScene()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>();

    Graphics* graphics = GetSubsystem<Graphics>();
    float width = (float)graphics->GetWidth();
    float height = (float)graphics->GetHeight();

    // Create camera
    cameraNode_ = scene_->CreateChild("Camera");
    Camera* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetOrthographic(true);
    camera->SetOrthoSize(Vector2(width, height));

    SpriteSheet* spriteSheet = cache->GetResource<SpriteSheet>("SpriteSheets/spineboy.xml");
    if (!spriteSheet)
        return;

    const Vector<String>& spriteNames = spriteSheet->GetAllSpriteNames();
    for (unsigned i = 0; i < 50; ++i)
    {
        SharedPtr<Node> spriteNode(scene_->CreateChild("Sprite"));

        Sprite2D* sprite2D = spriteNode->CreateComponent<Sprite2D>();
        sprite2D->SetSpriteFrame(spriteSheet, spriteNames[i % spriteNames.Size()]);
        sprite2D->SetBlendMode(BLEND_ALPHA);
        
        Vector3 moveSpeed(Random(400.0f) - 200.0f, Random(400.0f) - 200.0f, 0.0f);
        spriteNode->SetVar("MoveSpeed", moveSpeed);        
        spriteNode->SetRotation(Quaternion(Atan2(moveSpeed.y_, moveSpeed.x_), Vector3::FORWARD));

        spriteNodes_.Push(spriteNode);
    }
}

void Sprite2DDemo::SetupViewport()
{
    Renderer* renderer = GetSubsystem<Renderer>();
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void Sprite2DDemo::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, HANDLER(Sprite2DDemo, HandleUpdate));
}

void Sprite2DDemo::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    Graphics* graphics = GetSubsystem<Graphics>();
    float xRange = (float)graphics->GetWidth() / 2.0f;
    float yRange = (float)graphics->GetHeight() / 2.0f;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();
    
    for (unsigned i = 0; i < spriteNodes_.Size(); ++i)
    {
        SharedPtr<Node> spriteNode = spriteNodes_[i];

        Vector3 moveSpeed = spriteNode->GetVar("MoveSpeed").GetVector3();
        Vector3 position = spriteNode->GetPosition() + moveSpeed * timeStep;
        if (position.x_ < -xRange || position.x_ > xRange)
        {
            moveSpeed.x_ = -moveSpeed.x_;
            position = spriteNode->GetPosition() + moveSpeed * timeStep;
            spriteNode->SetVar("MoveSpeed", moveSpeed);
            spriteNode->SetRotation(Quaternion(Atan2(moveSpeed.y_, moveSpeed.x_), Vector3::FORWARD));
        }

        if (position.y_ < -yRange || position.y_ > yRange)
        {
            moveSpeed.y_ = -moveSpeed.y_;
            position = spriteNode->GetPosition() + moveSpeed * timeStep;
            spriteNode->SetVar("MoveSpeed", moveSpeed);
            spriteNode->SetRotation(Quaternion(Atan2(moveSpeed.y_, moveSpeed.x_), Vector3::FORWARD));
        }

        spriteNode->SetPosition(position);
    }
}
