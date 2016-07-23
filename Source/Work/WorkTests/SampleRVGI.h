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

#pragma once

#include "Sample.h"
#include "Urho3D/Math/Matrix4.h"

namespace Urho3D
{
class Node;
class Scene;
class ShaderBuffer;
struct RenderPathCommand;
class Material;
class Pass;
}

/// Compute shader example.
class SampleRVGI : public Sample
{
    URHO3D_OBJECT(SampleRVGI, Sample)

public:
    /// Construct.
    SampleRVGI(Context* context);

    /// Setup after engine initialization and before running the main loop.
    virtual void Start();

private:
    /// Construct the scene content.
    void CreateScene();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();
    /// Subscribe to application-wide logic update and post-render update events.
    void SubscribeToEvents();
    /// Read input and moves the camera.
    void MoveCamera(float timeStep);
    /// Animate the scene.
    void AnimateScene(float timeStep, bool aroundX);
    /// Handle the logic update event.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle the post-render update event.
    void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);

    void Log(const String &text = String());

    void MaterialAddGridFillPass(Material* material, const String& defines = String::EMPTY);
    void InitGrid();
    void UpdateGrid();

    /// Flag for drawing debug geometry.
    bool drawDebug_;
    /// Log text element
    Text* text_;

    SharedPtr<ShaderBuffer> bufferIn0_;

    RenderPathCommand* gridVisCommand_;
    RenderPathCommand* gridFillCommand_;
    RenderPathCommand* gridLightCommand_;
    RenderPathCommand* gridLightPropagateCommand_;

    HashSet<Pass*> gridFillPasses_;

    Matrix4 gridMatrices_[3];

    struct CustomUB
    {
        CustomUB() :
            gridCellSize(1.0f, 1.0f),
            gridSnappedPosition(0.0f, 0.0f, 0.0f, 0.0f),
            //globalIllumParams(2.55f, 1.25f, 0.45f, 0.0f),
            globalIllumParams(3.5f, 4.0f, 1.2f, 0.0f),
            factors(0.0f, 0.0f, 0.0f, 0.0f),
            repeats(3),
            distance(0.5),
            dirty(true),
            set(0)
        {}

        // front, left, bottom view matrices
        Matrix4 gridViewProjMatrices[3];
        // x = grid cell size, y = 1/x
        Vector2 gridCellSize;
        // x,y,z = grid position, w=1
        Vector4 gridSnappedPosition;
        // x=flux amplifier, y=occlusion amplifier, z=diffuse power, w=propagate steps
        Vector4 globalIllumParams;
        // x=shadow radius, y=shadow offset
        Vector4 factors;
        // propagation steps
        unsigned repeats;
        // camera distance from the grid center
        float distance;

        bool dirty;
        unsigned set;
    } customUB_;
};
