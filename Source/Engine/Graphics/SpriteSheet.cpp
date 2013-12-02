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

SpriteFrame::SpriteFrame() : x_(0), 
    y_(0), 
    width_(0), 
    height_(0), 
    rotated_(false), 
    offsetX_(0), 
    offsetY_(0), 
    originWidth_(0), 
    originHeight_(0)
{

}

SpriteFrame::~SpriteFrame()
{

}

SpriteSheet::SpriteSheet(Context* context) : Resource(context)
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
    spriteNames_.Clear();
    spriteFrameMapping_.Clear();

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

    if (rootElem.GetName() == "SpriteSheet")
        return LoadSpriteSheet(rootElem);
    else if (rootElem.GetName() == "plist")
        return LoadPropertyList(rootElem);
    else
    {
        LOGERROR("Invalid sprite sheet file");
        return false;
    }
}

bool SpriteSheet::Save(Serializer& dest) const
{
    XMLFile xmlFile(context_);
    
    XMLElement root = xmlFile.CreateRoot("SpriteSheet");
    root.SetString("texture", textureFileName_);

    for (HashMap<String, SpriteFrame>::ConstIterator i = spriteFrameMapping_.Begin(); i != spriteFrameMapping_.End(); ++i)
    {
        XMLElement spriteElem = root.CreateChild("SpriteFrame");
        spriteElem.SetString("name", i->first_);

        const SpriteFrame& spriteFrame = i->second_;
        spriteElem.SetInt("x", spriteFrame.x_);
        spriteElem.SetInt("y", spriteFrame.y_);
        spriteElem.SetInt("width", spriteFrame.width_);
        spriteElem.SetInt("height", spriteFrame.height_);
        
        if (spriteFrame.rotated_)
            spriteElem.SetBool("rotated", spriteFrame.rotated_);
        
        if (spriteFrame.offsetX_ != 0)
            spriteElem.SetInt("offsetX", spriteFrame.offsetX_);
        
        if (spriteFrame.offsetY_ != 0)
            spriteElem.SetInt("offsetY", spriteFrame.offsetY_);

        if (spriteFrame.originWidth_ != spriteFrame.width_)
            spriteElem.SetInt("originWidth", spriteFrame.originWidth_);

        if (spriteFrame.originHeight_!= spriteFrame.height_)
            spriteElem.SetInt("originHeight", spriteFrame.originHeight_);
    }

    return xmlFile.Save(dest);
}

const SpriteFrame* SpriteSheet::GetSpriteFrame(const String& spriteName) const
{
    HashMap<String, SpriteFrame>::ConstIterator i = spriteFrameMapping_.Find(spriteName);
    if (i != spriteFrameMapping_.End())
        return &i->second_;

    return 0;
}

bool SpriteSheet::LoadSpriteSheet(XMLElement& source)
{
    if (!SetTextureFileName(source.GetAttribute("texture")))
        return false;

    XMLElement spriteElem = source.GetChild("SpriteFrame");
    while (spriteElem)
    {
        String spriteName = spriteElem.GetAttribute("name");

        SpriteFrame spriteFrame;
        spriteFrame.x_ = spriteElem.GetInt("x");
        spriteFrame.y_ = spriteElem.GetInt("y");
        spriteFrame.width_ = spriteElem.GetInt("width");
        spriteFrame.height_ = spriteElem.GetInt("height");

        if (spriteElem.HasAttribute("rotated"))
            spriteFrame.rotated_ = spriteElem.GetBool("rotated");

        if (spriteElem.HasAttribute("offsetX"))
            spriteFrame.offsetX_ = spriteElem.GetInt("offsetX");

        if (spriteElem.HasAttribute("offsetY"))
            spriteFrame.offsetY_ = spriteElem.GetInt("offsetY");

        spriteFrame.originWidth_ = spriteFrame.width_;
        if (spriteElem.HasAttribute("originWidth"))
            spriteFrame.originWidth_ = spriteElem.GetInt("originWidth");

        spriteFrame.originHeight_ = spriteFrame.height_;
        if (spriteElem.HasAttribute("originHeight"))
            spriteFrame.originHeight_ = spriteElem.GetInt("originHeight");

        spriteNames_.Push(spriteName);
        spriteFrameMapping_[spriteName] = spriteFrame;

        spriteElem = spriteElem.GetNext("SpriteFrame");
    }

    return true;
}

bool SpriteSheet::LoadPropertyList(XMLElement& source)
{
    PropertyList plist(context_);
    if (!plist.LoadXML(source))
        return false;

    const PLDictionary& root = plist.GetRoot();

    const PLDictionary& metadata = root.GetDictionary("metadata");
    int format = metadata.GetInt("format");
    if (format != 1 && format != 2)
    {
        LOGERROR("Unsupported format.");
        return false;
    }

    if (!SetTextureFileName(metadata.GetString("realTextureFileName")))
        return false;

    const PLDictionary& frames = root.GetDictionary("frames");
    for (PLDictionary::ConstIterator i = frames.Begin(); i != frames.End(); ++i)
    {
        String spriteName = i->first_;
        const PLDictionary& frame = i->second_->ToDictionary();

        SpriteFrame spriteFrame;
        PLStringToIntXYWH(frame.GetString("frame"), spriteFrame.x_, spriteFrame.y_, spriteFrame.width_, spriteFrame.height_);

        spriteFrame.rotated_ = frame.GetBool("rotated");

        PLStringToIntXY(frame.GetString("offset"), spriteFrame.offsetX_, spriteFrame.offsetY_);
        PLStringToIntXY(frame.GetString("sourceSize"), spriteFrame.originWidth_, spriteFrame.originHeight_);

        spriteNames_.Push(spriteName);
        spriteFrameMapping_[spriteName] = spriteFrame;
    }

    return true;
}

bool SpriteSheet::SetTextureFileName(const String& textureFileName)
{
    textureFileName_ = textureFileName;

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    texture_ = cache->GetResource<Texture2D>(textureFileName_);
    if (!texture_)
    {
        LOGERROR("Count not load texture");
        return false;
    }

    return true;
}

}