
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2014 Srinath Ravichandran, The appleseedhq Organization
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

#ifndef APPLESEED_FOUNDATION_MATH_BEZIERCURVE_H
#define APPLESEED_FOUNDATION_MATH_BEZIERCURVE_H

// appleseed.foundation headers.
#include "foundation/math/aabb.h"
#include "foundation/math/bezier.h"
#include "foundation/math/matrix.h"
#include "foundation/math/minmax.h"
#include "foundation/math/ray.h"
#include "foundation/math/scalar.h"
#include "foundation/math/vector.h"

// Standard headers.
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>

namespace foundation
{

//
// Reference:
//
//   Ray Tracing for Curves Primitive
//   Koji Nakamaru, Yoshio Ohno
//   http://wscg.zcu.cz/wscg2002/Papers_2002/A83.pdf
//

//
// Base class for Bezier curves.
// N is the degree of the curve.
// A curve of degree N has N + 1 control points.
//

template <typename T, size_t N>
class BezierCurveBase
{
  public:
    // Types.
    typedef T ValueType;
    typedef Vector<T, 3> VectorType;
    typedef AABB<T, 3> AABBType;
    typedef Matrix<T, 4, 4> MatrixType;

    // Degree of the Bezier curve.
    static const size_t Degree = N;

    BezierCurveBase()
    {
    }

    BezierCurveBase(const VectorType ctrl_pts[N + 1], const ValueType width)
    {
        assert(width >= ValueType(0.0));

        for (size_t i = 0; i < N + 1; ++i)
        {
            m_ctrl_pts[i] = ctrl_pts[i];
            m_width[i] = width;
        }

        compute_max_width();
        compute_bbox();
    }

    BezierCurveBase(const VectorType ctrl_pts[N + 1], const ValueType width[N + 1])
    {
        for (size_t i = 0; i < N + 1; ++i)
        {
            assert(width[i] >= ValueType(0.0));
            m_ctrl_pts[i] = ctrl_pts[i];
            m_width[i] = width[i];
        }

        compute_max_width();
        compute_bbox();
    }

    BezierCurveBase(const BezierCurveBase& curve, const MatrixType& xfm)
    {
        for (size_t i = 0; i < N + 1; ++i)
        {
            m_ctrl_pts[i] = transform_point(xfm, curve.m_ctrl_pts[i]);
            m_width[i] = curve.m_width[i];
        }

        compute_max_width();
        compute_bbox();
    }

    size_t get_control_point_count() const
    {
        return N + 1;
    }

    const VectorType& get_control_point(const size_t index) const
    {
        assert(index < N + 1);
        return m_ctrl_pts[index];
    }

    ValueType get_width(const size_t index) const
    {
        assert(index < N + 1);
        return m_width[index];
    }

    ValueType get_max_width() const
    {
        return m_max_width;
    }

    const AABBType& get_bbox() const
    {
        return m_bbox;
    }

    size_t compute_max_recursion_depth() const
    {
        if (N < 2)
            return 0;

        ValueType l0 =
            std::max(
                std::abs(m_ctrl_pts[0].x - ValueType(2.0) * m_ctrl_pts[1].x + m_ctrl_pts[2].x),
                std::abs(m_ctrl_pts[0].y - ValueType(2.0) * m_ctrl_pts[1].y + m_ctrl_pts[2].y));

        for (size_t i = 1; i <= N - 2; ++i)
        {
            l0 =
                max(
                    l0,
                    std::abs(m_ctrl_pts[i].x - ValueType(2.0) * m_ctrl_pts[i + 1].x + m_ctrl_pts[i + 2].x),
                    std::abs(m_ctrl_pts[i].y - ValueType(2.0) * m_ctrl_pts[i + 1].y + m_ctrl_pts[i + 2].y));
        }

        const ValueType epsilon = m_max_width * ValueType(0.05);    // 1/20 of max_width
        const ValueType value = (ValueType(SqrtTwo) * N * (N - 1) * l0) / (ValueType(8.0) * epsilon);
        const ValueType RcpLog4 = ValueType(0.7213475204444817);
        const ValueType r0 = std::log(value) * RcpLog4;
        const ValueType clamped_r0 = clamp(r0, ValueType(0.0), ValueType(5.0));

        return truncate<size_t>(clamped_r0);
    }

