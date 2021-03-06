
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2016 Francois Beaune, The appleseedhq Organization
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

#ifndef APPLESEED_RENDERER_KERNEL_LIGHTING_SCATTERINGMODE_H
#define APPLESEED_RENDERER_KERNEL_LIGHTING_SCATTERINGMODE_H

// appleseed.renderer headers.
#include "renderer/modeling/scene/visibilityflags.h"

// Standard headers.
#include <cassert>

namespace renderer
{

//
// All possible scattering modes.
//

class ScatteringMode
{
  public:
    enum Mode
    {
        Absorption  = 0,
        Diffuse     = 1 << 0,
        Glossy      = 1 << 1,
        Specular    = 1 << 2,
        All         = Diffuse | Glossy | Specular
    };

    // Test for the presence of specific scattering modes.
    static bool has_diffuse(const int modes);
    static bool has_glossy(const int modes);
    static bool has_specular(const int modes);
    static bool has_diffuse_or_glossy(const int modes);
    static bool has_glossy_or_specular(const int modes);

    // Determine the appropriate visibility type for a given scattering mode.
    static VisibilityFlags::Type get_vis_flags(const Mode mode);
};


//
// ScatteringMode class implementation.
//

inline bool ScatteringMode::has_diffuse(const int modes)
{
    return (modes & Diffuse) != 0;
}

inline bool ScatteringMode::has_glossy(const int modes)
{
    return (modes & Glossy) != 0;
}

inline bool ScatteringMode::has_specular(const int modes)
{
    return (modes & Specular) != 0;
}

inline bool ScatteringMode::has_diffuse_or_glossy(const int modes)
{
    return (modes & (Diffuse | Glossy)) != 0;
}

inline bool ScatteringMode::has_glossy_or_specular(const int modes)
{
    return (modes & (Glossy | Specular)) != 0;
}

inline VisibilityFlags::Type ScatteringMode::get_vis_flags(const Mode mode)
{
    switch (mode)
    {
      case Diffuse:
        return VisibilityFlags::DiffuseRay;

      case Glossy:
        return VisibilityFlags::GlossyRay;

      case Specular:
        return VisibilityFlags::SpecularRay;

      default:
        assert(!"Invalid scattering mode.");
        return VisibilityFlags::DiffuseRay;
    }
}

}       // namespace renderer

#endif  // !APPLESEED_RENDERER_KERNEL_LIGHTING_SCATTERINGMODE_H
