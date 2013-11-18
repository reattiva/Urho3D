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
#include "Particle2DDemo.h"
#include "ParticleEmitter2D.h"
#include "Renderer.h"
#include "ResourceCache.h"
#include "Scene.h"
#include "SpriteSheet.h"

#include "DebugNew.h"

DEFINE_APPLICATION_MAIN(Particle2DDemo)

Particle2DDemo::Particle2DDemo(Context* context) :
Sample(context)
{    
}

void Particle2DDemo::Start()
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

void Particle2DDemo::CreateScene()
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

    SharedPtr<Node> particleNode(scene_->CreateChild("Particle"));
    ParticleEmitter2D* particle = particleNode->CreateComponent<ParticleEmitter2D>();
    particle->Load("Particle2D/LavaFlow.plist");
    particleNodes.Push(particleNode);
}

void Particle2DDemo::SetupViewport()
{
    Renderer* renderer = GetSubsystem<Renderer>();
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void Particle2DDemo::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, HANDLER(Particle2DDemo, HandleUpdate));
}

void Particle2DDemo::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;
    float timeStep = eventData[P_TIMESTEP].GetFloat();
}