  protected:
    template <typename>
    friend class BezierCurveIntersector;

    VectorType  m_ctrl_pts[N + 1];      // control points of the curve
    ValueType   m_width[N + 1];         // per-control point widths
    ValueType   m_max_width;            // maximum control point width
    AABBType    m_bbox;                 // bounding box of the curve

    static VectorType transform_point(const MatrixType& xfm, const VectorType& p)
    {
        const Vector<ValueType, 4> pt(p.x, p.y, p.z, ValueType(1.0));
        const Vector<ValueType, 4> xpt = xfm * pt;

        assert(xpt.w != ValueType(0.0));
        const ValueType rcp_w = ValueType(1.0) / xpt.w;

        return VectorType(xpt.x * rcp_w, xpt.y * rcp_w, xpt.z * rcp_w);
    }

    void compute_max_width()
    {
        m_max_width = m_width[0];

        for (size_t i = 1; i < N + 1; ++i)
            m_max_width = std::max(m_max_width, m_width[i]);
    }

    void compute_bbox()
    {
        m_bbox.invalidate();

        for (size_t i = 0; i < N + 1; ++i)
            m_bbox.insert(m_ctrl_pts[i]);

        m_bbox.grow(VectorType(m_max_width * ValueType(0.5)));
        m_bbox.robust_grow(ValueType(1.0e-4));
    }
};


//
// Degree 1 Bezier curve (a straight line).
//

template <typename T>
class BezierCurve1
  : public BezierCurveBase<T, 1>
{
  public:
    typedef BezierCurveBase<T, 1> Base;
    typedef typename Base::ValueType ValueType;
    typedef typename Base::VectorType VectorType;
    typedef typename Base::AABBType AABBType;
    typedef typename Base::MatrixType MatrixType;

    BezierCurve1()
    {
    }

    BezierCurve1(const VectorType ctrl_pts[2], const ValueType width)
      : Base(ctrl_pts, width)
    {
    }

    BezierCurve1(const VectorType ctrl_pts[2], const ValueType width[2])
      : Base(ctrl_pts, width)
    {
    }

    BezierCurve1(const BezierCurve1& curve, const MatrixType& xfm)
      : Base(curve, xfm)
    {
    }

    VectorType evaluate_point(const ValueType t) const
    {
        return interpolate_bezier1(Base::m_ctrl_pts[0], Base::m_ctrl_pts[1], t);
    }

    ValueType evaluate_width(const ValueType t) const
    {
        return interpolate_bezier1(Base::m_width[0], Base::m_width[1], t);
    }

    void split(BezierCurve1& c1, BezierCurve1& c2) const
    {
        const VectorType midpt = evaluate_point(ValueType(0.5));
        const ValueType midw = evaluate_width(ValueType(0.5));

        c1.m_ctrl_pts[0] = Base::m_ctrl_pts[0];
        c1.m_ctrl_pts[1] = midpt;

        c1.m_width[0] = Base::m_width[0];
        c1.m_width[1] = midw;

        c1.compute_max_width();
        c1.compute_bbox();

        c2.m_ctrl_pts[0] = midpt;
        c2.m_ctrl_pts[1] = Base::m_ctrl_pts[1];

        c2.m_width[0] = midw;
        c2.m_width[1] = Base::m_width[1];

        c2.compute_max_width();
        c2.compute_bbox();
    }
};


//
// Degree 2 Bezier curve.
//

template <typename T>
class BezierCurve2
  : public BezierCurveBase<T, 2>
{
  public:
    typedef BezierCurveBase<T, 2> Base;
    typedef typename Base::ValueType ValueType;
    typedef typename Base::VectorType VectorType;
    typedef typename Base::AABBType AABBType;
    typedef typename Base::MatrixType MatrixType;

    BezierCurve2()
    {
    }

    BezierCurve2(const VectorType ctrl_pts[3], const ValueType width)
      : Base(ctrl_pts, width)
    {
    }

    BezierCurve2(const VectorType ctrl_pts[3], const ValueType width[3])
      : Base(ctrl_pts, width)
    {
    }

    BezierCurve2(const BezierCurve2& curve, const MatrixType& xfm)
      : Base(curve, xfm)
    {
    }

    VectorType evaluate_point(const ValueType t) const
    {
        return
            interpolate_bezier2(
                Base::m_ctrl_pts[0],
                Base::m_ctrl_pts[1],
                Base::m_ctrl_pts[2],
                t);
    }

    ValueType evaluate_width(const ValueType t) const
    {
        return
            interpolate_bezier2(
                Base::m_width[0],
                Base::m_width[1],
                Base::m_width[2],
                t);
    }

    void split(BezierCurve2& c1, BezierCurve2& c2) const
    {
        const VectorType midpt = evaluate_point(ValueType(0.5));
        const ValueType midw = evaluate_width(ValueType(0.5));

        c1.m_ctrl_pts[0] = Base::m_ctrl_pts[0];
        c1.m_ctrl_pts[1] = (Base::m_ctrl_pts[0] + Base::m_ctrl_pts[1]) * ValueType(0.5);
        c1.m_ctrl_pts[2] = midpt;

        c1.m_width[0] = Base::m_width[0];
        c1.m_width[1] = (Base::m_width[0] + Base::m_width[1]) * ValueType(0.5);
        c1.m_width[2] = midw;

        c1.compute_max_width();
        c1.compute_bbox();

        c2.m_ctrl_pts[0] = midpt;
        c2.m_ctrl_pts[1] = (Base::m_ctrl_pts[1] + Base::m_ctrl_pts[2]) * ValueType(0.5);
        c2.m_ctrl_pts[2] = Base::m_ctrl_pts[2];

        c2.m_width[0] = midw;
        c2.m_width[1] = (Base::m_width[1] + Base::m_width[2]) * ValueType(0.5);
        c2.m_width[2] = Base::m_width[2];

        c2.compute_max_width();
        c2.compute_bbox();
    }
};


//
// Degree 3 Bezier curve.
//

template <typename T>
class BezierCurve3
  : public BezierCurveBase<T, 3>
{
  public:
    typedef BezierCurveBase<T, 3> Base;
    typedef typename Base::ValueType ValueType;
    typedef typename Base::VectorType VectorType;
    typedef typename Base::AABBType AABBType;
    typedef typename Base::MatrixType MatrixType;

    BezierCurve3()
    {
    }

    BezierCurve3(const VectorType ctrl_pts[4], const ValueType width)
      : Base(ctrl_pts, width)
    {
    }

    BezierCurve3(const VectorType ctrl_pts[4], const ValueType width[4])
      : Base(ctrl_pts, width)
    {
    }

    BezierCurve3(const BezierCurve3& curve, const MatrixType& xfm)
      : Base(curve, xfm)
    {
    }

    VectorType evaluate_point(const ValueType t) const
    {
        return
            interpolate_bezier3(
                Base::m_ctrl_pts[0],
                Base::m_ctrl_pts[1],
                Base::m_ctrl_pts[2],
                Base::m_ctrl_pts[3],
                t);
    }

    ValueType evaluate_width(const ValueType t) const
    {
        return
            interpolate_bezier3(
                Base::m_width[0],
                Base::m_width[1],
                Base::m_width[2],
                Base::m_width[3],
                t);
    }

    void split(BezierCurve3& c1, BezierCurve3& c2) const
    {
        const VectorType midpt = evaluate_point(ValueType(0.5));
        const ValueType midw = evaluate_width(ValueType(0.5));

        const VectorType mc[] =
        {
            (Base::m_ctrl_pts[0] + Base::m_ctrl_pts[1]) * ValueType(0.5),
            (Base::m_ctrl_pts[1] + Base::m_ctrl_pts[2]) * ValueType(0.5),
            (Base::m_ctrl_pts[2] + Base::m_ctrl_pts[3]) * ValueType(0.5)
        };

        const ValueType mw[] =
        {
            (Base::m_width[0] + Base::m_width[1]) * ValueType(0.5),
            (Base::m_width[1] + Base::m_width[2]) * ValueType(0.5),
            (Base::m_width[2] + Base::m_width[3]) * ValueType(0.5)
        };

        c1.m_ctrl_pts[0] = Base::m_ctrl_pts[0];
        c1.m_ctrl_pts[1] = mc[0];
        c1.m_ctrl_pts[2] = (mc[0] + mc[1]) * ValueType(0.5);
        c1.m_ctrl_pts[3] = midpt;

        c1.m_width[0] = Base::m_width[0];
        c1.m_width[1] = mw[0];
        c1.m_width[2] = (mw[0] + mw[1]) * ValueType(0.5);
        c1.m_width[3] = midw;

        c1.compute_max_width();
        c1.compute_bbox();

        c2.m_ctrl_pts[0] = midpt;
        c2.m_ctrl_pts[1] = (mc[1] + mc[2]) * ValueType(0.5);
        c2.m_ctrl_pts[2] = mc[2];
        c2.m_ctrl_pts[3] = Base::m_ctrl_pts[3];

        c2.m_width[0] = midw;
        c2.m_width[1] = (mw[1] + mw[2]) * ValueType(0.5);
        c2.m_width[2] = mw[2];
        c2.m_width[3] = Base::m_width[3];

        c2.compute_max_width();
        c2.compute_bbox();
    }
};


//
// Full specializations for degree 1, 2, 3 Bezier curves of type float and double.
//

typedef BezierCurve1<float>  BezierCurve1f;
typedef BezierCurve1<double> BezierCurve1d;
typedef BezierCurve2<float>  BezierCurve2f;
typedef BezierCurve2<double> BezierCurve2d;
typedef BezierCurve3<float>  BezierCurve3f;
typedef BezierCurve3<double> BezierCurve3d;


//
// Bezier curve intersector.
//

template <typename BezierCurveType>
class BezierCurveIntersector
{
  public:
    typedef typename BezierCurveType::ValueType ValueType;
    typedef typename BezierCurveType::VectorType VectorType;
    typedef typename BezierCurveType::AABBType AABBType;
    typedef typename BezierCurveType::MatrixType MatrixType;
    typedef Ray<ValueType, 3> RayType;

