// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright Barend Gehrels 2007-2009, Geodan, Amsterdam, the Netherlands.
// Copyright Bruno Lalande 2008, 2009
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_MULTI_CORE_RING_TYPE_HPP
#define BOOST_GEOMETRY_MULTI_CORE_RING_TYPE_HPP


#include <boost/range/metafunctions.hpp>

#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/multi/core/tags.hpp>



namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace core_dispatch
{

template <typename MultiPolygon>
struct ring_type<multi_polygon_tag, MultiPolygon>
{
    typedef typename geometry::ring_type
        <
            typename boost::range_value<MultiPolygon>::type
        >::type type;
};




} // namespace core_dispatch
#endif


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_CORE_RING_TYPE_HPP
