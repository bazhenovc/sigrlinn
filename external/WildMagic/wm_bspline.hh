
// Geometric Tools, LLC
// Copyright (c) 1998-2014
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
//
// File Version: 5.0.1 (2010/10/01)

#pragma once

#include <glm/glm.hpp>
#include "WildMagic/wm_memory.hh"

#include <limits>

namespace WM
{

template <typename T>
using Vector3 = glm::tvec3<T, glm::highp>;

#define assertion(X, M) assert((X) && M)

//-----------------------------------------------------------------------------
// BSplineBasis

template <typename Real>
class BSplineBasis
{
public:
    // Default constructor.  The number of control points is n+1 and the
    // indices i for the control points satisfy 0 <= i <= n.  The degree of
    // the curve is d.  The knot array has n+d+2 elements.  Whether uniform
    // or nonuniform knots, it is required that
    //   knot[i] = 0, 0 <= i <= d
    //   knot[i] = 1, n+1 <= i <= n+d+1
    // BSplineBasis enforces these conditions by not exposing SetKnot for the
    // relevant values of i.
    BSplineBasis ();

    // Open uniform or periodic uniform.  The knot array is internally
    // generated with equally spaced elements.  It is required that
    //   knot[i] = (i-d)/(n+1-d), d+1 <= i <= n
    // BSplineBasis enforces these conditions by not exposing SetKnot for the
    // relevant values of i.  GetKnot(j) will return knot[i] for i = j+d+1.
    BSplineBasis (int numCtrlPoints, int degree, bool open);
    void Create (int numCtrlPoints, int degree, bool open);

    // Open nonuniform.  The interiorKnot array must have n-d nondecreasing
    // elements in the interval [0,1].  The values are
    //   knot[i] = interiorKnot[j]
    // with 0 <= j <= n-d-1 and i = j+d+1, so d+1 <= i <= n.  The caller is
    // responsible for interiorKnot if it was dynamically allocated.  An
    // internal copy is made, so to dynamically change knots you must use
    // the SetKnot(j,*) function.
    BSplineBasis (int numCtrlPoints, int degree, const Real* interiorKnot);
    void Create (int numCtrlPoints, int degree, const Real* interiorKnot);

    virtual ~BSplineBasis ();

    int GetNumCtrlPoints () const;
    int GetDegree () const;
    bool IsOpen () const;
    bool IsUniform () const;

    // For a nonuniform spline, the knot[i] are modified by SetKnot(j,value)
    // for j = i+d+1.  That is, you specify j with 0 <= j <= n-d-1, i = j+d+1,
    // and knot[i] = value.  SetKnot(j,value) does nothing for indices outside
    // the j-range or for uniform splines.  GetKnot(j) returns knot[i]
    // regardless of whether the spline is uniform or nonuniform.
    void SetKnot (int j, Real knot);
    Real GetKnot (int j) const;

    // Access basis functions and their derivatives.
    Real GetD0 (int i) const;
    Real GetD1 (int i) const;
    Real GetD2 (int i) const;
    Real GetD3 (int i) const;

    // Evaluate basis functions and their derivatives.
    void Compute (Real t, unsigned int order, int& minIndex, int& maxIndex) const;

protected:
    int Initialize (int numCtrlPoints, int degree, bool open);
    Real** Allocate () const;
    void Deallocate (Real** data);

    // Determine knot index i for which knot[i] <= rfTime < knot[i+1].
    int GetKey (Real& t) const;

    int mNumCtrlPoints;   // n+1
    int mDegree;          // d
    Real* mKnot;          // knot[n+d+2]
    bool mOpen, mUniform;

