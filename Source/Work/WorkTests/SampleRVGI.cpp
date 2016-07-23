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
// Based on "Rasterized Voxel-based Dynamic Global Illumination" by Hawar Doghramachi:
// http://hd-prg.com/RVGlobalIllumination.html
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
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>

#include "SampleRVGI.h"

#include <Urho3D/DebugNew.h>
#include <Urho3D/IO/Log.h>

// Path of the example data folder (relative to the bin folder)
#define RESOURCE_PATH "../../Source/Work/WorkTests/Data/RVGI"

float GRID_SIZE = 10.0f;//8.0f;
const unsigned GRID_CELLS = 32;

SampleRVGI::SampleRVGI(Context* context) :
    Sample(context),
    drawDebug_(false),
    text_(0),
    gridVisCommand_(0),
    gridFillCommand_(0),
    gridLightCommand_(0),
    gridLightPropagate_(0),
    gridFinal_(0)
{
}

void SampleRVGI::Start()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    cache->AddResourceDir(RESOURCE_PATH);

    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Setup the viewport for displaying the scene
    SetupViewport();

    InitGrid();

    // Hook up to the frame update and render post-update events
    SubscribeToEvents();
}

void SampleRVGI::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(SampleRVGI, HandleUpdate));

    // Subscribe HandlePostRenderUpdate() function for processing the post-render update event, during which we request
    // debug geometry
    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(SampleRVGI, HandlePostRenderUpdate));
}

void SampleRVGI::Log(const String& text)
{
    if (!text_)
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();
        UI* ui = GetSubsystem<UI>();

        text_ = ui->GetRoot()->CreateChild<Text>();
        text_->SetPosition(20, 20);
        text_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
        text_->SetColor(Color(1.0f, 0.0f, 0.0f));
    }

    if (text.Empty())
        text_->SetText(text);
    else
        text_->SetText(text_->GetText() + "\n" + text);
}

