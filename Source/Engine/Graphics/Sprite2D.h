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
#include "GraphicsDefs.h"
#include "VertexBuffer.h"

namespace Urho3D
{

class ImageSet;

/// Vertex2D.
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
    void SetTextureRect(const IntRect& rect);
    /// Set use whole texture.
    void SetUseWholeTexture(bool useWholeTexture);
    /// Set image set.
    void SetImageSet(ImageSet* imageSet);
    /// Set image name.
    void SetImageName(const String& imageName);
    /// Set image from image set and image name.
    bool SetImage(ImageSet* imageSet, const String& imageName);
    /// Set material.
    void SetMaterial(Material* material);
    /// Set blend mode.
    void SetBlendMode(BlendMode mode);

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
    ImageSet* GetImageset() const;
    /// Return image name.
    const String& GetImageName() const { return imageName_; }
    /// Return material.
    Material* GetMaterial() const;
    /// Return blend mode.
    BlendMode GetBlendMode() const { return blendMode_; }

    /// Set texture attribute.
    void SetTextureAttr(ResourceRef value);
    /// Return texture attribute.
    ResourceRef GetTextureAttr() const;
    /// Set image set attribute.
    void SetImagesetAttr(ResourceRef value);
    /// Return image set attribute.
    ResourceRef GetImagesetAttr() const;
    /// Set material attribute.
    void SetMaterialAttr(ResourceRef value);
    /// Return material attribute.
    ResourceRef GetMaterialAttr() const;

protected:
    /// Recalculate the world-space bounding box.
    virtual void OnWorldBoundingBoxUpdate();
    
private:
    /// Mark sprite & geometry dirty.
    void MarkSpriteDirty();
    /// Update sprite.
    void UpdateSprite();
    /// Update sprite batches.
    void UpdateSpriteBatches();
    /// Create materials for sprite rendering.
    void UpdateSpriteMaterials(bool forceUpdate = false);

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
    /// Image set.
    SharedPtr<ImageSet> imageSet_;
    /// Image name.
    String imageName_;
    /// Material.
    SharedPtr<Material> material_;
    /// Blend mode.
    BlendMode blendMode_;

    /// Geometries.
    Vector<SharedPtr<Geometry> > geometries_;
    /// Vertex buffer.
    SharedPtr<VertexBuffer> vertexBuffer_;
    /// Vertices.
    Vector<Vertex2D> vertices_;
    /// sprite needs update flag.
    bool spriteDirty_;
    /// Geometry dirty flag.
    bool geometryDirty_;
};

}