    // Storage for the basis functions and their derivatives first three
    // derivatives.  The basis array is always allocated by the constructor
    // calls.  A derivative basis array is allocated on the first call to a
    // derivative member function.
    Real** mBD0;          // bd0[d+1][n+d+1]
    mutable Real** mBD1;  // bd1[d+1][n+d+1]
    mutable Real** mBD2;  // bd2[d+1][n+d+1]
    mutable Real** mBD3;  // bd3[d+1][n+d+1]
};

//----------------------------------------------------------------------------
template <typename Real>
BSplineBasis<Real>::BSplineBasis ()
{
}
//----------------------------------------------------------------------------
template <typename Real>
BSplineBasis<Real>::BSplineBasis (int numCtrlPoints, int degree, bool open)
{
    Create(numCtrlPoints, degree, open);
}
//----------------------------------------------------------------------------
template <typename Real>
void BSplineBasis<Real>::Create (int numCtrlPoints, int degree, bool open)
{
    mUniform = true;

    int i, numKnots = Initialize(numCtrlPoints, degree, open);
    Real factor = ((Real)1)/(mNumCtrlPoints - mDegree);
    if (mOpen)
    {
        for (i = 0; i <= mDegree; ++i)
        {
            mKnot[i] = (Real)0;
        }

        for (/**/; i < mNumCtrlPoints; ++i)
        {
            mKnot[i] = (i - mDegree)*factor;
        }

        for (/**/; i < numKnots; ++i)
        {
            mKnot[i] = (Real)1;
        }
    }
    else
    {
        for (i = 0; i < numKnots; ++i)
        {
            mKnot[i] = (i - mDegree)*factor;
        }
    }
}
//----------------------------------------------------------------------------
template <typename Real>
BSplineBasis<Real>::BSplineBasis (int numCtrlPoints, int degree,
    const Real* interiorKnot)
{
    Create(numCtrlPoints, degree, interiorKnot);
}
//----------------------------------------------------------------------------
template <typename Real>
void BSplineBasis<Real>::Create (int numCtrlPoints, int degree,
    const Real* interiorKnot)
{
    mUniform = false;

    int i, numKnots = Initialize(numCtrlPoints, degree, true);
    for (i = 0; i <= mDegree; ++i)
    {
        mKnot[i] = (Real)0;
    }

    for (int j = 0; i < mNumCtrlPoints; ++i, ++j)
    {
        mKnot[i] = interiorKnot[j];
    }

    for (/**/; i < numKnots; ++i)
    {
        mKnot[i] = (Real)1;
    }
}
//----------------------------------------------------------------------------
template <typename Real>
BSplineBasis<Real>::~BSplineBasis ()
{
    delete1(mKnot);
    Deallocate(mBD0);
    Deallocate(mBD1);
    Deallocate(mBD2);
    Deallocate(mBD3);
}
//----------------------------------------------------------------------------
template <typename Real>
int BSplineBasis<Real>::GetNumCtrlPoints () const
{
    return mNumCtrlPoints;
}
//----------------------------------------------------------------------------
template <typename Real>
int BSplineBasis<Real>::GetDegree () const
{
    return mDegree;
}
//----------------------------------------------------------------------------
template <typename Real>
bool BSplineBasis<Real>::IsOpen () const
{
    return mOpen;
}
//----------------------------------------------------------------------------
template <typename Real>
bool BSplineBasis<Real>::IsUniform () const
{
    return mUniform;
}
//----------------------------------------------------------------------------
template <typename Real>
Real BSplineBasis<Real>::GetD0 (int i) const
{
    return mBD0[mDegree][i];
}
//----------------------------------------------------------------------------
template <typename Real>
Real BSplineBasis<Real>::GetD1 (int i) const
{
    return mBD1[mDegree][i];
}
//----------------------------------------------------------------------------
template <typename Real>
Real BSplineBasis<Real>::GetD2 (int i) const
{
    return mBD2[mDegree][i];
}
//----------------------------------------------------------------------------
template <typename Real>
Real BSplineBasis<Real>::GetD3 (int i) const
{
    return mBD3[mDegree][i];
}
//----------------------------------------------------------------------------
template <typename Real>
Real** BSplineBasis<Real>::Allocate () const
{
    int numRows = mDegree + 1;
    int numCols = mNumCtrlPoints + mDegree;
    Real** data = new2<Real>(numCols, numRows);
    memset(data[0], 0, numRows*numCols*sizeof(Real));
    return data;
}
//----------------------------------------------------------------------------
template <typename Real>
void BSplineBasis<Real>::Deallocate (Real** data)
{
    delete2(data);
}
//----------------------------------------------------------------------------
template <typename Real>
int BSplineBasis<Real>::Initialize (int numCtrlPoints, int degree, bool open)
{
    assertion(numCtrlPoints >= 2, "Invalid input\n");
    assertion(1 <= degree && degree <= numCtrlPoints-1, "Invalid input\n");

    mNumCtrlPoints = numCtrlPoints;
    mDegree = degree;
    mOpen = open;

    int numKnots = mNumCtrlPoints + mDegree + 1;
    mKnot = new1<Real>(numKnots);

    mBD0 = Allocate();
    mBD1 = 0;
    mBD2 = 0;
    mBD3 = 0;

    return numKnots;
}
//----------------------------------------------------------------------------
template <typename Real>
void BSplineBasis<Real>::SetKnot (int j, Real value)
{
    if (!mUniform)
    {
        // Access only allowed to elements d+1 <= i <= n.
        int i = j + mDegree + 1;
        if (mDegree + 1 <= i && i <= mNumCtrlPoints)
        {
            mKnot[i] = value;
        }
        else
        {
            assertion(false, "Knot index out of range.\n");
        }
    }
    else
    {
        assertion(false, "Knots cannot be set for uniform splines.\n");
    }
}
//----------------------------------------------------------------------------
template <typename Real>
Real BSplineBasis<Real>::GetKnot (int j) const
{
    // Access only allowed to elements d+1 <= i <= n.
    int i = j + mDegree + 1;
    if (mDegree + 1 <= i && i <= mNumCtrlPoints)
    {
        return mKnot[i];
    }

    assertion(false, "Knot index out of range.\n");
    return std::numeric_limits<Real>::max();
}
//----------------------------------------------------------------------------
template <typename Real>
int BSplineBasis<Real>::GetKey (Real& t) const
{
    if (mOpen)
    {
        // Open splines clamp to [0,1].
        if (t <= (Real)0)
        {
            t = (Real)0;
            return mDegree;
        }
        else if (t >= (Real)1)
        {
            t = (Real)1;
            return mNumCtrlPoints - 1;
        }
    }
    else
    {
        // Periodic splines wrap to [0,1).
        if (t < (Real)0 || t >= (Real)1)
        {
            t -= ::floor(t);
        }
    }


    int i;

    if (mUniform)
    {
        i = mDegree + (int)((mNumCtrlPoints - mDegree)*t);
    }
    else
    {
        for (i = mDegree + 1; i <= mNumCtrlPoints; ++i)
        {
            if (t < mKnot[i])
            {
                break;
            }
        }
        --i;
    }

    return i;
}
//----------------------------------------------------------------------------
template <typename Real>
void BSplineBasis<Real>::Compute (Real t, unsigned int order, int& minIndex,
    int& maxIndex) const
{
    assertion(order <= 3, "Only derivatives to third order supported\n");

    if (order >= 1)
    {
        if (!mBD1)
        {
            mBD1 = Allocate();
        }

        if (order >= 2)
        {
            if (!mBD2)
            {
                mBD2 = Allocate();
            }

            if (order >= 3)
            {
                if (!mBD3)
                {
                    mBD3 = Allocate();
                }
            }
        }
    }

    int i = GetKey(t);
    mBD0[0][i] = (Real)1;

    if (order >= 1)
    {
        mBD1[0][i] = (Real)0;
        if (order >= 2)
        {
            mBD2[0][i] = (Real)0;
            if (order >= 3)
            {
                mBD3[0][i] = (Real)0;
            }
        }
    }

    Real n0 = t - mKnot[i], n1 = mKnot[i+1] - t;
    Real invD0, invD1;
    int j;
    for (j = 1; j <= mDegree; j++)
    {
        invD0 = ((Real)1)/(mKnot[i+j] - mKnot[i]);
        invD1 = ((Real)1)/(mKnot[i+1] - mKnot[i-j+1]);

        mBD0[j][i] = n0*mBD0[j-1][i]*invD0;
        mBD0[j][i-j] = n1*mBD0[j-1][i-j+1]*invD1;

        if (order >= 1)
        {
            mBD1[j][i] = (n0*mBD1[j-1][i] + mBD0[j-1][i])*invD0;
            mBD1[j][i-j] = (n1*mBD1[j-1][i-j+1] - mBD0[j-1][i-j+1])*invD1;

            if (order >= 2)
            {
                mBD2[j][i] = (n0*mBD2[j-1][i] + ((Real)2)*mBD1[j-1][i])*invD0;
                mBD2[j][i-j] = (n1*mBD2[j-1][i-j+1] -
                    ((Real)2)*mBD1[j-1][i-j+1])*invD1;

                if (order >= 3)
                {
                    mBD3[j][i] = (n0*mBD3[j-1][i] +
                        ((Real)3)*mBD2[j-1][i])*invD0;
                    mBD3[j][i-j] = (n1*mBD3[j-1][i-j+1] -
                        ((Real)3)*mBD2[j-1][i-j+1])*invD1;
                }
            }
        }
    }

    for (j = 2; j <= mDegree; ++j)
    {
        for (int k = i-j+1; k < i; ++k)
        {
            n0 = t - mKnot[k];
            n1 = mKnot[k+j+1] - t;
            invD0 = ((Real)1)/(mKnot[k+j] - mKnot[k]);
            invD1 = ((Real)1)/(mKnot[k+j+1] - mKnot[k+1]);

            mBD0[j][k] = n0*mBD0[j-1][k]*invD0 + n1*mBD0[j-1][k+1]*invD1;

            if (order >= 1)
            {
                mBD1[j][k] = (n0*mBD1[j-1][k]+mBD0[j-1][k])*invD0 +
                    (n1*mBD1[j-1][k+1]-mBD0[j-1][k+1])*invD1;

                if (order >= 2)
                {
                    mBD2[j][k] = (n0*mBD2[j-1][k] +
                        ((Real)2)*mBD1[j-1][k])*invD0 +
                        (n1*mBD2[j-1][k+1] - ((Real)2)*mBD1[j-1][k+1])*invD1;

                    if (order >= 3)
                    {
                        mBD3[j][k] = (n0*mBD3[j-1][k] +
                            ((Real)3)*mBD2[j-1][k])*invD0 +
                            (n1*mBD3[j-1][k+1] - ((Real)3)*
                            mBD2[j-1][k+1])*invD1;
                    }
                }
            }
        }
    }

    minIndex = i - mDegree;
    maxIndex = i;
}

//-----------------------------------------------------------------------------
// BSplineVolume

template <typename Real>
class BSplineVolume
{
public:

    // Construction and destruction of an open uniform B-spline volume.  The
    // class will allocate space for the control points.  The caller is
    // responsible for setting the values with the member function
    // ControlPoint.

    BSplineVolume (int numUCtrlPoints, int numVCtrlPoints, int numWCtrlPoints, int uDegree, int vDegree, int wDegree);

    ~BSplineVolume ();

    int GetNumCtrlPoints (int dim) const;
    int GetDegree (int dim) const;

    // Control points may be changed at any time.  If any input index is
    // invalid, the returned point is a vector whose components are all
    // MAX_REAL.
    void SetControlPoint (int uIndex, int vIndex, int wIndex,const Vector3<Real>& ctrlPoint);
    Vector3<Real> GetControlPoint (int uIndex, int vIndex, int wIndex) const;

    // The spline is defined for 0 <= u <= 1, 0 <= v <= 1, and 0 <= w <= 1.
    // The input values should be in this domain.  Any inputs smaller than 0
    // are clamped to 0.  Any inputs larger than 1 are clamped to 1.
    Vector3<Real> GetPosition (Real u, Real v, Real w) const;
    Vector3<Real> GetDerivativeU (Real u, Real v, Real w) const;
    Vector3<Real> GetDerivativeV (Real u, Real v, Real w) const;
    Vector3<Real> GetDerivativeW (Real u, Real v, Real w) const;