void SampleRVGI::CreateScene()
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
    cameraNode_->SetPosition(Vector3(0.0f, 1.0f, -3.0f));
    cameraNode_->SetPosition(Vector3(-2.0f, 1.0f, 0.0f));

    // Create a Zone component for ambient lighting & fog control
    Node* zoneNode = scene_->CreateChild("Zone");
    Zone* zone = zoneNode->CreateComponent<Zone>();
    //zone->SetAmbientColor(Color(0.001f, 0.001f, 0.001f));
    zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
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
    light->SetShadowCascade(CascadeParameters(20.0f, 50.0f, 200.0f, 0.0f, 0.8f));

    if (0)
    {
        Node* floorNode = scene_->CreateChild("Floor");
        floorNode->SetScale(Vector3(20.0f, 1.0f, 20.0f));
        floorNode->SetPosition(Vector3(0.0f, -0.5f, 0.0f));
    #if 0
        floorNode->SetPosition(Vector3(0.0f, 0.0f, 5.0f));
        floorNode->SetRotation(Quaternion(-90.0f, 0.0f, 0.0f));
    #endif
        StaticModel* floorObject = floorNode->CreateComponent<StaticModel>();
        floorObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        floorObject->SetMaterial(cache->GetResource<Material>("Materials/Terrain.xml"));
        MaterialAddGridFillPass(floorObject->GetMaterial(), "TERRAIN");

        Node* boxNode1 = scene_->CreateChild("Box");
        boxNode1->SetPosition(Vector3(0.0f, 0.5f, 5.0f));
        boxNode1->SetRotation(Quaternion(0.0f, 45.0f, 0.0f));
        boxNode1->SetScale(1.0f);
        StaticModel* boxObject1 = boxNode1->CreateComponent<StaticModel>();
        boxObject1->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        boxObject1->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
        boxObject1->SetCastShadows(true);
        boxObject1->SetOccluder(true);
        MaterialAddGridFillPass(boxObject1->GetMaterial());

        Node* boxNode2 = boxNode1->Clone();
        boxNode2->SetPosition(Vector3(-3.5f, 0.5f, 5.0f));
        boxNode2->SetRotation(Quaternion(0.0f, 0.0f, 0.0f));
        StaticModel* boxObject2 = boxNode2->GetComponent<StaticModel>();
        boxObject2->SetMaterial(cache->GetResource<Material>("Materials/Mushroom.xml"));
        MaterialAddGridFillPass(boxObject2->GetMaterial());

        Node* colorNode1 = scene_->CreateChild("CubeColor");
        colorNode1->SetPosition(Vector3(1.5f, 3.0f, 5.0f));
        colorNode1->SetRotation(Quaternion(0.0f, 0.0f, 0.0f));
        colorNode1->SetScale(3.0f);
        StaticModel* colorObject1 = colorNode1->CreateComponent<StaticModel>();
        colorObject1->SetModel(cache->GetResource<Model>("Models/CubeColor.mdl"));
        colorObject1->SetMaterial(cache->GetResource<Material>("Materials/CubeColor.xml"));
        colorObject1->SetCastShadows(true);
        colorObject1->SetOccluder(true);

        Node* colorNode2 = colorNode1->Clone();
        colorNode2->SetPosition(Vector3(-7.0f, 2.5f, 5.0f));
        colorNode2->SetRotation(Quaternion(70.0f, 45.0f, 0.0f));

        Node* ninjaNode = scene_->CreateChild("Ninja");
        ninjaNode->SetPosition(Vector3(-1.0f, 0.0f, 4.0f));
        ninjaNode->SetRotation(Quaternion(0.0f, 180.0f, 0.0f));
        StaticModel* ninjaObject = ninjaNode->CreateComponent<StaticModel>();
        ninjaObject->SetModel(cache->GetResource<Model>("Models/NinjaSnowWar/Ninja.mdl"));
        Material* ninjaMaterial = cache->GetResource<Material>("Materials/NinjaSnowWar/Ninja.xml");
        ninjaObject->SetMaterial(ninjaMaterial);
        MaterialAddGridFillPass(ninjaMaterial);
    }

    if (0)
    {
        const float SIZE = 12.0f;
        const float HEIGHT = SIZE/2.0f;
        const float THICK = 1.0f;

        Material* materialRed = cache->GetResource<Material>("Materials/DefaultGrey.xml");
        assert(materialRed);
        materialRed->SetShaderParameter("MatDiffColor", Color(1.0f, 0.0f, 0.0f, 1.0f));
        SharedPtr<Material> materialBlue = materialRed->Clone();
        materialBlue->SetShaderParameter("MatDiffColor", Color(0.0f, 0.0f, 1.0f, 1.0f));
        SharedPtr<Material> materialWhite = materialRed->Clone();
        materialWhite->SetShaderParameter("MatDiffColor", Color(0.6f, 0.6f, 0.6f, 1.0f));
        SharedPtr<Material> materialYellow = materialRed->Clone();
        materialYellow->SetShaderParameter("MatDiffColor", Color(1.0f, 1.0f, 0.0f, 1.0f));

        MaterialAddGridFillPass(materialRed);
        MaterialAddGridFillPass(materialBlue);
        MaterialAddGridFillPass(materialWhite);
        MaterialAddGridFillPass(materialYellow);

        Node* floorNode = scene_->CreateChild("Box");
        floorNode->SetPosition(Vector3(0.0f, -THICK/2.0f, 0.0f));
        floorNode->SetScale(Vector3(SIZE, THICK, SIZE));
        StaticModel* floorObject = floorNode->CreateComponent<StaticModel>();
        floorObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        floorObject->SetMaterial(materialWhite);
        floorObject->SetCastShadows(true);
        floorObject->SetOccluder(true);

        Node* wallNodeUp = floorNode->Clone();
        wallNodeUp->SetPosition(Vector3(0.0f, HEIGHT/2.0f, (SIZE-THICK)/2.0f));
        wallNodeUp->SetScale(Vector3(SIZE, HEIGHT, THICK));
        wallNodeUp->GetComponent<StaticModel>()->SetMaterial(materialWhite);

        Node* wallNodeLeft = floorNode->Clone();
        wallNodeLeft->SetPosition(Vector3(-(SIZE-THICK)/2.0f, HEIGHT/2.0f, 0.0f));
        wallNodeLeft->SetScale(Vector3(THICK, HEIGHT, SIZE));
        wallNodeLeft->GetComponent<StaticModel>()->SetMaterial(materialWhite);

        Node* wallNodeRight = wallNodeLeft->Clone();
        wallNodeRight->SetPosition(Vector3((SIZE-THICK)/2.0f, HEIGHT/2.0f, 0.0f));
        wallNodeRight->GetComponent<StaticModel>()->SetMaterial(materialBlue);

        Node* boxNode1 = floorNode->Clone();
        boxNode1->SetPosition(Vector3(-SIZE/5.0f, SIZE/6.0f, SIZE/5.0f));
        boxNode1->SetScale(Vector3(SIZE/4.0f, HEIGHT*0.8f, SIZE/4.0f));
        boxNode1->SetRotation(Quaternion(0.0f, -15.0f, 0.0f));
        boxNode1->GetComponent<StaticModel>()->SetMaterial(materialRed);

        Node* boxNode2 = floorNode->Clone();
        boxNode2->SetPosition(Vector3(SIZE/8.0f, SIZE/10.0f, -SIZE/8.0f));
        boxNode2->SetScale(SIZE/5.0f);
        boxNode2->SetRotation(Quaternion(0.0f, 15.0f, 0.0f));
        boxNode2->GetComponent<StaticModel>()->SetMaterial(materialYellow);
    }

    if (1)
    {
        const float HALFX = 4.0f;
        const float HALFY = 1.0f;
        const float THICK = 0.8f;

        Node* boxNode1 = scene_->CreateChild("Box");
        boxNode1->SetPosition(Vector3(2*HALFX, HALFY, 2*HALFX));
        boxNode1->SetScale(1.0f);
        StaticModel* boxObject1 = boxNode1->CreateComponent<StaticModel>();
        boxObject1->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        boxObject1->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
        boxObject1->SetCastShadows(true);
        boxObject1->SetOccluder(true);
        MaterialAddGridFillPass(boxObject1->GetMaterial());

        // Floor
        Node* wallNode = scene_->CreateChild("Wall");
        wallNode->SetScale(Vector3(6*HALFX, THICK, 6*HALFX));
        StaticModel* wallObject = wallNode->CreateComponent<StaticModel>();
        wallObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        //wallObject->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
        wallObject->SetMaterial(cache->GetResource<Material>("Materials/NinjaSnowWar/Snow.xml"));
        wallObject->SetCastShadows(true);
        wallObject->SetOccluder(true);
        MaterialAddGridFillPass(wallObject->GetMaterial());

        // Roof
        Node* wallNode2 = wallNode->Clone();
        wallNode2->SetPosition(Vector3(0.0f, 2*HALFY+THICK*0.5f, 0.0f));
        wallNode2->SetScale(Vector3(2.0f*HALFX, THICK, 2.0f*HALFX));

        // Left wall
        Node* wallNode3 = wallNode->Clone();
        wallNode3->SetScale(Vector3(THICK, 2*HALFY, 1.99f*HALFX));
        wallNode3->SetPosition(Vector3(-HALFX+THICK*0.5f, HALFY, 0.0f));

        // Right wall
        Node* wallNode4 = wallNode3->Clone();
        wallNode4->SetPosition(Vector3(HALFX-THICK*0.5f, HALFY, 0.0f));

        // Back wall
        Node* wallNode5 = wallNode3->Clone();
        wallNode5->SetRotation(Quaternion(0.0f, 90.0f, 0.0f));
        wallNode5->SetPosition(Vector3(0.0f, HALFY, -HALFX+THICK*0.5f));

        // Front wall
        Node* wallNode6 = wallNode5->Clone();
        wallNode6->SetScale(Vector3(THICK, 2*HALFY, HALFX));
        wallNode6->SetPosition(Vector3(0.0f, HALFY, HALFX-THICK*0.5f));

    }
}

