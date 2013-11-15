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

class Texture;
class XMLElement;

/// %Sprite sheet resource.
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

    /// Set texture file name.
    bool SetTextureFileName(const String& textureFileName);
    /// Return texture file name.
    const String& GetTextureFileName() const { return textureFileName_; }
    /// Return texture.
    Texture* GetTexture() const { return texture_; }
    /// Return all sprite names.
    const Vector<String>& GetSpriteNames() const { return spriteNames_; }
    /// Return sprite texture rectangle.
    const IntRect& GetSpriteTextureRect(const String& name) const;

private:
    /// Load from sprite sheet file.
    bool LoadSpriteSheet(XMLElement& source);
    /// Load from property list file.
    bool LoadPropertyList(XMLElement& source);    

    /// Texture file name.
    String textureFileName_;
    /// Texture.
    SharedPtr<Texture> texture_;
    /// Sprite names.
    Vector<String> spriteNames_;
    /// Sprite texture rect mapping.
    HashMap<String, IntRect> spriteTextureRectMapping_;
};

}
