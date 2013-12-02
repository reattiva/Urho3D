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

#include "Resource.h"

namespace Urho3D
{

class SpriteSheet;
class Texture;
class XMLElement;

/// Sprite frame.
class URHO3D_API SpriteFrame
{
public:
    /// Construct empty.
    SpriteFrame();
    /// Destruct.
    virtual ~SpriteFrame();

    /// X.
    int x_;
    /// Y.
    int y_;
    /// Width.
    int width_;
    /// Height.
    int height_;

    /// Rotated.
    bool rotated_;
    /// Offset X.
    int offsetX_;
    /// Offset Y.
    int offsetY_;
    /// Origin with.
    int originWidth_;
    /// Origin height.
    int originHeight_;
};

/// %Sprite sheet.
class URHO3D_API SpriteSheet : public Resource
{
    OBJECT(SpriteSheet);
    
public:
    /// Construct empty.
    SpriteSheet(Context* context);
    /// Destruct.
    virtual ~SpriteSheet();
    /// Register object factory.
    static void RegisterObject(Context* context);
    
    /// Load resource. Return true if successful.
    virtual bool Load(Deserializer& source);
    /// Save resource. Return true if successful.
    virtual bool Save(Serializer& dest) const;

    /// Return texture.
    Texture* GetTexture() const { return texture_; }
    /// Return sprite frame by sprite name.
    const SpriteFrame* GetSpriteFrame(const String& spriteName) const;
    /// Return all sprite names.
    const Vector<String>& GetAllSpriteNames() const { return spriteNames_; }

private:
    /// Load from sprite sheet file.
    bool LoadSpriteSheet(XMLElement& source);
    /// Load from property list file.
    bool LoadPropertyList(XMLElement& source);
    /// Set texture file name.
    bool SetTextureFileName(const String& textureFileName);

    /// Texture file name.
    String textureFileName_;
    /// Texture.
    SharedPtr<Texture> texture_;
    /// Sprite names.
    Vector<String> spriteNames_;
    /// Sprite frame mapping.
    HashMap<String, SpriteFrame> spriteFrameMapping_;
};

}