void SampleRVGI::MaterialAddGridFillPass(Material* material, const String& defines)
{
    String gridDefines = " "+defines+" HALFCELLS="+String(GRID_CELLS/2);
    //gridDefines += " BLOAT";

    const Vector<TechniqueEntry>& techniques = material->GetTechniques();
    for (unsigned i=0; i < techniques.Size(); ++i)
    {
        // Note that different materials can share the same techniques
        // (e.g. Mushroom and Ninja)
        Technique* technique = techniques[i].technique_;
        Pass* customPass = technique->CreatePass("gridfill");
        customPass->SetVertexShader("GridFill");
        customPass->SetPixelShader("GridFill");
        customPass->SetGeometryShader("GridFill");
        customPass->SetPixelShaderDefines(technique->GetPass("base")->GetPixelShaderDefines() + gridDefines);

        customPass->SetCullMode(CULL_NONE); // Default: CULL_CCW
        customPass->SetDepthTestMode(CMP_DISABLED);
        customPass->SetDepthWrite(false);

        gridFillPasses_.Insert(customPass);
    }
}

void SampleRVGI::SetupViewport()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

#if 1
    XMLFile* renderPathXml = cache->GetResource<XMLFile>("RenderPaths/RVGI.xml");
#else
    XMLFile* renderPathXml = cache->GetResource<XMLFile>("RenderPaths/Deferred.xml");
