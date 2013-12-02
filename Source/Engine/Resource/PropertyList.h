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

class PLArray;
class PLDictionary;
class XMLElement;

class URHO3D_API PLNode
{
public:
    /// Node type.
    enum NodeType
    {
        NT_NONE = 0,
        NT_VALUE,
        NT_ARRAY,
        NT_DICTIONARY,
    };

    /// Constructor.
    PLNode(NodeType type);
    virtual ~PLNode();

    /// Return bool.
    virtual bool GetBool() const;
    /// Return float.
    virtual float GetFloat() const;
    /// Return int.
    virtual int GetInt() const;
    /// Return string.
    virtual const String& GetString() const;
    /// Convert to array.
    virtual const PLArray& ToArray() const;
    /// Convert to dictionary.
    virtual const PLDictionary& ToDictionary() const;

    /// Return node type.
    NodeType GetType() const { return nodeType_; }

private:
    /// Node type.
    NodeType nodeType_;
};

class URHO3D_API PLValue : public PLNode
{
public:
    PLValue();
    PLValue(bool value);
    PLValue(float value);
    PLValue(int value);
    PLValue(const String& value);
    virtual ~PLValue();

    /// To bool.
    virtual bool GetBool() const;
    /// To float.
    virtual float GetFloat() const;
    /// To int.
    virtual int GetInt() const;
    /// To string.
    virtual const String& GetString() const;

    /// Return value type.
    VariantType GetValueType() const { return valueType_; }

    /// Empty value.
    static const PLValue EMPTY;

private:
    /// Value type.
    VariantType valueType_;
    /// Value.
    union
    {
        bool bool_;
        float float_;
        int int_;
        String* string_;
    };
};

class URHO3D_API PLArray : public PLNode, public Vector<PLNode*>
{
public:
    PLArray();
    virtual ~PLArray();

    /// To array.
    virtual const PLArray& ToArray() const;

    /// Return bool.
    bool GetBool(unsigned index, bool defValue = false) const;
    /// Return float.
    float GetFloat(unsigned index, float defValue = 0.0f) const;
    /// Return int.
    int GetInt(unsigned index, int defValue = 0) const;
    /// Return string.
    const String& GetString(unsigned index, const String& defValue = "") const;
    /// Return array.
    const PLArray& GetArray(unsigned index) const;
    /// Return dictionary.
    const PLDictionary& GetDictionary(unsigned index) const;

    /// Empty array.
    static const PLArray EMPTY;
};

class URHO3D_API PLDictionary : public PLNode, public HashMap<String, PLNode*>
{
public:
    PLDictionary();
    virtual ~PLDictionary();

    /// To dictionary.
    virtual const PLDictionary& ToDictionary() const;

    /// Return bool.
    bool GetBool(const String& key, bool defValue = false) const;
    /// Return float.
    float GetFloat(const String& key, float defValue = 0.0f) const;
    /// Return int.
    int GetInt(const String& key, int defValue = 0) const;
    /// Return string.
    const String& GetString(const String& key, const String& defValue = "") const;
    /// Return array.
    const PLArray& GetArray(const String& key) const;
    /// Return dictionary.
    const PLDictionary& GetDictionary(const String& key) const;

    /// Empty dictionary.
    static const PLDictionary EMPTY;
};

/// %Property list resource.
class URHO3D_API PropertyList : public Resource
{
    OBJECT(PropertyList);

public:
    /// Construct empty.
    PropertyList(Context* context);
    /// Destruct.
    virtual ~PropertyList();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Load resource. Return true if successful.
    virtual bool Load(Deserializer& source);
    /// Load from XML data.
    virtual bool LoadXML(const XMLElement& source, bool setInstanceDefault = false);

    /// Return root.
    const PLDictionary& GetRoot() const { return *root_; }


private:
    /// Parse proeprty list dictionary node.
    PLDictionary* ParseDictionary(const XMLElement& element);
    /// Parse property list array node.
    PLArray* ParseArray(const XMLElement& element);
    /// Parse property list node.
    PLNode* Parse(const XMLElement& element);

    /// Root.
    PLDictionary* root_;
};

URHO3D_API void PLStringToIntXY(const String& string, int& x, int& y);
URHO3D_API void PLStringToIntXYWH(const String& string, int& x, int& y, int& w, int& h);

}