    // for array indexing:  i = 0 for u, i = 1 for v, i = 2 for w
    Vector3<Real> GetPosition (Real pos[3]) const;
    Vector3<Real> GetDerivative (int i, Real pos[3]) const;

    // for glm
    Vector3<Real> GetPosition (Vector3<Real> pos) const;
    Vector3<Real> GetDerivative (int i, Vector3<Real> pos) const;

private:
    Vector3<Real>*** mCtrlPoint;  // ctrl[unum][vnum][wnum]
    BSplineBasis<Real> mBasis[3];
};

//----------------------------------------------------------------------------
template <typename Real>
BSplineVolume<Real>::BSplineVolume (int numUCtrlPoints, int numVCtrlPoints,
    int numWCtrlPoints, int uDegree, int vDegree, int wDegree)
{
    assertion(numUCtrlPoints >= 2, "Invalid input\n");
    assertion(1 <= uDegree && uDegree <= numUCtrlPoints - 1,
        "Invalid input\n");
    assertion(numVCtrlPoints >= 2, "Invalid input\n");
    assertion(1 <= vDegree && vDegree <= numVCtrlPoints - 1,
        "Invalid input\n");
    assertion(numWCtrlPoints >= 2, "Invalid input\n");
    assertion(1 <= wDegree && wDegree <= numWCtrlPoints - 1,
        "Invalid input\n");

    mCtrlPoint = new3<Vector3<Real> >(numUCtrlPoints, numVCtrlPoints,
        numWCtrlPoints);
    memset(mCtrlPoint[0][0], 0, numUCtrlPoints*numVCtrlPoints*numWCtrlPoints*
        sizeof(Vector3<Real>));

    mBasis[0].Create(numUCtrlPoints, uDegree, true);
    mBasis[1].Create(numVCtrlPoints, vDegree, true);
    mBasis[2].Create(numWCtrlPoints, wDegree, true);
}
//----------------------------------------------------------------------------
template <typename Real>
BSplineVolume<Real>::~BSplineVolume ()
{
    delete3(mCtrlPoint);
}
//----------------------------------------------------------------------------
template <typename Real>
int BSplineVolume<Real>::GetNumCtrlPoints (int dim) const
{
    return mBasis[dim].GetNumCtrlPoints();
}
//----------------------------------------------------------------------------
template <typename Real>
int BSplineVolume<Real>::GetDegree (int dim) const
{
    return mBasis[dim].GetDegree();
}
//----------------------------------------------------------------------------
template <typename Real>
void BSplineVolume<Real>::SetControlPoint (int uIndex, int vIndex,
    int wIndex, const Vector3<Real>& ctrlPoint)
{
    if (0 <= uIndex && uIndex < mBasis[0].GetNumCtrlPoints()
    &&  0 <= vIndex && vIndex < mBasis[1].GetNumCtrlPoints()
    &&  0 <= wIndex && wIndex < mBasis[2].GetNumCtrlPoints())
    {
        mCtrlPoint[uIndex][vIndex][wIndex] = ctrlPoint;
    }
}
//----------------------------------------------------------------------------
template <typename Real>
Vector3<Real> BSplineVolume<Real>::GetControlPoint (int uIndex, int vIndex,
    int wIndex) const
{
    if (0 <= uIndex && uIndex < mBasis[0].GetNumCtrlPoints()
    &&  0 <= vIndex && vIndex < mBasis[1].GetNumCtrlPoints()
    &&  0 <= wIndex && wIndex < mBasis[2].GetNumCtrlPoints())
    {
        return mCtrlPoint[uIndex][vIndex][wIndex];
    }

    return Vector3<Real>(std::numeric_limits<Real>::max(), std::numeric_limits<Real>::max(),
        std::numeric_limits<Real>::max());
}
//----------------------------------------------------------------------------
template <typename Real>
Vector3<Real> BSplineVolume<Real>::GetPosition (Real u, Real v, Real w) const
{
    int iumin, iumax, ivmin, ivmax, iwmin, iwmax;
    mBasis[0].Compute(u, 0, iumin, iumax);
    mBasis[1].Compute(v, 0, ivmin, ivmax);
    mBasis[2].Compute(w, 0, iwmin, iwmax);

    Vector3<Real> pos = Vector3<Real>(0);
    for (int iu = iumin; iu <= iumax; ++iu)
    {
        Real tmp0 = mBasis[0].GetD0(iu);
        for (int iv = ivmin; iv <= ivmax; ++iv)
        {
            Real tmp1 = mBasis[1].GetD0(iv);
            for (int iw = iwmin; iw <= iwmax; ++iw)
            {
                Real tmp2 = mBasis[2].GetD0(iw);
                Real prod = tmp0*tmp1*tmp2;
                pos += prod*mCtrlPoint[iu][iv][iw];
            }
        }
    }

    return pos;
}
//----------------------------------------------------------------------------
template <typename Real>
Vector3<Real> BSplineVolume<Real>::GetDerivativeU (Real u, Real v, Real w)
    const
{
    int iumin, iumax, ivmin, ivmax, iwmin, iwmax;
    mBasis[0].Compute(u, 1, iumin, iumax);
    mBasis[1].Compute(v, 0, ivmin, ivmax);
    mBasis[2].Compute(w, 0, iwmin, iwmax);

    Vector3<Real> derU = Vector3<Real>(0);
    for (int iu = iumin; iu <= iumax; ++iu)
    {
        Real tmp0 = mBasis[0].GetD1(iu);
        for (int iv = ivmin; iv <= ivmax; ++iv)
        {
            Real tmp1 = mBasis[1].GetD0(iv);
            for (int iw = iwmin; iw <= iwmax; ++iw)
            {
                Real tmp2 = mBasis[2].GetD0(iw);
                Real prod = tmp0*tmp1*tmp2;
                derU += prod*mCtrlPoint[iu][iv][iw];
            }
        }
    }

    return derU;
}
//----------------------------------------------------------------------------
template <typename Real>
Vector3<Real> BSplineVolume<Real>::GetDerivativeV (Real u, Real v, Real w)
    const
{
    int iumin, iumax, ivmin, ivmax, iwmin, iwmax;
    mBasis[0].Compute(u, 0, iumin, iumax);
    mBasis[1].Compute(v, 1, ivmin, ivmax);
    mBasis[2].Compute(w, 0, iwmin, iwmax);

    Vector3<Real> derV = Vector3<Real>(0);
    for (int iu = iumin; iu <= iumax; ++iu)
    {
        Real tmp0 = mBasis[0].GetD0(iu);
        for (int iv = ivmin; iv <= ivmax; ++iv)
        {
            Real tmp1 = mBasis[1].GetD1(iv);
            for (int iw = iwmin; iw <= iwmax; ++iw)
            {
                Real tmp2 = mBasis[2].GetD0(iw);
                Real prod = tmp0*tmp1*tmp2;
                derV += prod*mCtrlPoint[iu][iv][iw];
            }
        }
    }

    return derV;
}
//----------------------------------------------------------------------------
template <typename Real>
Vector3<Real> BSplineVolume<Real>::GetDerivativeW (Real u, Real v, Real w)
    const
{
    int iumin, iumax, ivmin, ivmax, iwmin, iwmax;
    mBasis[0].Compute(u, 0, iumin, iumax);
    mBasis[1].Compute(v, 0, ivmin, ivmax);
    mBasis[2].Compute(w, 1, iwmin, iwmax);

    Vector3<Real> derW = Vector3<Real>(0);
    for (int iu = iumin; iu <= iumax; ++iu)
    {
        Real tmp0 = mBasis[0].GetD0(iu);
        for (int iv = ivmin; iv <= ivmax; ++iv)
        {
            Real tmp1 = mBasis[1].GetD0(iv);
            for (int iw = iwmin; iw <= iwmax; ++iw)
            {
                Real tmp2 = mBasis[2].GetD1(iw);
                Real prod = tmp0*tmp1*tmp2;
                derW += prod*mCtrlPoint[iu][iv][iw];
            }
        }
    }

    return derW;
}
//----------------------------------------------------------------------------
template <typename Real>
Vector3<Real> BSplineVolume<Real>::GetPosition (Real pos[3]) const
{
    return GetPosition(pos[0], pos[1], pos[2]);
}
//----------------------------------------------------------------------------
template <typename Real>
Vector3<Real> BSplineVolume<Real>::GetDerivative (int i, Real pos[3]) const
{
    switch (i)
    {
    case 0:  return GetDerivativeU(pos[0], pos[1], pos[2]);
    case 1:  return GetDerivativeV(pos[0], pos[1], pos[2]);
    case 2:  return GetDerivativeW(pos[0], pos[1], pos[2]);
    }

    assertion(false, "Derivatives larger than order 3 not supported\n");
    return Vector3<Real>(0);
}

//----------------------------------------------------------------------------
template <typename Real>
Vector3<Real> BSplineVolume<Real>::GetPosition (Vector3<Real> pos) const
{
    return GetPosition(pos.x, pos.y, pos.z);
}
//----------------------------------------------------------------------------
template <typename Real>
Vector3<Real> BSplineVolume<Real>::GetDerivative (int i, Vector3<Real> pos) const
{
    switch (i)
    {
    case 0:  return GetDerivativeU(pos.x, pos.y, pos.z);
    case 1:  return GetDerivativeV(pos.x, pos.y, pos.z);
    case 2:  return GetDerivativeW(pos.x, pos.y, pos.z);
    }

    assertion(false, "Derivatives larger than order 3 not supported\n");
    return Vector3<Real>(0);
}

}
