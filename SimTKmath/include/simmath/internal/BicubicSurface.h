#ifndef SimTK_BICUBIC_SURFACE_H_
#define SimTK_BICUBIC_SURFACE_H_

/* -------------------------------------------------------------------------- *
 *                        SimTK Simbody: SimTKmath                            *
 * -------------------------------------------------------------------------- *
 * This is part of the SimTK biosimulation toolkit originating from           *
 * Simbios, the NIH National Center for Physics-Based Simulation of           *
 * Biological Structures at Stanford, funded under the NIH Roadmap for        *
 * Medical Research, grant U54 GM072970. See https://simtk.org.               *
 *                                                                            *
 * Portions copyright (c) 2011 Stanford University and the Authors.           *
 * Authors: Matthew Millard                                                   *
 * Contributors: Michael Sherman                                              *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,    *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR      *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE  *
 * USE OR OTHER DEALINGS IN THE SOFTWARE.                                     *
 * -------------------------------------------------------------------------- */

/** @file
This file defines the BicubicSurface class, and the BicubicFunction class
that uses it to create a two-argument Function object. **/

#include "SimTKcommon.h"
#include "simmath/internal/common.h"

#include <limits>

namespace SimTK { 

//==============================================================================
//                           CLASS BICUBIC SURFACE
//==============================================================================
/** This class will create a smooth surface that approximates a two-argument
function F(X,Y) from a given set of samples of that function on a rectangular
grid with regular or irregular spacing. A bicubic surface interpolation is used
to approximate the function
between the sample points. That is desirable for simulation use because it is 
continuous up to the second derivative, providing smoothly varying first 
derivatives, and a very smooth surface. The third derivatives will be 
discontinuous between grid boundaries; all higher derivatives are zero.

The user need only provide two vectors x and y defining the sample points,
and a matrix f that defines the value of the function at each sample (you can
think of that as the height Z of the surface over the X-Y plane). If the 
samples along both axes are regularly spaced, x and y can be defined just by
giving the spacing; otherwise, the sample locations are given explicitly.

Graphically if these vectors and matrices were laid next to each other 
consistently with how the surface is computed the diagram would look like this:
<pre>
             y(0)       y(1)    ...   y(ny-1)
            ------     ------         --------
    x(0)  |  f(0,0)     f(0,1)  ...   f(0,ny-1)
    x(1)  |  f(1,0)     f(1,1)  ...   f(1,ny-1)
     .    |    .          .              .
     .    |    .          .              .
     .    |    .          .              .
  x(nx-1) | f(nx-1,0)  f(nx-1,1)    f(nx-1,ny-1)
</pre>
such that f(i,j)=F(x(i),y(j)).

Note that the each XY location can only have a unique value associated with it
-- cave-like structures cannot be represented using this interpolation method.
    
Technically a bicubic surface interpolation requires the partial derivatives 
fx, fy and fxy at each of the grid points. To take this burden from the user, 
these partial derivative matrices are computed using only the the supplied 
points for X, Y and F. For the interested reader, these partial derivatives are 
computed by fitting splines through the points provided, and then taking 
derivatives of splines. 

These splines will pass through the points exactly when the smoothness 
parameter of the surface is set to 0, and will be interpolated using natural 
cubic splines, meaning that the curvature will be zero at the boundaries. 
When the smoothness parameter is between 0 and 1, the surface 
will be 'relaxed' using the algorithm used in SplineFitter, and will not
exactly pass through the points given, but will smoothly come close to the 
points. The smoothness parameter can thus
be used to generate a surface that smoothly interpolates noisy surface data.

Here is the Wikipedia entry from which we implemented this method:
http://en.wikipedia.org/wiki/Bicubic_interpolation

@see SplineFitter for implementation notes regarding smoothing. **/
class SimTK_SIMMATH_EXPORT BicubicSurface {
public:   
    class PatchHint; // See below for definition of PatchHint.

