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
#include "StringUtils.h"
#include "XMLFile.h"
#include <cstdio>

#include "DebugNew.h"

namespace Urho3D
{
const PLValue PLValue::EMPTY;
const PLArray PLArray::EMPTY;
const PLDictionary PLDictionary::EMPTY;

PLNode::PLNode(NodeType type) : nodeType_(type)
{

}

PLNode::~PLNode()
{

}

bool PLNode::GetBool() const
{
    LOGERROR("Invalid call.");
    return false;
}

float PLNode::GetFloat() const
{
    LOGERROR("Invalid call.");
    return 0.0f;
}

int PLNode::GetInt() const
{
    LOGERROR("Invalid call.");
    return 0;
}

const String& PLNode::GetString() const
{
    LOGERROR("Invalid call.");
    return String::EMPTY;
}

const PLArray& PLNode::ToArray() const
{
    LOGERROR("Invalid call.");
    return PLArray::EMPTY;
}

const PLDictionary& PLNode::ToDictionary() const
{
    LOGERROR("Invalid call.");
    return PLDictionary::EMPTY;
}

PLValue::PLValue() : PLNode(NT_VALUE), valueType_(VAR_NONE)
{

}

PLValue::PLValue(bool value) : PLNode(NT_VALUE), valueType_(VAR_BOOL), bool_(value)
{

}

PLValue::PLValue(float value) : PLNode(NT_VALUE), valueType_(VAR_FLOAT), float_(value)
{

}

PLValue::PLValue(int value) : PLNode(NT_VALUE), valueType_(VAR_INT), int_(value)
{

}

PLValue::PLValue(const String& value) : PLNode(NT_VALUE), valueType_(VAR_STRING)
{
    string_ = new String(value);
}

PLValue::~PLValue()
{
    if (valueType_ == VAR_STRING)
        delete string_;
}

bool PLValue::GetBool() const
{
    if (valueType_ != VAR_BOOL)
    {
        LOGERROR("Invalid call.");
        return false;
    }
    return bool_;
}

float PLValue::GetFloat() const
{
    if (valueType_ != VAR_FLOAT)
    {
        LOGERROR("Invalid call.");
        return 0.0f;
    }
    return float_;
}

int PLValue::GetInt() const
{
    if (valueType_ != VAR_INT)
    {
        LOGERROR("Invalid call.");
        return 0;
    }
    return int_;
}

const String& PLValue::GetString() const
{
    if (valueType_ != VAR_STRING)
    {
        LOGERROR("Invalid call.");
        return String::EMPTY;
    }
    return *string_;
}

PLArray::PLArray() : PLNode(NT_ARRAY)
{
}

PLArray::~PLArray()
{
    for (unsigned i = 0; i < Size(); ++i)
        delete At(i);
}

const PLArray& PLArray::ToArray() const
{
    return *this;
}

bool PLArray::GetBool(unsigned index, bool defValue) const
{
    if (index < Size())
        return At(index)->GetBool();
    return defValue;
}

float PLArray::GetFloat(unsigned index, float defValue) const
{
    if (index < Size())
        return At(index)->GetFloat();
    return defValue;
}
int PLArray::GetInt(unsigned index, int defValue) const
{
    if (index < Size())
        return At(index)->GetInt();
    return defValue;
}

const String& PLArray::GetString(unsigned index, const String& defValue /*= ""*/) const
{
    if (index < Size())
        return At(index)->GetString();
    return defValue;
}

const PLArray& PLArray::GetArray(unsigned index) const
{
    if (index < Size())
        return At(index)->ToArray();
    return PLArray::EMPTY;
}

const PLDictionary& PLArray::GetDictionary(unsigned index) const
{
    if (index < Size())
        return At(index)->ToDictionary();
    return PLDictionary::EMPTY;
}

PLDictionary::PLDictionary() : PLNode(NT_DICTIONARY)
{

}

PLDictionary::~PLDictionary()
{
    for (Iterator i = Begin(); i != End(); ++i)
        delete i->second_;
}

const PLDictionary& PLDictionary::ToDictionary() const
{
    return *this;
}

bool PLDictionary::GetBool(const String& key, bool defValue) const
{
    ConstIterator i = Find(key);
    if (i != End())
        return i->second_->GetBool();
    return defValue;
}

float PLDictionary::GetFloat(const String& key, float defValue) const
{
    ConstIterator i = Find(key);
    if (i != End())
        return i->second_->GetFloat();
    return defValue;
}

int PLDictionary::GetInt(const String& key, int defValue) const
{
    ConstIterator i = Find(key);
    if (i != End())
        return i->second_->GetInt();
    return defValue;
}

const String& PLDictionary::GetString(const String& key, const String& defValue) const
{
    ConstIterator i = Find(key);
    if (i != End())
        return i->second_->GetString();
    return defValue;
}

const PLArray& PLDictionary::GetArray(const String& key) const
{
    ConstIterator i = Find(key);
    if (i != End())
        return i->second_->ToArray();
    return PLArray::EMPTY;
}

const PLDictionary& PLDictionary::GetDictionary(const String& key) const
{
    ConstIterator i = Find(key);
    if (i != End())
        return i->second_->ToDictionary();
    return PLDictionary::EMPTY;
}

PropertyList::PropertyList(Context* context) : Resource(context),
    root_(0)
{
}

PropertyList::~PropertyList()
{
    if (root_)
        delete root_;
}

void PropertyList::RegisterObject(Context* context)
{
    context->RegisterFactory<PropertyList>();
}

bool PropertyList::Load(Deserializer& source)
{
    XMLFile xmlFile(context_);
    if (!xmlFile.Load(source))
        return false;

    XMLElement rootElem = xmlFile.GetRoot("plist");
    if (!rootElem)
        return false;

    return LoadXML(rootElem);
}

bool PropertyList::LoadXML(const XMLElement& source, bool setInstanceDefault)
{
    if (source.GetName() != "plist")
        return false;

    if (root_)
        delete root_;

    root_ = ParseDictionary(source.GetChild());

    return true;
}

PLDictionary* PropertyList::ParseDictionary(const XMLElement& element)
{
    PLDictionary* dictionary = new PLDictionary;

    XMLElement child = element.GetChild();
    while (child)
    {
        if (child.GetName() != "key")
        {
            delete dictionary;
            return 0;
        }

        String key = child.GetValue();

        child = child.GetNext();
        if (!child)
        {
            delete dictionary;
            return 0;
        }

        PLNode* value = Parse(child);
        if (!value)
        {
            delete dictionary;
            return 0;
        }

        (*dictionary)[key] = value;

        child = child.GetNext();
    }

    return dictionary;
}

PLArray* PropertyList::ParseArray(const XMLElement& element)
{
    PLArray* array = new PLArray;
    XMLElement child = element.GetChild();

    while (child)
    {
        PLNode* value = Parse(child);
        if (!value)
        {
            delete array;
            return 0;
        }

        array->Push(value);

        child = child.GetNext();
    }

    return array;
}

PLNode* PropertyList::Parse(const XMLElement& element)
{
    String name = element.GetName();

    if (name == "true")
        return new PLValue(true);
    else if (name == "false")
        return new PLValue(false);
    else if (name == "real")
        return new PLValue(ToFloat(element.GetValue()));
    else if (name == "integer")
        return new PLValue(ToInt(element.GetValue()));
    else if (name == "string")
        return new PLValue(element.GetValue());
    else if (name == "array")
        return ParseArray(element);
    else if (name == "dict")
        return ParseDictionary(element);

    LOGERROR("Unknown data type");
    return 0;
}

URHO3D_API IntRect PLStringToIntRect(const String& string)
{
    IntRect result;
    sscanf(string.CString(), "{{%d,%d},{%d,%d}}", &result.left_, &result.top_, &result.right_, &result.bottom_);
    result.right_ += result.left_;
    result.bottom_ += result.top_;

    return result;
}

URHO3D_API IntVector2 PLStringToIntVector2(const String& string)
{
    IntVector2 result;
    sscanf(string.CString(), "{%d,%d}", &result.x_, &result.y_);

    return result;
}

}
