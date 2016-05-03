//
// Copyright (c) 2008-2016 the Urho3D project.
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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Shader.h"
#include "../Graphics/ShaderVariation.h"
#include "../IO/Deserializer.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"

#include "../DebugNew.h"

namespace Urho3D
{

void ProcessCode(String& code, ShaderType type)
{
    const unsigned COMMENT_START = 0;
    const unsigned COMMENT_END = 1;
    const unsigned NAME = 2;
    static const String voidString("void ");
    static const String names[] = { "VS(", "PS(", "GS(" };
    static const String oglName("main(");
    assert(sizeof(names) == MAX_SHADER_TYPE * sizeof(String));

    unsigned startPos = 0;
    Vector<Pair<unsigned, unsigned> >list;

    for (;;)
    {
        unsigned pos = code.Find(voidString, startPos);
        if (pos == String::NPOS)
            return;
        startPos = pos + voidString.Length();

        bool comment = false;
        for (unsigned i = 0; i < MAX_SHADER_TYPE; ++i)
        {
            if (i != type && code.Compare(startPos, names[i].Length(), names[i]) == 0)
            {
                if (i == type)
                    list.Push(Pair<unsigned, unsigned>(NAME+i, startPos));
                else
                    comment = true;
                break;
            }
        }
        if (!comment)
            continue;

        // Search for the geometry shaders parameter
        unsigned prev = pos;
        while (prev > 0 && code[prev-1] != '\n')
            --prev;
        if (code[prev] == '[')
            pos = prev;

        list.Push(Pair<unsigned, unsigned>(COMMENT_START, pos));

        unsigned braceLevel = 0;
        for (unsigned i = startPos; i < code.Length(); ++i)
        {
            if (code[i] == '{')
                ++braceLevel;
            else if (code[i] == '}')
            {
                --braceLevel;
                if (braceLevel == 0)
                {
                    ++i;
                    list.Push(Pair<unsigned, unsigned>(COMMENT_END, i));
                    startPos = i;
                    break;
                }
            }
        }
    }

    for (Vector<Pair<unsigned, unsigned> >::ConstIterator i = list.End(); i != list.Begin();)
    {
        --i;
        if (i->first_ == COMMENT_START)
            code.Insert(i->second_, "/*");
        else if (i->first_ == COMMENT_END)
            code.Insert(i->second_, "*/");
        else if (i->first_ >= NAME)
            code.Replace(i->second_, names[i->first_ - NAME].Length(), oglName);
    }

#if 0
    unsigned startPos = code.Find(signature);
    unsigned braceLevel = 0;
    if (startPos == String::NPOS)
        return;

    code.Insert(startPos, "/*");

    for (unsigned i = startPos + 2 + signature.Length(); i < code.Length(); ++i)
    {
        if (code[i] == '{')
            ++braceLevel;
        else if (code[i] == '}')
        {
            --braceLevel;
            if (braceLevel == 0)
            {
                code.Insert(i + 1, "*/");
                return;
            }
        }
    }
#endif
}

Shader::Shader(Context* context) :
    Resource(context),
    timeStamp_(0),
    numVariations_(0)
{
    RefreshMemoryUse();
}

Shader::~Shader()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    if (cache)
        cache->ResetDependencies(this);
}

void Shader::RegisterObject(Context* context)
{
    context->RegisterFactory<Shader>();
}

struct CodePoint
{
    enum Point
    {
        INVALID = 0,
        NAME,
        START,
        END
    };

    CodePoint() :
        type(0),
        point(INVALID),
        pos(0)
    {}

    CodePoint(unsigned type, Point point, unsigned pos) :
        type(type),
        point(point),
        pos(pos)
    {}

    // Type of shader code
    unsigned type;
    // Type of point
    Point point;
    // Position
    unsigned pos;
};

void Shader::SieveShadersCode(const String& code)
{
    static const String strVoid("void ");
    static const String strNames[] = { "VS(", "PS(", "GS(" };
    static const String oglName("main(");
    assert(sizeof(strNames) == MAX_SHADER_TYPE * sizeof(String));

    Vector<CodePoint>list;

    // Search for the main shaders functions in the code
    unsigned startPos = 0;
    for (;;)
    {
        unsigned pos = code.Find(strVoid, startPos);
        if (pos == String::NPOS)
            break;
        startPos = pos + strVoid.Length();

        unsigned type;
        for (type = 0; type < MAX_SHADER_TYPE; ++type)
        {
            if (code.Compare(startPos, strNames[type].Length(), strNames[type]) == 0)
                break;
        }
        if (type == MAX_SHADER_TYPE)
            continue;

        // Search for the square brackets parameter of geometry shaders
        if (type == GS && pos > 0)
        {
            unsigned prev = pos - 1;
            while (prev > 0 && code[prev-1] != '\n')
                --prev;
            if (code[prev] == '[')
                pos = prev;
        }

        // Save function start position
        list.Push(CodePoint(type, CodePoint::START, pos));
        // Save function name position
        list.Push(CodePoint(type, CodePoint::NAME, startPos));

        unsigned braceLevel = 0;
        for (unsigned i = startPos; i < code.Length(); ++i)
        {
            if (code[i] == '{')
                ++braceLevel;
            else if (code[i] == '}')
            {
                --braceLevel;
                if (braceLevel == 0)
                {
                    ++i;
                    // Save function end position
                    list.Push(CodePoint(type, CodePoint::END, i));
                    startPos = i;
                    break;
                }
            }
        }
    }

    // Generate single shader source codes by commenting out the others
    for (unsigned type = 0; type < MAX_SHADER_TYPE; ++type)
    {
        String& shaderCode = sourceCode_[type];
        shaderCode = code;
        // From end to start to keep positions valid
        for (Vector<CodePoint>::ConstIterator i = list.End(); i != list.Begin();)
        {
            --i;
            if (i->type != type)
            {
                if (i->point == CodePoint::START)
                    shaderCode.Insert(i->pos, "/*");
                else if (i->point == CodePoint::END)
                    shaderCode.Insert(i->pos, "*/");
            }
#ifdef URHO3D_OPENGL
            // OpenGL: rename the function name to main()
            else if (i->point == CodePoint::NAME)
                shaderCode.Replace(i->pos, strNames[i->type].Length(), oglName);
#endif
        }
    }
}