#endif
    RenderPath* renderPath = new RenderPath();
    renderPath->Load(renderPathXml);

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>(), renderPath));

    Renderer* renderer = GetSubsystem<Renderer>();
    renderer->SetViewport(0, viewport);

    String& gridDefines = " HALFCELLS="+String(GRID_CELLS/2);

    for (unsigned i = 0; i < renderPath->GetNumCommands(); ++i)
    {
        RenderPathCommand* command = renderPath->GetCommand(i);

        if (command->tag_.Contains("Grid"))
            command->pixelShaderDefines_ += gridDefines;

        if (command->computeShaderName_.Contains("Grid"))
        {
            command->computeShaderDefines_ += gridDefines;
            for (unsigned i = 0; i < 3; ++i)
                command->computeGroups_[i] = GRID_CELLS/4;
        }
        else if (command->tag_ == "GridFill")
        {
            gridFillCommand_ = command;
        }
        else if (command->tag_ == "GridVis")
        {
            gridVisCommand_ = command;
        }
        else if (command->tag_ == "GridLights")
        {
            gridLightCommand_ = command;
            command->instances_ = GRID_CELLS;
        }
        else if (command->tag_ == "GridFinal")
        {
            gridFinal_ = command;
        }
    }

    if (!gridFillCommand_)
        URHO3D_LOGERROR("GridFill command not found");
    if (!gridVisCommand_)
        URHO3D_LOGERROR("GridVis command not found");

    for (unsigned i = 0; i < renderPath->renderTargets_.Size(); ++i)
    {
        const RenderTargetInfo& renderTarget = renderPath->renderTargets_[i];
        if (renderTarget.layers_ > 1 && (renderTarget.layers_ != GRID_CELLS ||
                renderTarget.size_.x_ != GRID_CELLS || renderTarget.size_.y_ != GRID_CELLS))
            URHO3D_LOGERRORF("RenderTarget %s does not have size %d", renderTarget.name_.CString(), GRID_CELLS);
    }
}

