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
#include "Log.h"
#include "PropertyList.h"
#include "ResourceCache.h"
#include "SpriteSheet.h"
#include "Texture2D.h"
#include "XMLFile.h"
#include <cstdio>

#include "DebugNew.h"

namespace Urho3D
{

SpriteSheet::SpriteSheet(Context* context) :
    Resource(context)
{
}

SpriteSheet::~SpriteSheet()
{
}

void SpriteSheet::RegisterObject(Context* context)
{
    context->RegisterFactory<SpriteSheet>();
}

bool SpriteSheet::Load(Deserializer& source)
{
    SharedPtr<XMLFile> xmlFile(new XMLFile(context_));
    if(!xmlFile->Load(source))
    {
        LOGERROR("Could not load image set file");
        return false;
    }

    SetMemoryUse(source.GetSize());

    XMLElement rootElem = xmlFile->GetRoot();
    if (!rootElem)
    {
        LOGERROR("Count not find image set element");
        return false;
    }

    if (rootElem.GetName() == "Imageset")
        return LoadImageSet(rootElem);
    else if (rootElem.GetName() == "plist")
        return LoadPropertyList(rootElem);
    else
    {
        LOGERROR("Invalid sprite sheet file");
        return false;
    }
}

const IntRect& SpriteSheet::GetSpriteTextureRect(const String& name) const
{
    HashMap<String, IntRect>::ConstIterator i = spriteTextureRectMapping_.Find(name);
    if (i != spriteTextureRectMapping_.End())
        return i->second_;

    return IntRect::ZERO;
}

bool SpriteSheet::LoadImageSet(XMLElement &rootElem)
{
    textureFileName_ = rootElem.GetAttribute("Imagefile");
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    texture_ = cache->GetResource<Texture2D>(textureFileName_);
    if (!texture_)
    {
        LOGERROR("Count not load texture");
        return false;
    }

    XMLElement imageElem = rootElem.GetChild("Image");
    while (imageElem)
    {
        String name = imageElem.GetAttribute("Name");

        IntRect textureRect;
        textureRect.left_ = imageElem.GetInt("XPos");
        textureRect.top_  = imageElem.GetInt("YPos");
        textureRect.right_ = textureRect.left_ + imageElem.GetInt("Width");
        textureRect.bottom_ = textureRect.top_ + imageElem.GetInt("Height");

        spriteNames_.Push(name);
        spriteTextureRectMapping_[name] = textureRect;

        imageElem = imageElem.GetNext("Image");
    }

    return true;
}

bool SpriteSheet::LoadPropertyList(XMLElement& rootElem)
{
    PropertyList plist(context_);
    if (!plist.LoadXML(rootElem))
        return false;

    const PLDictionary& root = plist.GetRoot();

    const PLDictionary& metadata = root.GetDictionary("metadata");
    int format = metadata.GetInt("format");
    if (format != 1 && format != 2)
    {
        LOGERROR("Unsupported format.");
        return false;
    }

    textureFileName_ = metadata.GetString("realTextureFileName");
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    texture_ = cache->GetResource<Texture2D>(textureFileName_);
    if (!texture_)
    {
        LOGERROR("Count not load texture");
        return false;
    }

    const PLDictionary& frames = root.GetDictionary("frames");
    for (PLDictionary::ConstIterator i = frames.Begin(); i != frames.End(); ++i)
    {
        String name = i->first_;
        
        const PLDictionary& frame = i->second_->ToDictionary();        
        IntRect textureRect = PLStringToIntRect(frame.GetString("frame"));

        spriteNames_.Push(name);
        spriteTextureRectMapping_[name] = textureRect;
    }

    return true;
}

}