    /** Construct an uninitialized BicubicSurface handle. This can be filled
    in later by assignment. **/    
    BicubicSurface() : guts(0) {}
    /** Destructor deletes the underlying surface if there are no more handles
    referencing it, otherwise does nothing. **/
    ~BicubicSurface();
    /** Copy constructor makes a shallow copy of \a source; the new handle
    will reference the same underlying surface as does \a source. **/
    BicubicSurface(const BicubicSurface& source);
    /** Copy assignment is shallow; it makes this handle reference the same 
    underlying surface as does \a source. If this handle was currently 
    referencing a different surface, that surface will be destructed if that
    was the last reference to it. **/
    BicubicSurface& operator=(const BicubicSurface& source);

    
    /** Construct a bicubic surface that approximates f(x,y)
    with the spacing between each grid point in f defined by the vectors x and 
    y. The smoothness parameter controls how closely the surface approaches the 
    grid points specified in matrix f, with the default being that the surface
    will pass exactly through those points.

    @param[in]      x    
        Vector of sample locations along the X axis (minimum 4 values). Must be
        monotonically increasing (no duplicates).
    @param[in]      y    
        Vector of sample locations along the Y axis (minimum 4 values). Must be
        monotonically increasing (no duplicates).            
    @param[in]      f    
        Matrix of function values (or surface heights) evaluated at the grid 
        points formed by x and y (dimension x.size() X y.size()), such that 
        f(i,j) is F(x[i],y[j]) where F is the function being approximated here.
    @param[in]      smoothness 
        A value of 0 will force surface to pass through all of the 
        points in f(x,y). As smoothness tends to 1, the surface will 
        become smoother and smoother, but will not pass through the knot 
        points stored in matrix \a f. (Optional, default is 0.)

    If your sample points are regularly spaced, use the other constructor. **/
    BicubicSurface(const Vector& x, const Vector& y, const Matrix& f, 
                   Real smoothness=0);

    /** Construct a bicubic surface that approximates f(x,y)
    over a grid with regular spacing in both the x and y directions. The 
    smoothness parameter controls how closely the surface approaches the 
    grid points specified in matrix f, with the default being that the surface
    will pass exactly through those points.

    @param[in]      XY           
        A Vec2 giving the (x0,y0) sample location associated with the (0,0) 
        grid position in matrix \a f.
    @param[in]      spacing     
        A Vec2 giving regular spacing along the x and y directions; both 
        entries must be greater than 0. The (i,j)th sample location is then 
        taken to be XY + (i*spacing[0], j*spacing[1]).
    @param[in]      f            
        Matrix of function values (or surface heights) evaluated at points of 
        the x-y plane regularly sampled using the supplied spacings. Can be 
        rectangular but must have minimum dimension 4x4. Here 
        f(i,j)=F(i*spacing[0],j*spacing[1]) where F is the function being 
        approximated.
    @param[in]      smoothness 
        A value of 0 will force surface to pass through all of the 
        points in \a f. As smoothness tends to 1, the surface will 
        become smoother and smoother, but will not pass through the knot 
        points stored in matrix \a f. (Optional, default is 0.)

    If your sample points are not regularly spaced, use the other constructor
    which allows for specified sample points. **/
    BicubicSurface(const Vec2& XY, const Vec2& spacing, 
                   const Matrix& f, Real smoothness=0);

    /** Calculate the value of the surface at a particular XY coordinate.
     
    @param[in]      XY 
        A Vec2 giving the (X,Y) point at which F(X,Y) is to be evaluated.
    @param[in,out]  hint 
        Information saved from an earlier invocation of calcValue()
        or calcDerivative() that is used to reduce execution time. 
    @return The interpolated value of the function at point (X,Y). 

    Cost is minimal for repeated access to the same point, and considerably
    reduced if access is to the same patch. We also take advantage of 
    a regularly-spaced grid if there is one to avoid searching for the right 
    patch. **/
    Real calcValue(const Vec2& XY, PatchHint& hint) const;

    /** This is a slow-but-convenient version of calcValue() since it does 
    not provide for a PatchHint. See the other signature for a much faster
    version. **/
    Real calcValue(const Vec2& XY) const;
    
    /** Calculate a partial derivative of this function at a particular point.  
    Which derivative to take is specified by listing the input components
    (0==x, 1==y) with which to take it. For example, if derivComponents=={0}, 
    that indicates a first derivative with respective to argument x.  
    If derivComponents=={0, 0, 0}, that indicates a third derivative with
    respective to argument x.  If derivComponents=={0, 1}, that indicates 
    a partial second derivative with respect to x and y, that is Df(x,y)/DxDy.
    (We use capital D to indicate partial derivative.)
     
    @param[in]      derivComponents  
        The input components with respect to which the derivative should be 
        taken. Its size must be less than or equal to the  value returned by 
        getMaxDerivativeOrder().      
    @param[in]      XY    
        The vector of two input arguments that define the XY location on the 
        surface.
    @param[in,out   hint 
        Information saved from an earlier invocation of calcValue()
        or calcDerivative() that is used to reduce execution time. 
    @return The interpolated value of the selected function partial derivative
    for arguments (X,Y). 

    See comments in calcValue() for a discussion of cost and how the hint
    is used to reduce the cost. **/
    Real calcDerivative(const Array_<int>& derivComponents, 
                        const Vec2& XY, PatchHint& hint) const;

    /** This is a slow-but-convenient version of calcDerivative() since it
    does not provide for a PatchHint. See the other signature for a much faster
    version. **/
    Real calcDerivative(const Array_<int>& derivComponents, 
                        const Vec2& XY) const;
    
