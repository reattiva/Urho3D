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

#include "Sample.h"

namespace Urho3D
{
class Node;
class Scene;
class Text;
class ShaderBuffer;
}

/// Example on how to write (RWStructuredBuffer) and read (StructuredBuffer) a buffer
/// in the pixel shader, and how to unbind a shader view.
class SampleRWBUF : public Sample
{
    URHO3D_OBJECT(SampleRWBUF, Sample)

public:
    /// Construct.
    SampleRWBUF(Context* context);

    /// Setup after engine initialization and before running the main loop.
    virtual void Start();

private:
    void Init();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();

    SharedPtr<ShaderBuffer> dataBuffer_;
};