void SampleRVGI::InitGrid()
{
    // Create the grid buffer
    bufferIn0_ = new ShaderBuffer(context_, ShaderBuffer::BUFFER_STRUCTURED |
                                  ShaderBuffer::BUFFER_READ | ShaderBuffer::BUFFER_WRITE);

    struct Voxel
    {
        unsigned colorOcclusionMask;
        unsigned normalMasks[4];
    };

    unsigned elemSize = sizeof(Voxel);
    unsigned elemCount = GRID_CELLS * GRID_CELLS * GRID_CELLS;
    bufferIn0_->SetSize(elemSize, elemCount);

    // Link this buffer to the name used in the shaders
    Graphics* graphics = GetSubsystem<Graphics>();
    graphics->AddShaderBuffer("BufferGrid", bufferIn0_);

    // Populate the buffer with some colors for testing
    Vector<Color> colors;
    colors.Push(Color::BLUE);
    colors.Push(Color::RED);
    colors.Push(Color::YELLOW);
    colors.Push(Color::WHITE);
    colors.Push(Color::GRAY);
    colors.Push(Color::BLACK);

    for (unsigned i=0; i<elemCount; ++i)
    {
        Color c = colors[i % colors.Size()] * 255.0f;
        unsigned ci = (unsigned)c.r_ << 16 | (unsigned)c.g_ << 8 | (unsigned)c.b_;

        Voxel v;
        v.colorOcclusionMask = ci;
        v.normalMasks[0] = i+0;
        v.normalMasks[1] = i+0;
        v.normalMasks[2] = i+0;
        v.normalMasks[3] = i+0;

        bufferIn0_->SetElements(i, 1, &v);
    }
    bufferIn0_->Apply();

    // Init shader uniforms
    customUB_.gridCellSize.x_ = GRID_SIZE / GRID_CELLS;
    customUB_.gridCellSize.y_ = 1 / customUB_.gridCellSize.x_;
    if (gridFillCommand_)
        gridFillCommand_->SetShaderParameter("GridCellSize", customUB_.gridCellSize);

    // Init precalculated view-proj matrices
    Camera gridCamera(context_);
    gridCamera.SetOrthographic(true);
    gridCamera.SetOrthoSize(GRID_SIZE);
    gridCamera.SetFarClip(GRID_SIZE);

    // The viewProj matrix is:
    //  viewProj = proj * view
    // The view matrix is:
    //  view = Inverse( Matrix(cameraPos, cameraRot) )
    // In Urho the matrix is built as rotation then translation:
    //  view = Inverse( Matrix(cameraPos) * Matrix(cameraRot) )
    // For 2 invertible matrices A and B we know that: inv(A * B) = inv(B) * inv(A):
    //  view = Inverse(Matrix(cameraRot)) * Inverse(Matrix(cameraPos))
    //  view = Matrix(-cameraAngle) * Matrix(-cameraPos)
    // In this way we can precalculate: proj * Matrix(-cameraAngle)

    Matrix4 proj = gridCamera.GetProjection();
    // Front view matrix
    gridMatrices_[0] = proj;
    //Matrix3 mF = Quaternion(180.0f, 0.0f, 0.0f).RotationMatrix();
    //gridMatrices_[0] = proj * Matrix4(mF);
    // Left view matrix
    Matrix3 mRL = Quaternion(0.0f, -90.0f, 0.0f).RotationMatrix();
    gridMatrices_[1] = proj * Matrix4(mRL);
    // Bottom view matrix
    Matrix3 mTB = Quaternion(90.0f, 0.0f, 0.0f).RotationMatrix();
    gridMatrices_[2] = proj * Matrix4(mTB);
}

