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
#include "Context.h"
#include "ResourceCache.h"
#include "Sprite2D.h"
#include "SpriteSheet.h"
#include "Texture.h"

#include "DebugNew.h"

namespace Urho3D
{

extern const char* GEOMETRY_CATEGORY;

Sprite2D::Sprite2D(Context* context) : Drawable2D(context),
    unitPerPixel_(1.0f),
    flipX_(false),
    flipY_(false),
    hotSpot_(0.5f, 0.5f),
    color_(Color::WHITE)
{
    vertices_.Reserve(6);
}

Sprite2D::~Sprite2D()
{
}

void Sprite2D::RegisterObject(Context* context)
{
    context->RegisterFactory<Sprite2D>(GEOMETRY_CATEGORY);

    ACCESSOR_ATTRIBUTE(Sprite2D, VAR_FLOAT, "Unit Per Pixel", GetUnitPerPixel, SetUnitPerPixel, float, 1.0f, AM_DEFAULT);
    ACCESSOR_ATTRIBUTE(Sprite2D, VAR_BOOL, "Flip X", GetFlipX, SetFilpX, bool, false, AM_DEFAULT);
    ACCESSOR_ATTRIBUTE(Sprite2D, VAR_BOOL, "Flip Y", GetFlipY, SetFlipY, bool, false, AM_DEFAULT);
    REF_ACCESSOR_ATTRIBUTE(Sprite2D, VAR_VECTOR2, "Hot Spot", GetHotSpot, SetHotSpot, Vector2, Vector2(0.5f, 0.5f), AM_FILE);
    REF_ACCESSOR_ATTRIBUTE(Sprite2D, VAR_COLOR, "Color", GetColor, SetColor, Color, Color::WHITE, AM_DEFAULT);
    COPY_BASE_ATTRIBUTES(Sprite2D, Drawable2D);
}

void Sprite2D::SetUnitPerPixel(float unitPerPixel)
{
    unitPerPixel_ = Max(1.0f, unitPerPixel);
    MarkGeometryDirty();
}

void Sprite2D::SetFilpX(bool flipX)
{
    if (flipX_ != flipX)
    {
        flipX_ = flipX;
        MarkGeometryDirty();
    }
}

void Sprite2D::SetFlipY(bool flipY)
{
    if (flipY_ != flipY)
    {
        flipY_ = flipY;
        MarkGeometryDirty();
    }
}

void Sprite2D::SetHotSpot(const Vector2& hotSpot)
{
    hotSpot_ = hotSpot;

    MarkGeometryDirty();
}

void Sprite2D::SetHotSpot(float x, float y)
{
    SetHotSpot(Vector2(x, y));
}

void Sprite2D::SetColor(const Color& color)
{
    color_ = color;

    MarkGeometryDirty();
}

void Sprite2D::UpdateVertices()
{
    if (!verticesDirty_)
        return;

    vertices_.Clear();

    Texture* texture = texture_;
    if (spriteSheet_)
        texture = spriteSheet_->GetTexture();

    if (!texture)
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

    if (!spriteFrame_)
        BuildQuadVertices(vertex0, vertex1, vertex2, vertex3, texture);
    else
        BuildQuadVertices(vertex0, vertex1, vertex2, vertex3, texture, spriteFrame_);

    vertices_.Push(vertex0);
    vertices_.Push(vertex1);
    vertices_.Push(vertex2);

    vertices_.Push(vertex0);
    vertices_.Push(vertex2);
    vertices_.Push(vertex3);

    verticesDirty_ = false;
    geometryDirty_ = true;
}

void Sprite2D::BuildQuadVertices(Vertex2D& vertex0, Vertex2D& vertex1, Vertex2D& vertex2, Vertex2D& vertex3, Texture* texture)
{
    float width = (float)texture->GetWidth();
    float height = (float)texture->GetHeight();
    width /= unitPerPixel_;
    height /= unitPerPixel_;

    float hotSpotX = flipX_ ? (1.0f - hotSpot_.x_) : hotSpot_.x_;
    float hotSpotY = flipY_ ? (1.0f - hotSpot_.y_) : hotSpot_.y_;
    float leftX = width * hotSpotX;
    float bottomY = height * hotSpotY;

    vertex0.position_ = Vector3(leftX, bottomY, 0.0f);
    vertex1.position_ = Vector3(leftX, bottomY + height, 0.0f);
    vertex2.position_ = Vector3(leftX + width, bottomY + height, 0.0f);
    vertex3.position_ = Vector3(leftX + width, bottomY, 0.0f);

    vertex0.uv_ = Vector2(0.0f, 1.0f);
    vertex1.uv_ = Vector2(0.0f, 0.0f);
    vertex2.uv_ = Vector2(1.0f, 0.0f);
    vertex3.uv_ = Vector2(1.0f, 1.0f);

    if (flipX_)
    {
        Swap(vertex0.uv_.x_, vertex3.uv_.x_);
        Swap(vertex1.uv_.x_, vertex2.uv_.x_);
    }
    if (flipY_)
    {
        Swap(vertex0.uv_.y_, vertex1.uv_.y_);
        Swap(vertex2.uv_.y_, vertex3.uv_.y_);
    }
    vertex0.color_ = vertex1.color_ = vertex2.color_  = vertex3.color_ = color_.ToUInt();
}

void Sprite2D::BuildQuadVertices(Vertex2D& vertex0, Vertex2D& vertex1, Vertex2D& vertex2, Vertex2D& vertex3, Texture* texture, const SpriteFrame* spriteFrame)
{
    float pixelPerUnit = 1.0f / unitPerPixel_;
    float width = (float)spriteFrame_->width_ * pixelPerUnit;
    float height = (float)spriteFrame_->height_ * pixelPerUnit;
    float offsetX = (float)spriteFrame_->offsetX_ * pixelPerUnit;
    float offsetY = (float)spriteFrame_->offsetY_ * pixelPerUnit;

    float hotSpotX = flipX_ ? (1.0f - hotSpot_.x_) : hotSpot_.x_;
    float hotSpotY = flipY_ ? (1.0f - hotSpot_.y_) : hotSpot_.y_;
    float leftX = offsetX - width * hotSpotX;
    float bottomY = offsetY - height * hotSpotY;

    vertex0.position_ = Vector3(leftX, bottomY, 0.0f);
    vertex1.position_ = Vector3(leftX, bottomY + height, 0.0f);
    vertex2.position_ = Vector3(leftX + width, bottomY + height, 0.0f);
    vertex3.position_ = Vector3(leftX + width, bottomY, 0.0f);

    float invTexW = 1.0f / texture->GetWidth();
    float invTexH = 1.0f / texture->GetHeight();
    float leftU = spriteFrame_->x_ * invTexW;
    float topV = spriteFrame_->y_* invTexH;

    if (spriteFrame_->rotated_)
    {
        float rightU = (spriteFrame_->x_ + spriteFrame_->height_) * invTexW;
        float bottomV = (spriteFrame_->y_ + spriteFrame_->width_) * invTexH;

        vertex0.uv_ = Vector2(leftU, topV);
        vertex1.uv_ = Vector2(rightU, topV);
        vertex2.uv_ = Vector2(rightU, bottomV);
        vertex3.uv_ = Vector2(leftU, bottomV);
    }
    else
    {

        float rightU = (spriteFrame_->x_ + spriteFrame_->width_) * invTexW;
        float bottomV = (spriteFrame_->y_ + spriteFrame_->height_) * invTexH;

        vertex0.uv_ = Vector2(leftU, bottomV);
        vertex1.uv_ = Vector2(leftU, topV);
        vertex2.uv_ = Vector2(rightU, topV);
        vertex3.uv_ = Vector2(rightU, bottomV);
    }

    if (flipX_)
    {
        Swap(vertex0.uv_.x_, vertex3.uv_.x_);
        Swap(vertex1.uv_.x_, vertex2.uv_.x_);
    }
    if (flipY_)
    {
        Swap(vertex0.uv_.y_, vertex1.uv_.y_);
        Swap(vertex2.uv_.y_, vertex3.uv_.y_);
    }
    vertex0.color_ = vertex1.color_ = vertex2.color_  = vertex3.color_ = color_.ToUInt();
}
}
