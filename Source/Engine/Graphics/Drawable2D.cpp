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
#include "Drawable2D.h"
#include "Geometry.h"
#include "Material.h"
#include "Node.h"
#include "ResourceCache.h"
#include "SpriteSheet.h"
#include "Technique.h"
#include "Texture.h"
#include "VertexBuffer.h"

#include "DebugNew.h"

namespace Urho3D
{

extern const char* blendModeNames[];

Drawable2D::Drawable2D(Context* context) : Drawable(context, DRAWABLE_GEOMETRY),
    spriteFrame_(0), 
    blendMode_(BLEND_REPLACE),
    vertexBuffer_(new VertexBuffer(context_)),
    verticesDirty_(true),
    geometryDirty_(true),
    materialDirty_(true)
{    
}

Drawable2D::~Drawable2D()
{
}

void Drawable2D::RegisterObject(Context* context)
{
    ACCESSOR_ATTRIBUTE(Drawable2D, VAR_RESOURCEREF, "Sprite Sheet", GetSpriteSheetAttr, SetSpriteSheetAttr, ResourceRef, ResourceRef(SpriteSheet::GetTypeStatic()), AM_FILE);
    REF_ACCESSOR_ATTRIBUTE(Drawable2D, VAR_STRING, "Sprite Name", GetSpriteName, SetSpriteName, String, String::EMPTY, AM_FILE);
    ACCESSOR_ATTRIBUTE(Drawable2D, VAR_RESOURCEREF, "Texture", GetTextureAttr, SetTextureAttr, ResourceRef, ResourceRef(Texture::GetTypeStatic()), AM_FILE);
    ACCESSOR_ATTRIBUTE(Drawable2D, VAR_RESOURCEREF, "Material", GetMaterialAttr, SetMaterialAttr, ResourceRef, ResourceRef(Material::GetTypeStatic()), AM_DEFAULT);
    ENUM_ACCESSOR_ATTRIBUTE(Drawable2D, "Blend Mode", GetBlendMode, SetBlendMode, BlendMode, blendModeNames, 0, AM_FILE);
    COPY_BASE_ATTRIBUTES(Drawable2D, Drawable);
}

void Drawable2D::ApplyAttributes()
{
    UpdateVertices();
    UpdateMaterial(true);
}

void Drawable2D::UpdateBatches(const FrameInfo& frame)
{
    if (batches_.Empty())
        batches_.Resize(1);

    if (materialDirty_)
        UpdateMaterial();

    batches_[0].geometry_ = geometry_;
    batches_[0].material_ = usedMaterial_;
    
    const Matrix3x4& worldTransform = node_->GetWorldTransform();
    distance_ = frame.camera_->GetDistance(GetWorldBoundingBox().Center());

    batches_[0].distance_ = distance_;
    batches_[0].worldTransform_ = &worldTransform;
}

void Drawable2D::UpdateGeometry(const FrameInfo& frame)
{
    if (!geometry_)
    {
        geometry_ = new Geometry(context_);
        geometry_->SetVertexBuffer(0, vertexBuffer_, MASK_POSITION | MASK_COLOR | MASK_TEXCOORD1);
    }
    
    if (verticesDirty_)
        UpdateVertices();

    if ((geometryDirty_ || vertexBuffer_->IsDataLost()) && vertices_.Size())
    {
        unsigned vertexCount = vertices_.Size();
        if (vertexBuffer_->GetVertexCount() != vertexCount)
            vertexBuffer_->SetSize(vertexCount, MASK_POSITION | MASK_COLOR | MASK_TEXCOORD1);
        vertexBuffer_->SetData(&vertices_[0]);
        geometry_->SetDrawRange(TRIANGLE_LIST, 0, 0, 0, vertices_.Size());
    }

    geometryDirty_ = false;
}

UpdateGeometryType Drawable2D::GetUpdateGeometryType()
{
    if (geometryDirty_ || vertexBuffer_->IsDataLost())
        return UPDATE_MAIN_THREAD;
    else
        return UPDATE_NONE;
}

void Drawable2D::SetSpriteFrame(SpriteSheet* spriteSheet, const String& spriteName)
{
    if (spriteSheet_ != spriteSheet)
    {
        spriteSheet_ = spriteSheet;
        verticesDirty_ = true;
        materialDirty_ = true;
    }

    if (spriteName_ != spriteName)
    {
        spriteName_ = spriteName;
        verticesDirty_ = true;
    }

    spriteFrame_ = 0;
    if (spriteSheet_ && !spriteName_.Empty())
        spriteFrame_ = spriteSheet_->GetSpriteFrame(spriteName);
}

void Drawable2D::SetTexture(Texture* texture)
{
    texture_ = texture;

    materialDirty_ = true;
}

void Drawable2D::SetMaterial(Material* material)
{
    material_ = material;

    materialDirty_ = true;
}

void Drawable2D::SetBlendMode(BlendMode mode)
{
    if (blendMode_ != mode)
    {
        blendMode_ = mode;
        materialDirty_ = true;
    }
}

SpriteSheet* Drawable2D::GetSpriteSheet() const
{
    return spriteSheet_;
}

Texture* Drawable2D::GetTexture() const
{
    return texture_;
}

Material* Drawable2D::GetMaterial() const
{
    return material_;
}

void Drawable2D::OnWorldBoundingBoxUpdate()
{
    if (verticesDirty_)
    {
        UpdateVertices();
        boundingBox_.defined_ = false;
        boundingBox_.min_ = boundingBox_.max_ = Vector3::ZERO;
        for (unsigned i = 0; i < vertices_.Size(); ++i)
            boundingBox_.Merge(vertices_[i].position_);
    }

    worldBoundingBox_ = boundingBox_.Transformed(node_->GetWorldTransform());
}

void Drawable2D::SetSpriteSheetAttr(ResourceRef value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    SetSpriteFrame(cache->GetResource<SpriteSheet>(value.id_), spriteName_);
}

Urho3D::ResourceRef Drawable2D::GetSpriteSheetAttr() const
{
    return GetResourceRef(spriteSheet_, SpriteSheet::GetTypeStatic());
}

void Drawable2D::SetSpriteName(const String& spriteName)
{
    SetSpriteFrame(spriteSheet_, spriteName);
}

void Drawable2D::SetTextureAttr(ResourceRef value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    SetTexture(cache->GetResource<Texture>(value.id_));
}

Urho3D::ResourceRef Drawable2D::GetTextureAttr() const
{
    return GetResourceRef(texture_, Texture::GetTypeStatic());
}

void Drawable2D::SetMaterialAttr(ResourceRef value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    SetMaterial(cache->GetResource<Material>(value.id_));
}

ResourceRef Drawable2D::GetMaterialAttr() const
{
    return GetResourceRef(material_, Material::GetTypeStatic());
}

void Drawable2D::UpdateMaterial(bool forceUpdate)
{
    if (!materialDirty_ && !forceUpdate)
        return;

    if (!material_)
    {
        usedMaterial_ = new Material(context_);
        Technique* tech = new Technique(context_);
        Pass* pass = tech->CreatePass(PASS_ALPHA);
        pass->SetVertexShader("Basic_DiffVCol");

        if (blendMode_ != BLEND_ALPHA && blendMode_ != BLEND_ADDALPHA && blendMode_ != BLEND_PREMULALPHA)
            pass->SetPixelShader("Basic_DiffVCol");
        else
            pass->SetPixelShader("Basic_DiffAlphaMaskVCol");

        pass->SetBlendMode(blendMode_);
        pass->SetDepthWrite(false);
        usedMaterial_->SetTechnique(0, tech);
        usedMaterial_->SetCullMode(CULL_NONE);
    }
    else
        usedMaterial_ = material_->Clone();

    Texture* texture = texture_;
    if (spriteSheet_)
        texture = spriteSheet_->GetTexture();
    usedMaterial_->SetTexture(TU_DIFFUSE, texture);

    materialDirty_ = false;
}

}
