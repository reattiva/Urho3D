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

#include "Drawable.h"
#include "VertexBuffer.h"

namespace Urho3D
{

class Texture;
class SpriteSheet;

/// 2D vertex.
struct Vertex2D
{
    /// Position.
    Vector3 position_;
    /// Color.
    unsigned color_;
    /// UV.
    Vector2 uv_;
};

/// 2D sprite component.
class URHO3D_API Sprite2D : public Drawable
{
    OBJECT(Sprite2D);

public:
    /// Construct.
    Sprite2D(Context* context);
    /// Destruct.
    ~Sprite2D();
    /// Register object factory. Drawable must be registered first.
    static void RegisterObject(Context* context);

    /// Apply attribute changes that can not be applied immediately.
    virtual void ApplyAttributes();
    /// Calculate distance and prepare batches for rendering. May be called from worker thread(s), possibly re-entrantly.
    virtual void UpdateBatches(const FrameInfo& frame);
    /// Prepare geometry for rendering. Called from a worker thread if possible (no GPU update.)
    virtual void UpdateGeometry(const FrameInfo& frame);
    /// Return whether a geometry update is necessary, and if it can happen in a worker thread.
    virtual UpdateGeometryType GetUpdateGeometryType();

    /// Set size.
    void SetSize(const Vector2& size);
    /// Set size.
    void SetSize(float w, float h);
    /// Set use texture size.
    void SetUseTextureSize(bool useTextureSize);
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
    /// Set texture.
    void SetTexture(Texture* texture);
    /// Set texture rectangle.
    void SetTextureRect(const IntRect& textureRect);
    /// Set use whole texture.
    void SetUseWholeTexture(bool useWholeTexture);
    /// Set sprite sheet.
    void SetSpriteSheet(SpriteSheet* spriteSheet);
    /// Set sprite name.
    void SetSpriteName(const String& spriteName);
    /// Set sprite from sprite sheet and sprite name.
    void SetSprite(SpriteSheet* spriteSheet, const String& spriteName);
    /// Set material.
    void SetMaterial(Material* material);
    /// Set blend mode.
    void SetBlendMode(BlendMode mode);

    /// Return size.
    const Vector2& GetSize() const { return size_; }
    /// Return use texture size.
    bool GetUseTextureSize() const { return useTextureSize_; }
    /// Return flip X.
    bool GetFlipX() const { return flipX_; }
    /// Return flip Y.
    bool GetFlipY() const { return flipY_; }
    /// Return hot spot for positioning and rotation.
    const Vector2& GetHotSpot() const { return hotSpot_; }
    /// Return color.
    const Color& GetColor() const { return color_; }
    /// Return texture.
    Texture* GetTexture() const;
    /// Return texture rectangle.
    const IntRect& GetTextureRect() const { return textureRect_; }
    /// Return use whole texture.
    bool IsUseWholeTexture() const { return useWholeTexture_; }
    /// Return image set.
    SpriteSheet* GetSpriteSheet() const;
    /// Return image name.
    const String& GetSpriteName() const { return spriteName_; }
    /// Return material.
    Material* GetMaterial() const;
    /// Return blend mode.
    BlendMode GetBlendMode() const { return blendMode_; }

    /// Set texture attribute.
    void SetTextureAttr(ResourceRef value);
    /// Return texture attribute.
    ResourceRef GetTextureAttr() const;
    /// Set image set attribute.
    void SetSpriteSheetAttr(ResourceRef value);
    /// Return image set attribute.
    ResourceRef GetSpriteSheetAttr() const;
    /// Set material attribute.
    void SetMaterialAttr(ResourceRef value);
    /// Return material attribute.
    ResourceRef GetMaterialAttr() const;

protected:
    /// Recalculate the world-space bounding box.
    virtual void OnWorldBoundingBoxUpdate();
    /// Update vertices.
    virtual void UpdateVertices();
    /// Create materials for sprite rendering.
    virtual void UpdateMaterial(bool forceUpdate = false);

    /// Size.
    Vector2 size_;
    /// Use texture size.
    bool useTextureSize_;
    /// Flip X.
    bool flipX_;
    /// Flip Y.
    bool flipY_;
    /// Hot spot.
    Vector2 hotSpot_;
    /// Color.
    Color color_;
    /// Texture.
    SharedPtr<Texture> texture_;
    /// Texture rect.
    IntRect textureRect_;
    /// Use whole texture.
    bool useWholeTexture_;
    /// Sprite sheet.
    SharedPtr<SpriteSheet> spriteSheet_;
    /// Sprite name.
    String spriteName_;
    /// Material.
    SharedPtr<Material> material_;
    /// Used material.
    SharedPtr<Material> usedMaterial_;
    /// Blend mode.
    BlendMode blendMode_;

    /// Vertices.
    Vector<Vertex2D> vertices_;
    /// Geometry.
    SharedPtr<Geometry> geometry_;
    /// Vertex buffer.
    SharedPtr<VertexBuffer> vertexBuffer_;    
    /// Vertices dirty flag.
    bool verticesDirty_;
    /// Material dirty flag.
    bool materialDirty_;
    /// Geometry dirty flag.
    bool geometryDirty_;
};

}

