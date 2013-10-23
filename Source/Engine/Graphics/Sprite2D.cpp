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

#include "Precompiled.h"
#include "Camera.h"
#include "Context.h"
#include "Geometry.h"
#include "ImageSet.h"
#include "Material.h"
#include "Node.h"
#include "ResourceCache.h"
#include "Sprite2D.h"
#include "Technique.h"
#include "Texture.h"
#include "VertexBuffer.h"

#include "DebugNew.h"

namespace Urho3D
{

extern const char* GEOMETRY_CATEGORY;
extern const char* blendModeNames[];
static const unsigned SPRITE2D_NUMBER_VERTICES = 6;

Sprite2D::Sprite2D(Context* context) : Drawable(context, DRAWABLE_GEOMETRY),
    flipX_(false),
    flipY_(false),
    hotSpot_(0.5f, 0.5f),    
    color_(Color::WHITE),
    textureRect_(0, 0, 0, 0),
    useWholeTexture_(true),
    blendMode_(BLEND_REPLACE),
    vertexBuffer_(new VertexBuffer(context_)),
    geometryDirty_(true)
{
    vertices_.Reserve(SPRITE2D_NUMBER_VERTICES);
}

Sprite2D::~Sprite2D()
{
}

void Sprite2D::RegisterObject(Context* context)
{
    context->RegisterFactory<Sprite2D>(GEOMETRY_CATEGORY);

    ACCESSOR_ATTRIBUTE(Sprite2D, VAR_BOOL, "Flip X", GetFlipX, SetFilpX, bool, false, AM_DEFAULT);
    ACCESSOR_ATTRIBUTE(Sprite2D, VAR_BOOL, "Flip Y", GetFlipY, SetFlipY, bool, false, AM_DEFAULT);
    REF_ACCESSOR_ATTRIBUTE(Sprite2D, VAR_VECTOR2, "Hot Spot", GetHotSpot, SetHotSpot, Vector2, Vector2(0.5f, 0.5f), AM_FILE);    
    REF_ACCESSOR_ATTRIBUTE(Sprite2D, VAR_COLOR, "Color", GetColor, SetColor, Color, Color::WHITE, AM_DEFAULT);
    ACCESSOR_ATTRIBUTE(Sprite2D, VAR_RESOURCEREF, "Texture", GetTextureAttr, SetTextureAttr, ResourceRef, ResourceRef(Texture::GetTypeStatic()), AM_FILE);
    ACCESSOR_ATTRIBUTE(Sprite2D, VAR_RESOURCEREF, "Imageset", GetImagesetAttr, SetImagesetAttr, ResourceRef, ResourceRef(ImageSet::GetTypeStatic()), AM_DEFAULT);
    REF_ACCESSOR_ATTRIBUTE(Sprite2D, VAR_STRING, "Image Name", GetImageName, SetImageName, String, String::EMPTY, AM_FILE);
    ACCESSOR_ATTRIBUTE(Sprite2D, VAR_RESOURCEREF, "Material", GetMaterialAttr, SetMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);
    REF_ACCESSOR_ATTRIBUTE(Sprite2D, VAR_INTRECT, "Texture Rect", GetTextureRect, SetTextureRect, IntRect, IntRect::ZERO, AM_FILE);
    ACCESSOR_ATTRIBUTE(Sprite2D, VAR_BOOL, "Use Whole Texture", IsUseWholeTexture, SetUseWholeTexture, bool, true, AM_DEFAULT);
    // ENUM_ACCESSOR_ATTRIBUTE(Sprite2D, "Blend Mode", GetBlendMode, SetBlendMode, BlendMode, blendModeNames, 0, AM_FILE);
    COPY_BASE_ATTRIBUTES(Sprite2D, Drawable);
}

void Sprite2D::ApplyAttributes()
{
    MarkSpriteDirty();
    UpdateSpriteBatches();
    UpdateSpriteMaterials();
}

void Sprite2D::UpdateBatches(const FrameInfo& frame)
{
    const Matrix3x4& worldTransform = node_->GetWorldTransform();
    distance_ = frame.camera_->GetDistance(GetWorldBoundingBox().Center());

    for (unsigned i = 0; i < batches_.Size(); ++i)
    {
        batches_[i].distance_ = distance_;
        batches_[i].worldTransform_ = &worldTransform;
    }
}

void Sprite2D::UpdateGeometry(const FrameInfo& frame)
{
    if (geometryDirty_)
    {
        for (unsigned i = 0; i < batches_.Size(); ++i)
        {
            Geometry* geometry = geometries_[i];
            geometry->SetDrawRange(TRIANGLE_LIST, 0, 0, 0, SPRITE2D_NUMBER_VERTICES);
        }
    }
    
    if ((geometryDirty_ || vertexBuffer_->IsDataLost()) && vertices_.Size())
    {
        unsigned vertexCount = vertices_.Size();
        if (vertexBuffer_->GetVertexCount() != vertexCount)
            vertexBuffer_->SetSize(vertexCount, MASK_POSITION | MASK_COLOR | MASK_TEXCOORD1);
        vertexBuffer_->SetData(&vertices_[0]);
    }
    
    geometryDirty_ = false;
}

UpdateGeometryType Sprite2D::GetUpdateGeometryType()
{
    if (geometryDirty_ || vertexBuffer_->IsDataLost())
        return UPDATE_MAIN_THREAD;
    else
        return UPDATE_NONE;
}

void Sprite2D::SetFilpX(bool flipX)
{
    if (flipX_ != flipX)
    {
        flipX_ = flipX;

        MarkSpriteDirty();
        UpdateSpriteBatches();
        UpdateSpriteMaterials();
    }
}

void Sprite2D::SetFlipY(bool flipY)
{
    if (flipY_ != flipY)
    {
        flipY_ = flipY;

        MarkSpriteDirty();
        UpdateSpriteBatches();
        UpdateSpriteMaterials();
    }
}

void Sprite2D::SetHotSpot(const Vector2& hotSpot)
{
    hotSpot_ = hotSpot;

    MarkSpriteDirty();
    UpdateSpriteBatches();
    UpdateSpriteMaterials();
}

void Sprite2D::SetHotSpot(float x, float y)
{
    SetHotSpot(Vector2(x, y));
}

void Sprite2D::SetColor(const Color& color)
{
    color_ = color;

    MarkSpriteDirty();
    UpdateSpriteBatches();
    UpdateSpriteMaterials();
}

void Sprite2D::SetTexture(Texture* texture)
{
    texture_ = texture;

    UpdateSpriteMaterials();
}

void Sprite2D::SetTextureRect(const IntRect& rect)
{
    textureRect_ = rect;
    useWholeTexture_ = false;

    MarkSpriteDirty();
    UpdateSpriteBatches();
    UpdateSpriteMaterials();
}

void Sprite2D::SetUseWholeTexture(bool useWholeTexture)
{
    if (useWholeTexture_ != useWholeTexture)
    {
        useWholeTexture_ = useWholeTexture;

        MarkSpriteDirty();
        UpdateSpriteBatches();
        UpdateSpriteMaterials();
    }
}

void Sprite2D::SetImageSet(ImageSet* imageSet)
{
    Texture* texture = imageSet->GetTexture();
    if (!texture)
        return;

    SetTexture(texture);
    imageSet_ = imageSet;

    if (imageName_.Empty())
        SetImageName(imageName_);
}

void Sprite2D::SetImageName(const String& imageName)
{
    if (imageSet_)
    {
        const IntRect& textureRect = imageSet_->GetTextureRect(imageName);
        if (textureRect.Width() == 0 || textureRect.Height() == 0)
            return;
        SetTextureRect(textureRect);
    }

    imageName_ = imageName;
}

bool Sprite2D::SetImage(ImageSet* imageSet, const String& imageName)
{
    if (!imageSet || imageName.Empty())
        return false;

    Texture* texture = imageSet->GetTexture();
    if (!texture)
        return false;

    const IntRect& textureRect = imageSet->GetTextureRect(imageName);
    if (textureRect.Width() == 0 || textureRect.Height() == 0)
        return false;

    SetTexture(texture);
    SetTextureRect(textureRect);

    imageSet_ = imageSet;
    imageName_ = imageName;

    return true;
}

void Sprite2D::SetMaterial(Material* material)
{
    material_ = material;
    
    UpdateSpriteMaterials(true);
}

void Sprite2D::SetBlendMode(BlendMode mode)
{
    if (blendMode_ != mode)
    {
        blendMode_ = mode;
        UpdateSpriteMaterials(true);
    }
}

Texture* Sprite2D::GetTexture() const
{
    return texture_;
}

ImageSet* Sprite2D::GetImageset() const
{
    return imageSet_;
}

Material* Sprite2D::GetMaterial() const
{
    return material_;
}

void Sprite2D::OnWorldBoundingBoxUpdate()
{
    if (spriteDirty_)
        UpdateSpriteBatches();

    worldBoundingBox_ = boundingBox_.Transformed(node_->GetWorldTransform());
}

void Sprite2D::MarkSpriteDirty()
{
    spriteDirty_ = true;
    
    OnMarkedDirty(node_);
    MarkNetworkUpdate();
}

void Sprite2D::SetTextureAttr(ResourceRef value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    SetTexture(cache->GetResource<Texture>(value.id_));
}

Urho3D::ResourceRef Sprite2D::GetTextureAttr() const
{
    return GetResourceRef(texture_, Texture::GetTypeStatic());
}

void Sprite2D::SetImagesetAttr(ResourceRef value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    SetImageSet(cache->GetResource<ImageSet>(value.id_));
}

Urho3D::ResourceRef Sprite2D::GetImagesetAttr() const
{
    return GetResourceRef(texture_, ImageSet::GetTypeStatic());
}

void Sprite2D::SetMaterialAttr(ResourceRef value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    SetMaterial(cache->GetResource<Material>(value.id_));
}

ResourceRef Sprite2D::GetMaterialAttr() const
{
    return GetResourceRef(material_, Material::GetTypeStatic());
}

void Sprite2D::UpdateSpriteBatches()
{
    vertices_.Clear();

    if (!texture_)
        return;

    /*
    V1 --------V2
    |         / |
    |       /   |
    |     /     |
    |   /       |
    | /         |
    V0 --------V3
    */
    Vertex2D vertex0;
    Vertex2D vertex1;
    Vertex2D vertex2;
    Vertex2D vertex3;

    float width;
    float height;
    if (useWholeTexture_)
    {
        width = (float)texture_->GetWidth();
        height = (float)texture_->GetHeight();
    }
    else
    {
        width = (float)textureRect_.Width();
        height = (float)textureRect_.Height();
    }

    float hotSpotX = flipX_ ? (1.0f - hotSpot_.x_) : hotSpot_.x_;
    float hotSpotY = flipY_ ? (1.0f - hotSpot_.y_) : hotSpot_.y_;
    float leftX = -width * hotSpotX;
    float bottomY = -height * hotSpotY;
    vertex0.position_ = Vector3(leftX, bottomY, 0.0f);
    vertex1.position_ = Vector3(leftX, bottomY + height, 0.0f);
    vertex2.position_ = Vector3(leftX + width, bottomY + height, 0.0f);
    vertex3.position_ = Vector3(leftX + width, bottomY, 0.0f);

    float leftU = 0.0f;
    float bottomV = 0.0f;
    float rightU = 1.0f;
    float topV = 1.0f;
    if (!useWholeTexture_)
    {
        float invTexW = 1.0f;
        float invTexH = 1.0f;
        if (texture_)
        {
            invTexW = 1.0f / (float)texture_->GetWidth();
            invTexH = 1.0f / (float)texture_->GetHeight();
        }
        leftU = textureRect_.left_ * invTexW;
        rightU = textureRect_.right_ * invTexW;
        bottomV = textureRect_.bottom_ * invTexH;
        topV = textureRect_.top_ * invTexH;
    }

    if (flipX_)
        Swap(leftU, rightU);    
    if (flipY_)
        Swap(bottomV, topV);
    vertex0.uv_ = Vector2(leftU, bottomV);
    vertex1.uv_ = Vector2(leftU, topV);
    vertex2.uv_ = Vector2(rightU, topV);
    vertex3.uv_ = Vector2(rightU, bottomV);

    vertex0.color_ = vertex1.color_ = vertex2.color_  = vertex3.color_ = color_.ToUInt();

    vertices_.Push(vertex0);
    vertices_.Push(vertex1);
    vertices_.Push(vertex2);

    vertices_.Push(vertex0);
    vertices_.Push(vertex2);
    vertices_.Push(vertex3);

    boundingBox_.defined_ = false;
    boundingBox_.min_ = boundingBox_.max_ = Vector3::ZERO;

    for (unsigned i = 0; i < vertices_.Size(); ++i)
        boundingBox_.Merge(vertices_[i].position_);

    spriteDirty_ = false;
    geometryDirty_ = true;
}

void Sprite2D::UpdateSpriteMaterials(bool forceUpdate)
{
    batches_.Resize(1);
    geometries_.Resize(1);

    for (unsigned i = 0; i < batches_.Size(); ++i)
    {
        if (!geometries_[i])
        {
            Geometry* geometry = new Geometry(context_);
            geometry->SetVertexBuffer(0, vertexBuffer_, MASK_POSITION | MASK_COLOR | MASK_TEXCOORD1);
            batches_[i].geometry_ = geometries_[i] = geometry;
        }
        
        if (!batches_[i].material_ || forceUpdate)
        {
            // If material not defined, create a reasonable default from scratch
            if (!material_)
            {
                Material* material = new Material(context_);
                Technique* tech = new Technique(context_);
                Pass* pass = tech->CreatePass(PASS_ALPHA);
                pass->SetVertexShader("Basic_DiffVCol");

                if (blendMode_ != BLEND_ALPHA && blendMode_ != BLEND_ADDALPHA && blendMode_ != BLEND_PREMULALPHA)
                    pass->SetPixelShader("Basic_DiffVCol");
                else
                    pass->SetPixelShader("Basic_DiffAlphaMaskVCol");

                pass->SetBlendMode(blendMode_);
                pass->SetDepthWrite(false);
                material->SetTechnique(0, tech);
                material->SetCullMode(CULL_NONE);
                batches_[i].material_ = material;
            }
            else
                batches_[i].material_ = material_->Clone();
        }
        
        Material* material = batches_[i].material_;
        material->SetTexture(TU_DIFFUSE, texture_);
    }
}

}
