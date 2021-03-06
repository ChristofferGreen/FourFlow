// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright Barend Gehrels 2007-2009, Geodan, Amsterdam, the Netherlands.
// Copyright Bruno Lalande 2008, 2009
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_CONVERT_HPP
#define BOOST_GEOMETRY_ALGORITHMS_CONVERT_HPP

#include <cstddef>

#include <boost/numeric/conversion/cast.hpp>
#include <boost/range/functions.hpp>
#include <boost/range/metafunctions.hpp>

#include <boost/geometry/algorithms/append.hpp>
#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/for_each.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>


/*!
\defgroup convert convert: convert geometries from one type to another
\details Convert from one geometry type to another type,
    for example from BOX to POLYGON
\par Geometries:
- \b point to \b box -> a zero-area box of a point
- \b box to \b ring -> a rectangular ring
- \b box to \b polygon -> a rectangular polygon
- \b ring to \b polygon -> polygon with an exterior ring (the input ring)
- \b polygon to \b ring -> ring, interior rings (if any) are ignored
*/

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace convert {

template
<
    typename Point,
    typename Box,
    std::size_t Index,
    std::size_t Dimension,
    std::size_t DimensionCount
>
struct point_to_box
{
    static inline void apply(Point const& point, Box& box)
    {
        typedef typename coordinate_type<Box>::type coordinate_type;

        set<Index, Dimension>(box,
                boost::numeric_cast<coordinate_type>(get<Dimension>(point)));
        point_to_box
            <
                Point, Box,
                Index, Dimension + 1, DimensionCount
            >::apply(point, box);
    }
};


template
<
    typename Point,
    typename Box,
    std::size_t Index,
    std::size_t DimensionCount
>
struct point_to_box<Point, Box, Index, DimensionCount, DimensionCount>
{
    static inline void apply(Point const& point, Box& box)
    {}
};


}} // namespace detail::convert
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Tag1, typename Tag2,
    std::size_t Dimensions,
    typename Geometry1, typename Geometry2
>
struct convert
{
};


template
<
    typename Tag,
    std::size_t Dimensions,
    typename Geometry1, typename Geometry2
>
struct convert<Tag, Tag, Dimensions, Geometry1, Geometry2>
{
    // Same geometry type -> copy coordinates from G1 to G2
    // Actually: we try now to just copy it
    static inline void apply(Geometry1 const& source, Geometry2& destination)
    {
        destination = source;
    }
};

template
<
    std::size_t Dimensions,
    typename Geometry1, typename Geometry2
>
struct convert<point_tag, point_tag, Dimensions, Geometry1, Geometry2>
{
    static inline void apply(Geometry1 const& source, Geometry2& destination)
    {
        geometry::copy_coordinates(source, destination);
    }
};


template <std::size_t Dimensions, typename Ring1, typename Ring2>
struct convert<ring_tag, ring_tag, Dimensions, Ring1, Ring2>
{
    static inline void apply(Ring1 const& source, Ring2& destination)
    {
        geometry::clear(destination);
        for (typename boost::range_const_iterator<Ring1>::type it
            = boost::begin(source);
            it != boost::end(source);
            ++it)
        {
            typename geometry::point_type<Ring2>::type p;
            geometry::copy_coordinates(*it, p);
            geometry::append(destination, p);
        }
    }
};


template <typename Box, typename Ring>
struct convert<box_tag, ring_tag, 2, Box, Ring>
{
    static inline void apply(Box const& box, Ring& ring)
    {
        // go from box to ring -> add coordinates in correct order
        geometry::clear(ring);
        typename point_type<Box>::type point;

        geometry::assign(point, get<min_corner, 0>(box), get<min_corner, 1>(box));
        geometry::append(ring, point);

        geometry::assign(point, get<min_corner, 0>(box), get<max_corner, 1>(box));
        geometry::append(ring, point);

        geometry::assign(point, get<max_corner, 0>(box), get<max_corner, 1>(box));
        geometry::append(ring, point);

        geometry::assign(point, get<max_corner, 0>(box), get<min_corner, 1>(box));
        geometry::append(ring, point);

        geometry::assign(point, get<min_corner, 0>(box), get<min_corner, 1>(box));
        geometry::append(ring, point);
    }
};


template <typename Box, typename Polygon>
struct convert<box_tag, polygon_tag, 2, Box, Polygon>
{
    static inline void apply(Box const& box, Polygon& polygon)
    {
        typedef typename ring_type<Polygon>::type ring_type;

        convert
            <
                box_tag, ring_tag,
                2, Box, ring_type
            >::apply(box, exterior_ring(polygon));
    }
};


template <typename Point, std::size_t Dimensions, typename Box>
struct convert<point_tag, box_tag, Dimensions, Point, Box>
{
    static inline void apply(Point const& point, Box& box)
    {
        detail::convert::point_to_box
            <
                Point, Box, min_corner, 0, Dimensions
            >::apply(point, box);
        detail::convert::point_to_box
            <
                Point, Box, max_corner, 0, Dimensions
            >::apply(point, box);
    }
};


template <typename Ring, std::size_t Dimensions, typename Polygon>
struct convert<ring_tag, polygon_tag, Dimensions, Ring, Polygon>
{
    static inline void apply(Ring const& ring, Polygon& polygon)
    {
        typedef typename ring_type<Polygon>::type ring_type;
        convert
            <
                ring_tag, ring_tag, Dimensions,
                Ring, ring_type
            >::apply(ring, exterior_ring(polygon));
    }
};


template <typename Polygon, std::size_t Dimensions, typename Ring>
struct convert<polygon_tag, ring_tag, Dimensions, Polygon, Ring>
{
    static inline void apply(Polygon const& polygon, Ring& ring)
    {
        typedef typename ring_type<Polygon>::type ring_type;

        convert
            <
                ring_tag, ring_tag, Dimensions,
                ring_type, Ring
            >::apply(exterior_ring(polygon), ring);
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

/*!
    \brief Converts one geometry to another geometry
    \details The convert algorithm converts one geometry, e.g. a BOX, to another geometry, e.g. a RING. This only
    if it is possible and applicable.
    \ingroup convert
    \tparam Geometry1 first geometry type
    \tparam Geometry2 second geometry type
    \param geometry1 first geometry (source)
    \param geometry2 second geometry (target)
 */
template <typename Geometry1, typename Geometry2>
inline void convert(Geometry1 const& geometry1, Geometry2& geometry2)
{
    concept::check_concepts_and_equal_dimensions<const Geometry1, Geometry2>();

    dispatch::convert
        <
            typename tag<Geometry1>::type,
            typename tag<Geometry2>::type,
            dimension<Geometry1>::type::value,
            Geometry1,
            Geometry2
        >::apply(geometry1, geometry2);
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_CONVERT_HPP