void SampleRVGI::UpdateGrid()
{
    Vector3 cameraPos = cameraNode_->GetPosition();
    Vector3 cameraDir = cameraNode_->GetDirection();
    Vector3 gridPos = cameraPos + cameraDir * GRID_SIZE * 0.5f;
    //Vector3 gridPos = Vector3(-3.0f, 4.0f, 6.0f);

    Vector3 gridSnapped = gridPos * customUB_.gridCellSize.y_;
    gridSnapped = Vector3(floor(gridSnapped.x_), floor(gridSnapped.y_), floor(gridSnapped.z_));
    gridSnapped = gridSnapped * customUB_.gridCellSize.x_;
    Vector4 newGridSnappedPosition = Vector4(gridSnapped, 1.0f);

    if (customUB_.dirty || !newGridSnappedPosition.Equals(customUB_.gridSnappedPosition))
    {
        customUB_.dirty = false;

        customUB_.gridSnappedPosition = newGridSnappedPosition;

        Matrix4 t;
        // Front view
        t.SetTranslation(- gridSnapped + Vector3(0.0f, 0.0f, GRID_SIZE * 0.5f));
        customUB_.gridViewProjMatrices[0] = gridMatrices_[0] * t;
        // Left view
        t.SetTranslation(- gridSnapped + Vector3(GRID_SIZE * 0.5f, 0.0f, 0.0f));
        customUB_.gridViewProjMatrices[1] = gridMatrices_[1] * t;
        // Bottom view
        t.SetTranslation(- gridSnapped + Vector3(0.0f, GRID_SIZE * 0.5f, 0.0f));
        customUB_.gridViewProjMatrices[2] = gridMatrices_[2] * t;

#if 0
        // Front view
        Matrix3x4 t0 = Matrix3x4(gridSnapped - Vector3(0.0f, 0.0f, GRID_SIZE * 0.5f),
                                 Quaternion(),
                                 1.0f).Inverse();
        //customUB_.gridViewProjMatrices[0] = gridMatrices_[0] * t0;
        assert( customUB_.gridViewProjMatrices[0].Equals(gridMatrices_[0] * t0) );

        // Left view
        Matrix3x4 t1 = Matrix3x4(gridSnapped - Vector3(GRID_SIZE * 0.5f, 0.0f, 0.0f),
                                 Quaternion(0.0f, 90.0f, 0.0f),
                                 1.0f).Inverse();
        //customUB_.gridViewProjMatrices[1] = gridMatrices_[0] * t1;
        assert( customUB_.gridViewProjMatrices[1].Equals(gridMatrices_[0] * t1) );

        // Bottom view
        Matrix3x4 t2 = Matrix3x4(gridSnapped - Vector3(0.0f, GRID_SIZE * 0.5f, 0.0f),
                                 Quaternion(-90.0f, 0.0f, 0.0f),
                                 1.0f).Inverse();
        //customUB_.gridViewProjMatrices[2] = gridMatrices_[0] * t2;
        assert( customUB_.gridViewProjMatrices[2].Equals(gridMatrices_[0] * t2) );
#endif

        if (gridVisCommand_)
        {
            gridVisCommand_->SetShaderParameter("GridCellSize", customUB_.gridCellSize);
            gridVisCommand_->SetShaderParameter("GridSnappedPosition", customUB_.gridSnappedPosition);
        }
        if (gridFillCommand_)
        {
            gridFillCommand_->SetShaderParameter("GridCellSize", customUB_.gridCellSize);
            gridFillCommand_->SetShaderParameter("GridSnappedPosition", customUB_.gridSnappedPosition);
            gridFillCommand_->SetShaderParameter("GridViewProjMatrix0", customUB_.gridViewProjMatrices[0]);
            gridFillCommand_->SetShaderParameter("GridViewProjMatrix1", customUB_.gridViewProjMatrices[1]);
            gridFillCommand_->SetShaderParameter("GridViewProjMatrix2", customUB_.gridViewProjMatrices[2]);
            gridFillCommand_->SetShaderParameter("GlobalIllumParams", customUB_.globalIllumParams);
            gridFillCommand_->SetShaderParameter("Factor", customUB_.factor);
            customUB_.set = 10;
        }
    }

    Log();
#if 0
    Log("GridCellSize.x = " + String(customUB_.gridCellSize.x_));
    Log("GridCellSize.y = " + String(customUB_.gridCellSize.y_));
    Log("GridSnappedPosition.x = " + String(customUB_.gridSnappedPosition.x_));
    Log("GridSnappedPosition.y = " + String(customUB_.gridSnappedPosition.y_));
    Log("GridSnappedPosition.z = " + String(customUB_.gridSnappedPosition.z_));
    Log("GlobalIllumParams = " + customUB_.globalIllumParams.ToString());
    Log("Factor = " + String(customUB_.factor));
    Log("" + debugMatrix.ToString());
#else
    Log("--- Urho global illumination sample ---"
        "\n[ALT] invert key function, [SHIFT] slow key function"
        "\n[R][F] rotate directional light"
        "\n[X] flux amplifier " + String(customUB_.globalIllumParams.x_) +
        "\n[C] occlusion amplifier " + String(customUB_.globalIllumParams.y_) +
        "\n[V] GI diffuse power " + String(customUB_.globalIllumParams.z_)
        );
#endif

    if (customUB_.set)
    {
        --customUB_.set;
        Log("SET");
    }
}