    // Dot product function that only considers the x and y components of the vectors.
    static ValueType dotxy(const VectorType& lhs, const VectorType& rhs)
    {
        return lhs.x * rhs.x + lhs.y * rhs.y;
    }

    // Compute the transformation matrix required for ray-curve intersection.
    static void make_facing_curve_transform(MatrixType& matrix, const RayType& ray)
    {
        // Build the rotation matrix.
        const VectorType rdir = normalize(ray.m_dir);
        const ValueType d = std::sqrt(rdir.x * rdir.x + rdir.z * rdir.z);
        if (d >= ValueType(1.0e-6))
        {
            const ValueType rcp_d = ValueType(1.0) / d;
            matrix[ 0] = rdir.z * rcp_d;             matrix[ 1] = ValueType(0.0);   matrix[ 2] = -rdir.x * rcp_d;            matrix[ 3] = ValueType(0.0);
            matrix[ 4] = -(rdir.x * rdir.y) * rcp_d; matrix[ 5] = d;                matrix[ 6] = -(rdir.y * rdir.z) * rcp_d; matrix[ 7] = ValueType(0.0);
            matrix[ 8] = rdir.x;                     matrix[ 9] = rdir.y;           matrix[10] = rdir.z;                     matrix[11] = ValueType(0.0);
            matrix[12] = ValueType(0.0);             matrix[13] = ValueType(0.0);   matrix[14] = ValueType(0.0);             matrix[15] = ValueType(1.0);
        }
        else
        {
            // We replace the matrix by one that rotates about the x axis by Pi/2.
            // The sign of rotation depends on the sign of the y component of the direction vector.
            const ValueType angle = rdir.y > ValueType(0.0) ? ValueType(HalfPi) : -ValueType(HalfPi);
            matrix = MatrixType::rotation_x(angle);
        }

        // Right-multiply the rotation matrix by a translation matrix.
        matrix[ 3] = -(matrix[ 0] * ray.m_org.x + matrix[ 1] * ray.m_org.y + matrix[ 2] * ray.m_org.z);
        matrix[ 7] = -(matrix[ 4] * ray.m_org.x + matrix[ 5] * ray.m_org.y + matrix[ 6] * ray.m_org.z);
        matrix[11] = -(matrix[ 8] * ray.m_org.x + matrix[ 9] * ray.m_org.y + matrix[10] * ray.m_org.z);
    }

