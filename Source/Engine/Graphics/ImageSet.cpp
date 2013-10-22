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
#include "ImageSet.h"
#include "Log.h"
#include "ResourceCache.h"
#include "Texture2D.h"
#include "XMLFile.h"

#include "DebugNew.h"

namespace Urho3D
{

ImageSet::ImageSet(Context* context) :
    Resource(context)
{
}

ImageSet::~ImageSet()
{
}

void ImageSet::RegisterObject(Context* context)
{
    context->RegisterFactory<ImageSet>();
}

bool ImageSet::Load(Deserializer& source)
{
    SharedPtr<XMLFile> xmlFile(new XMLFile(context_));
    if(!xmlFile->Load(source))
    {
        LOGERROR("Could not load image set file");
        return false;
    }

    XMLElement rootElem = xmlFile->GetRoot("Imageset");
    if (!rootElem)
    {
        LOGERROR("Count not find image set element");
        return false;
    }

    String imagefile = rootElem.GetAttribute("Imagefile");
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    texture_ = cache->GetResource<Texture2D>(imagefile);
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

        nameToTextureRectMapping_[name] = textureRect;

        imageElem = imageElem.GetNext("Image");
    }

    SetMemoryUse(source.GetSize());

    return true;
}

const IntRect& ImageSet::GetTextureRect(const String& name) const
{
    HashMap<String, IntRect>::ConstIterator i = nameToTextureRectMapping_.Find(name);
    if (i != nameToTextureRectMapping_.End())
        return i->second_;

    return IntRect::ZERO;
}

}