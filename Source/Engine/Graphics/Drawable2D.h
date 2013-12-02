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

class SpriteFrame;
class SpriteSheet;
class Texture;

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

/// Base class for 2D visible components.
class URHO3D_API Drawable2D : public Drawable
{
    OBJECT(Drawable2D);

public:
    /// Construct.
    Drawable2D(Context* context);
    /// Destruct.
    ~Drawable2D();
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

    /// Set sprite frame with sprite sheet and sprite name.
    void SetSpriteFrame(SpriteSheet* spriteSheet, const String& spriteName);
    /// Set texture.
    void SetTexture(Texture* texture);
    /// Set material.
    void SetMaterial(Material* material);
    /// Set blend mode.
    void SetBlendMode(BlendMode mode);

    /// Return sprite sheet.
    SpriteSheet* GetSpriteSheet() const;
    /// Return sprite name.
    const String& GetSpriteName() const { return spriteName_; }
    /// Return texture.
    Texture* GetTexture() const;
    /// Return material.
    Material* GetMaterial() const;
    /// Return blend mode.
    BlendMode GetBlendMode() const { return blendMode_; }
    /// Return vertices.
    const Vector<Vertex2D>& GetVertices() const { return  vertices_; }

    /// Set sprite sheet attribute.
    void SetSpriteSheetAttr(ResourceRef value);
    /// Return sprite sheet attribute.
    ResourceRef GetSpriteSheetAttr() const;
    /// Set sprite name.
    void SetSpriteName(const String& spriteName);
    /// Set texture attribute.
    void SetTextureAttr(ResourceRef value);
    /// Return texture attribute.
    ResourceRef GetTextureAttr() const;
    /// Set material attribute.
    void SetMaterialAttr(ResourceRef value);
    /// Return material attribute.
    ResourceRef GetMaterialAttr() const;

protected:
    /// Recalculate the world-space bounding box.
    virtual void OnWorldBoundingBoxUpdate();
    /// Update vertices.
    virtual void UpdateVertices() = 0;
    /// Create materials for sprite rendering.
    virtual void UpdateMaterial(bool forceUpdate = false);

    /// Sprite sheet.
    SharedPtr<SpriteSheet> spriteSheet_;
    /// Sprite name.
    String spriteName_;
    /// Sprite frame.
    const SpriteFrame* spriteFrame_;
    /// Texture.
    SharedPtr<Texture> texture_;
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
    /// Geometry dirty flag.
    bool geometryDirty_;
    /// Material dirty flag.
    bool materialDirty_;    
};

}