    static bool intersect(
        const BezierCurveType&  curve,
        const RayType&          ray,
        const MatrixType&       xfm,
        ValueType&              t)
    {
        const BezierCurveType xfm_curve(curve, xfm);
        const size_t depth = xfm_curve.compute_max_recursion_depth();

        if (converge(depth, curve, xfm_curve, xfm, ValueType(0.0), ValueType(1.0), t))
        {
            t /= norm(ray.m_dir);
            return true;
        }

        return false;
    }

  private:
    static bool converge(
        const size_t            depth,
        const BezierCurveType&  original_curve,
        const BezierCurveType&  curve,
        const Matrix4f&         xfm,
        const ValueType         v0,
        const ValueType         vn,
        ValueType&              t)
    {
        const AABBType& bbox = curve.get_bbox();
        const ValueType half_width = curve.get_max_width() * ValueType(0.5);

        if (bbox.min.z >= t          || bbox.max.z <= ValueType(1.0e-6) ||
            bbox.min.x >= half_width || bbox.max.x <= -half_width       ||
            bbox.min.y >= half_width || bbox.max.y <= -half_width)
            return false;

        if (depth > 0)
        {
            // Split the curve.
            BezierCurveType c1, c2;
            curve.split(c1, c2);

            // Recurse on the two child curves.
            const ValueType vm = (v0 + vn) * ValueType(0.5);
            return
                converge(depth - 1, original_curve, c1, xfm, v0, vm, t) ||
                converge(depth - 1, original_curve, c2, xfm, vm, vn, t);
        }
        else
        {
            // Compute the intersection.

            const VectorType& cp0 = curve.get_control_point(0);
            const VectorType& cpn = curve.get_control_point(BezierCurveType::Degree);
            const VectorType dir = cpn - cp0;

            VectorType dp0 = curve.get_control_point(1) - cp0;
            if (dotxy(dir, dp0) < ValueType(0.0))
                dp0 = -dp0;

            if (dotxy(dp0, cp0) > ValueType(0.0))
                return false;

            VectorType dpn = cpn - curve.get_control_point(BezierCurveType::Degree - 1);
            if (dotxy(dir, dpn) < ValueType(0.0))
                dpn = -dpn;

            if (dotxy(dpn, cpn) < ValueType(0.0))
                return false;

            // Compute w on the line segment.
            ValueType w = dir.x * dir.x + dir.y * dir.y;
            if (w < ValueType(1.0e-6))
                return false;
            w = -(cp0.x * dir.x + cp0.y * dir.y) / w;
            w = saturate(w);

            // Compute v on the line segment.
            const ValueType v = v0 * (ValueType(1.0) - w) + vn * w;

            // Compute point on original unsplit curve.
            const VectorType orig_p = original_curve.evaluate_point(v);

            // Transform point back to original frame.
            const VectorType p = BezierCurveType::transform_point(xfm, orig_p);

            if (p.z <= ValueType(1.0e-6) || t < p.z)
                return false;

            // Compute the correct interpolated width on the transformed curve and not original curve.
            // Note: We use the modified curve and not the actual curve because the width values are
            // correctly split and interpolated during split operations. In order to have a smooth
            // transition between the control point widths, we use the transformed curve.
            const ValueType half_width = ValueType(0.5) * curve.evaluate_width(w);

            if (p.x * p.x + p.y * p.y >= half_width * half_width)
                return false;

            // Found an intersection.
            t = p.z;
            return true;
        }
    }
};

}       // namespace foundation

#endif  // !APPLESEED_FOUNDATION_MATH_BEZIERCURVE_H