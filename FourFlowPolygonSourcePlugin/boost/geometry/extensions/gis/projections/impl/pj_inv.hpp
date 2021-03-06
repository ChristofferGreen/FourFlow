#ifndef _PROJECTIONS_PJ_INV_HPP
#define _PROJECTIONS_PJ_INV_HPP

// Boost.Geometry (aka GGL, Generic Geometry Library) - projections (based on PROJ4)
// This file is manually converted from PROJ4

// Copyright Barend Gehrels 2007-2009, Geodan, Amsterdam, the Netherlands.
// Copyright Bruno Lalande 2008, 2009
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This file is converted from PROJ4, http://trac.osgeo.org/proj
// PROJ4 is originally written by Gerald Evenden (then of the USGS)
// PROJ4 is maintained by Frank Warmerdam
// PROJ4 is converted to Geometry Library by Barend Gehrels (Geodan, Amsterdam)

// Original copyright notice:

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.


#include <boost/geometry/extensions/gis/projections/impl/adjlon.hpp>
#include <boost/geometry/core/radian_access.hpp>

/* general inverse projection */

namespace boost { namespace geometry { namespace projection {

namespace detail {

namespace inv
{
    static const double EPS = 1.0e-12;
}

 /* inverse projection entry */
template <typename PRJ, typename LL, typename XY, typename PAR>
void pj_inv(const PRJ& prj, const PAR& par, const XY& xy, LL& ll)
{
    /* can't do as much preliminary checking as with forward */
    /* descale and de-offset */
    double xy_x = (geometry::get<0>(xy) * par.to_meter - par.x0) * par.ra;
    double xy_y = (geometry::get<1>(xy) * par.to_meter - par.y0) * par.ra;
    double lon = 0, lat = 0;
    prj.inv(xy_x, xy_y, lon, lat); /* inverse project */
    lon += par.lam0; /* reduce from del lp.lam */
    if (!par.over)
        lon = adjlon(lon); /* adjust longitude to CM */
    if (par.geoc && fabs(fabs(lat)-HALFPI) > inv::EPS)
        lat = atan(par.one_es * tan(lat));

    geometry::set_from_radian<0>(ll, lon);
    geometry::set_from_radian<1>(ll, lat);
}

} // namespace detail
}}} // namespace boost::geometry::projection

#endif
