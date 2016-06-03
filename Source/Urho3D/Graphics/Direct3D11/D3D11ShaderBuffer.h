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

#pragma once

#include "../../Container/ArrayPtr.h"
#include "../../Core/Object.h"
#include "../../Graphics/GPUObject.h"
#include "../../Graphics/GraphicsDefs.h"

namespace Urho3D
{

/// Hardware shader buffer.
class URHO3D_API ShaderBuffer : public Object, public GPUObject
{
    URHO3D_OBJECT(ShaderBuffer, Object)

public:
    static const unsigned BUFFER_CONSTANT      = 0x0001;
    static const unsigned BUFFER_STRUCTURED    = 0x0002;
    static const unsigned BUFFER_RAW           = 0x0004;
    static const unsigned BUFFER_CPU_READ      = 0x0008;
    static const unsigned BUFFER_READ          = 0x0010;
    static const unsigned BUFFER_WRITE         = 0x0020;

    /// Construct.
    ShaderBuffer(Context* context, unsigned flags = 0);
    /// Destruct.
    virtual ~ShaderBuffer();

    /// Release buffer.
    virtual void Release();

    /// Return buffer flags.
    void SetFlags(unsigned flags) { usage_ = flags; }
    /// Copy description of another buffer, Apply does the effective copy.
    bool SetSize(ShaderBuffer* source);
    /// Set size and create GPU-side buffer. Return true on success.
    bool SetSize(unsigned size, unsigned count, const void* data = 0);
    /// Copy elements in the buffer and mark it dirty.
    void SetElements(unsigned offset, unsigned count, const void* data);
    /// Copy raw bytes in the buffer and mark it dirty.
    void SetBufferData(unsigned offset, unsigned size, const void* data);
    /// Apply to GPU.
    void Apply();
    /// Enable CPU read of the buffer, disable GPU access.
    void* Map();
    /// Disable CPU read of the buffer, enables GPU access.
    void Unmap();

    /// Return buffer usage flags.
    unsigned GetUsage() const { return usage_; }
    /// Return number of elements in the buffer.
    unsigned GetElementCount() const { return count_; }
    /// Return size in bytes of an element.
    unsigned GetElementSize() const { return size_; }
    /// Return size in bytes of the entire buffer.
    unsigned GetBufferSize() const { return size_*count_; }
    /// Return whether has unapplied data.
    bool IsDirty() const { return dirty_; }
    /// Get raw buffer pointer.
    void* GetBuffer();
    /// Get shader resource view for an input buffer.
    void* GetShaderResourceView() const { return shaderResourceView_; }
    /// Get unordered access view for an output buffer.
    void* GetUnorderedAccessView() const { return unorderedAccessView_; }

private:
    /// Create buffer and its views.
    bool CreateBufferAndViews(const void* data);

    /// Buffer usage flags.
    unsigned usage_;
    /// Buffer elements count.
    unsigned count_;
    /// Element byte size.
    unsigned size_;
    /// Dirty flag.
    bool dirty_;
    /// Shader resource view for reading the buffer.
    void* shaderResourceView_;
    /// Unordered access view for writing the buffer.
    void* unorderedAccessView_;
    /// Pointer to the source buffer.
    WeakPtr<ShaderBuffer> sourceBuffer_;
    /// Shadow data.
    SharedArrayPtr<unsigned char> shadowData_;
};

}