    /** The surface interpolation only works within the grid defined by the 
    vectors x and y used in the constructor. This function check to see if an 
    XYval is within the defined bounds of this particular BicubicSurface.
     
    @param XY   The vector of exactly 2 input arguments that define the XY 
                location on the surface.
    @return \c true if the point is in range, \c false otherwise. 
    
    An attempt to invoke calcValue() or calcDerivative() on an out-of-range
    point will raise an exception; use this method to check first if you 
    are not sure. **/
    bool isSurfaceDefined(const Vec2& XY) const;

    /** Create a mesh that can be used to visualize this surface. The default
    resolution will generate a few triangles per patch. Set \a resolution to
    larger numbers to get a denser mesh. **/
    PolygonalMesh createPolygonalMesh(Real resolution=1) const;

    //--------------------------------------------------------------------------
    /**@name                        Statistics
    This class keeps track of the number of surface accesses made (using
    either calcValue() or calcDerivative(), and how many of those were
    resolved successfully using some or all of the hint information. 
    Methods in this section allow access to those statistics. Note that
    these statistics include accesses from all users of this surface. **/
    /**@{**/
    /** This is the total number of calls made to either calcValue() or
    calcDerivative(). **/
    int getNumAccesses() const;
    /** This is the number of accesses which specified a point whose 
    information was already available in the hint. Note that if different
    information is requested about the point, and that information is not
    already available, we count that as "same patch" but not "same point". 
    These accesses are resolved with essentially no computation. **/
    int getNumAccessesSamePoint() const;
    /** This is the number of accesses which specified a new point on the
    same patch as was already present in the hint, or asked for new information
    about the same point. These accesses are resolved without having to search
    for the patch, and without having to compute patch information. However,
    specific point information still must be calculated. **/
    int getNumAccessesSamePatch() const;
    /** This is the number of accesses which specified on a point that was
    not on the patch currently in the hint, but was close enough that we did
    not have to do a general search. This also applies if the point is on an
    edge since those don't require searching either. So these accesses avoided
    searching, but still required patch and point information to be computed,
    which can be expensive. **/
    int getNumAccessesNearbyPatch() const;
    /** Reset all statistics to zero. Note that statistics are mutable so you
    do not have to have write access to the surface. Any user of this surface
    can reset statistics and we make no attempt to handle simultaneous access
    by multiple threads in any careful manner. **/
    void resetStatistics() const;
    /**@}**/


    //--------------------------------------------------------------------------
    /**@name               Advanced surface constructors
    These constructors assume you have already computed the function values
    and derivatives. Most users should use the constructors that construct
    this information automatically from given data points. **/
    /**@{**/

    /** A constructor for a bicubic surface that sets the partial derivatives 
    of the surface to the values specified by fx, fy, and fxy.

    @param x   vector of X grid points (minimum 4 values)
    @param y   vector of Y grid points (minimum 4 values)
    @param f   matrix of the surface heights evaluated at the grid formed 
                by x and y (minumum 4x4)
    @param fx  matrix of the partial derivative of f w.r.t to x (minumum 4x4)
    @param fy  matrix of the partial derivative of f w.r.t to y (minumum 4x4)
    @param fxy matrix of the partial derivative of f w.r.t to x,y (minumum 4x4)
    */
    BicubicSurface(const Vector& x, const Vector& y, const Matrix& f, 
                   const Matrix& fx, const Matrix& fy, const Matrix& fxy);
    /** Same, but with regular grid spacing. **/
    BicubicSurface(const Vec2& XY, const Vec2& spacing, const Matrix& f, 
                   const Matrix& fx, const Matrix& fy, const Matrix& fxy);
    /**@}**/

    //--------------------------------------------------------------------------
    /**@name                        Bookkeeping
    Methods in this section are administrative and most users will not need
    to use them. **/
    /**@{**/

    /** Return \c true if this is an empty handle meaning that it does not
    currently refer to any surface. This is the state the handle will have
    after default construction or a call to clear(). **/
    bool isEmpty() const {return guts==0;}

    /** Return this handle to its default-constructed state, meaning that
    it will not refer to any surface. If the handle was referencing some
    surface, and that was the last reference to that surface, then the
    surface will be destructed. After a call to clear(), isEmpty() will
    return \c true. **/
    void clear();
    /**@}**/

    /** @cond **/ // Hide from Doxygen.    
    class Guts; // Opaque implementation class.
    const BicubicSurface::Guts& getGuts() const
    {   assert(guts); return *guts; }
    /** @endcond **/
private:
    BicubicSurface::Guts* guts;
};    // END class BicubicSurface



//==============================================================================
//                 CLASS BICUBIC FUNCTION :: PATCH HINT
//==============================================================================
/** This object is used to hold precalculated data about the most recently
accessed patch to accelerate the common case of repeated access to the same
patch or to nearby patches. **/
class SimTK_SIMMATH_EXPORT BicubicSurface::PatchHint {
public:
    /** Creates an empty PatchHint, meaning it contains no meaningful
    hint information. **/
    PatchHint();
    /** Copy an existing PatchHint to create a new one with the same
    contents. If \a source is empty, the new one will be also. **/
    PatchHint(const PatchHint& source);
    /** Set the contents of this PatchHint to be the same as that of
    \a source. If \a source is empty, this one will be empty after the
    assignment. **/  
    PatchHint& operator=(const PatchHint& source);
    /** Destruct this PatchHint. **/
    ~PatchHint();