bool Shader::BeginLoad(Deserializer& source)
{
    Graphics* graphics = GetSubsystem<Graphics>();
    if (!graphics)
        return false;

    // Load the shader source code and resolve any includes
    timeStamp_ = 0;
    String shadersCode;
    if (!ProcessSource(shadersCode, source))
        return false;

    // Screen the code to get the single shader source codes
    SieveShadersCode(shadersCode);

    RefreshMemoryUse();
    return true;
}

bool Shader::EndLoad()
{
    // If variations had already been created, release them and require recompile
    for (unsigned type = 0; type < MAX_SHADER_TYPE; ++type)
    {
        HashMap<StringHash, SharedPtr<ShaderVariation> >& variations = variations_[type];
        for (HashMap<StringHash, SharedPtr<ShaderVariation> >::Iterator i = variations.Begin(); i != variations.End(); ++i)
            i->second_->Release();
    }

    return true;
}

ShaderVariation* Shader::GetVariation(ShaderType type, const String& defines)
{
    return GetVariation(type, defines.CString());
}

ShaderVariation* Shader::GetVariation(ShaderType type, const char* defines)
{
    if (type >= MAX_SHADER_TYPE)
        return 0;
    StringHash definesHash(defines);
    HashMap<StringHash, SharedPtr<ShaderVariation> >& variations(variations_[type]);
    HashMap<StringHash, SharedPtr<ShaderVariation> >::Iterator i = variations.Find(definesHash);
    if (i == variations.End())
    {
        // If shader not found, normalize the defines (to prevent duplicates) and check again. In that case make an alias
        // so that further queries are faster
        String normalizedDefines = NormalizeDefines(defines);
        StringHash normalizedHash(normalizedDefines);

        i = variations.Find(normalizedHash);
        if (i != variations.End())
            variations.Insert(MakePair(definesHash, i->second_));
        else
        {
            // No shader variation found. Create new
            i = variations.Insert(MakePair(normalizedHash, SharedPtr<ShaderVariation>(new ShaderVariation(this, type))));
            if (definesHash != normalizedHash)
                variations.Insert(MakePair(definesHash, i->second_));

            i->second_->SetName(GetFileName(GetName()));
            i->second_->SetDefines(normalizedDefines);
            ++numVariations_;
            RefreshMemoryUse();
        }
    }

    return i->second_;
}

bool Shader::ProcessSource(String& code, Deserializer& source)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    // If the source if a non-packaged file, store the timestamp
    File* file = dynamic_cast<File*>(&source);
    if (file && !file->IsPackaged())
    {
        FileSystem* fileSystem = GetSubsystem<FileSystem>();
        String fullName = cache->GetResourceFileName(file->GetName());
        unsigned fileTimeStamp = fileSystem->GetLastModifiedTime(fullName);
        if (fileTimeStamp > timeStamp_)
            timeStamp_ = fileTimeStamp;
    }

    // Store resource dependencies for includes so that we know to reload if any of them changes
    if (source.GetName() != GetName())
        cache->StoreResourceDependency(this, source.GetName());

    while (!source.IsEof())
    {
        String line = source.ReadLine();

        if (line.StartsWith("#include"))
        {
            String includeFileName = GetPath(source.GetName()) + line.Substring(9).Replaced("\"", "").Trimmed();

            SharedPtr<File> includeFile = cache->GetFile(includeFileName);
            if (!includeFile)
                return false;

            // Add the include file into the current code recursively
            if (!ProcessSource(code, *includeFile))
                return false;
        }
        else
        {
            code += line;
            code += "\n";
        }
    }

    // Finally insert an empty line to mark the space between files
    code += "\n";

    return true;
}

String Shader::NormalizeDefines(const String& defines)
{
    Vector<String> definesVec = defines.ToUpper().Split(' ');
    Sort(definesVec.Begin(), definesVec.End());
    return String::Joined(definesVec, " ");
}

void Shader::RefreshMemoryUse()
{
    unsigned used = sizeof(Shader) + numVariations_ * sizeof(ShaderVariation);
    for (unsigned type = 0; type < MAX_SHADER_TYPE; ++type)
        used += sourceCode_[type].Length();
    SetMemoryUse(used);
}

}
