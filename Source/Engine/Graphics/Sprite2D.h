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

#pragma once

#include "Drawable2D.h"

namespace Urho3D
{

/// 2D sprite component.
class URHO3D_API Sprite2D : public Drawable2D
{
    OBJECT(Sprite2D);

public:
    /// Construct.
    Sprite2D(Context* context);
    /// Destruct.
    ~Sprite2D();
    /// Register object factory. Drawable2D must be registered first.
    static void RegisterObject(Context* context);

    /// Set Unit per pixel.
    void SetUnitPerPixel(float unitPerPixel);
    /// Set flip X.
    void SetFilpX(bool flipX);
    /// Set flip Y.
    void SetFlipY(bool flipY);
    /// Set hot spot for positioning and rotation.
    void SetHotSpot(const Vector2& hotSpot);
    /// Set hot spot for positioning and rotation.
    void SetHotSpot(float x, float y);
    /// Set color.
    void SetColor(const Color& color);

    /// Return size.
    float GetUnitPerPixel() const { return unitPerPixel_; }
    /// Return flip X.
    bool GetFlipX() const { return flipX_; }
    /// Return flip Y.
    bool GetFlipY() const { return flipY_; }
    /// Return hot spot for positioning and rotation.
    const Vector2& GetHotSpot() const { return hotSpot_; }
    /// Return color.
    const Color& GetColor() const { return color_; }

protected:
    /// Update vertices.
    virtual void UpdateVertices();
    /// Build quad vertices.
    void BuildQuadVertices(Vertex2D& vertex0, Vertex2D& vertex1, Vertex2D& vertex2, Vertex2D& vertex3, Texture* texture);
    /// Build quad vertices.
    void BuildQuadVertices(Vertex2D& vertex0, Vertex2D& vertex1, Vertex2D& vertex2, Vertex2D& vertex3, Texture* texture, const SpriteFrame* spriteFrame);

    /// Unit per pixel.
    float unitPerPixel_;
    /// Flip X.
    bool flipX_;
    /// Flip Y.
    bool flipY_;
    /// Hot spot.
    Vector2 hotSpot_;
    /// Color.
    Color color_;
};

}