    /** Return \c true if this object currently contains no meaningful
    hint information. **/
    bool isEmpty() const;
    /** Erase any information currently stored in this %PatchHint. After this
    call isEmpty() will return \c true. **/
    void clear();

    /** @cond **/ // Hide from Doxygen
    class Guts; // Hidden implementation of PatchHint.
    const Guts& getGuts() const {return *guts;}
    Guts&       updGuts()       {return *guts;}
    /** @endcond **/
private:
    Guts* guts;
};



//==============================================================================
//                           CLASS BICUBIC FUNCTION
//==============================================================================

/** This is a two-argument Function built using a shared BicubicSurface and
managing current state to optimize for localized access. Each
distinct use of the BicubicSurface should create its own BicubicFunction,
which is a lightweight wrapper around the BicubicSurface. This allows for
localized access pattern optimization to be effective for each use of the
surface.

<h3>Thread safety</h3>
BicubicFunction is \e not thread-safe, but the underlying BicubicSurface is. 
Each thread should thus have a private BicubicFunction that it uses to access
the shared surface.
**/
class SimTK_SIMMATH_EXPORT BicubicFunction : public Function_<Real> {
public:
    /** Create a BicubicFunction referencing the given BicubicSurface, which
    is shared not copied. **/
    BicubicFunction(const BicubicSurface& surface) : surface(surface) {}

    /** Return a reference to the BicubicSurface object being used by this
    BicubicFunction. **/
    const BicubicSurface& getBicubicSurface() const {return surface;}

    /** Calculate the value of the function at a particular XY coordinate. Note
    that XY must be a vector with only 2 elements in it (because this is a
    2-argument function), anything else will throw an exception. This is the
    required implementation of the Function base class pure virtual.
     
    @param XY the 2-Vector of input arguments X and Y. 
    @return The interpolated value of the function at point (X,Y). **/
    virtual Real calcValue(const Vector& XY) const {
        SimTK_ERRCHK1(XY.size()==2, "BicubicFunction::calcValue()",
        "The argument Vector XY must have exactly 2 elements but had %d.",
        XY.size());        
        return surface.calcValue(Vec2(XY[0],XY[1]), hint); 
    }
    
    /** Calculate a partial derivative of this function at a particular point.  
    Which derivative to take is specified by listing the input components
    (0==x, 1==y) with which to take it. For example, if derivComponents=={0}, 
    that indicates a first derivative with respective to argument x.  
    If derivComponents=={0, 0, 0}, that indicates a third derivative with
    respective to argument x.  If derivComponents=={0, 1}, that indicates 
    a partial second derivative with respect to x and y, that is Df(x,y)/DxDy.
    (We use capital D to indicate partial derivative.)
     
    @param derivComponents  
        The input components with respect to which the derivative should be 
        taken. Each entry must be 0 or 1, and if there are 4 or more entries
        the result will be zero since the surface has only 3 non-zero 
        derivatives.
    @param XY    
        The vector of two input arguments that define the XY location on the 
        surface. 
    @return The interpolated value of the selected function partial derivative
    for arguments (X,Y). **/
    virtual Real calcDerivative(const Array_<int>& derivComponents, 
                                const Vector& XY) const {
        SimTK_ERRCHK1(XY.size()==2, "BicubicFunction::calcDerivative()",
        "The argument Vector XY must have exactly 2 elements but had %d.",
        XY.size());        
        return surface.calcDerivative(derivComponents, Vec2(XY[0],XY[1]), hint); 
    }

    /** This implements the Function base class pure virtual; here it
    always returns 2 (X and Y). **/
    virtual int getArgumentSize() const {return 2;}

    /** This implements the Function base class pure virtual specifying how
    many derivatives can be taken of this function; here it is unlimited.
    However, note that a bicubic surface is continuous up to the second 
    derivative, discontinuous at the third, and zero for any derivatives equal 
    to or higher than the fourth. **/
    virtual int getMaxDerivativeOrder() const 
    {   return std::numeric_limits<int>::max(); }
private:
    BicubicSurface                      surface;
    mutable BicubicSurface::PatchHint   hint;
};



}; //namespace
//=============================================================================
//=============================================================================

#endif  // SimTK_BICUBIC_SURFACE_H_