void SampleRVGI::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    Input* input = GetSubsystem<Input>();

    // Movement speed as world units per second
    float MOVE_SPEED = 3.0f;
    if (input->GetQualifierDown(QUAL_SHIFT))
        MOVE_SPEED *= 0.1f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Save camera position and rotaion
    //Quaternion oldCameraRot = cameraNode_->GetRotation();
    //Vector3 oldCameraPos = cameraNode_->GetPosition();

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

    float factor = 1.0f;
    if (input->GetQualifierDown(QUAL_SHIFT))
        factor /= 10.0f;
    if (input->GetQualifierDown(QUAL_ALT))
        factor = -factor;

    if (input->GetKeyDown('Z'))
    {
        customUB_.dirty = true;
        customUB_.factor += factor * 0.01f;
    }
    if (input->GetKeyDown('X'))
    {
        customUB_.dirty = true;
        customUB_.globalIllumParams.x_ += factor * 0.005f;
    }
    if (input->GetKeyDown('C'))
    {
        customUB_.dirty = true;
        customUB_.globalIllumParams.y_ += factor * 0.005f;
    }
    if (input->GetKeyDown('V'))
    {
        customUB_.dirty = true;
        customUB_.globalIllumParams.z_ += factor * 0.005f;
    }
    if (input->GetKeyDown('R') || input->GetKeyDown('F'))
    {
        AnimateScene(factor * 0.005f, input->GetKeyDown('F'));
    }

    if (input->GetKeyPress(KEY_T) && gridLightCommand_)
    {
        String& defines = gridLightCommand_->pixelShaderDefines_;
        if (defines.Contains(" TOGGLE "))
            defines.Replace(" TOGGLE ", String());
        else
            defines += " TOGGLE ";
    }

}

void SampleRVGI::AnimateScene(float timeStep, bool aroundX)
{
    // Get the light and billboard scene nodes
    PODVector<Node*> lightNodes;
    scene_->GetChildrenWithComponent<Light>(lightNodes);

    const float LIGHT_ROTATION_SPEED = 80.0f;

    // Rotate the lights around the world Y-axis
    for (unsigned i = 0; i < lightNodes.Size(); ++i)
        if (aroundX)
            lightNodes[i]->Rotate(Quaternion(LIGHT_ROTATION_SPEED * timeStep, 0.0f, 0.0f), TS_LOCAL);
        else
            lightNodes[i]->Rotate(Quaternion(0.0f, LIGHT_ROTATION_SPEED * timeStep, 0.0f), TS_WORLD);
}

void SampleRVGI::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera and animate the scene, scale movement with time step
    MoveCamera(timeStep);

    //AnimateScene(timeStep/1.0f);

    UpdateGrid();
}

void SampleRVGI::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    // If draw debug mode is enabled, draw viewport debug geometry. This time use depth test, as otherwise the result becomes
    // hard to interpret due to large object count
    if (drawDebug_)
        GetSubsystem<Renderer>()->DrawDebugGeometry(true);
}
