// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright Barend Gehrels 2007-2009, Geodan, Amsterdam, the Netherlands.
// Copyright Bruno Lalande 2008, 2009
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_CORE_RADIAN_ACCESS_HPP
#define BOOST_GEOMETRY_CORE_RADIAN_ACCESS_HPP


#include <cstddef>

#include <boost/numeric/conversion/cast.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/cs.hpp>


#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry {


#ifndef DOXYGEN_NO_DETAIL
namespace detail {

template<std::size_t Dimension, typename Geometry>
struct degree_radian_converter
{
    typedef typename coordinate_type<Geometry>::type coordinate_type;

    static inline coordinate_type get(Geometry const& geometry)
    {
        return boost::numeric_cast
            <
                coordinate_type
            >(geometry::get<Dimension>(geometry) * geometry::math::d2r);
    }

    static inline void set(Geometry& geometry, coordinate_type const& radians)
    {
        geometry::set<Dimension>(geometry, boost::numeric_cast
            <
                coordinate_type
            >(radians * geometry::math::r2d));
    }

};


// Default, radian (or any other coordinate system) just works like "get"
template <std::size_t Dimension, typename Geometry, typename DegreeOrRadian>
struct radian_access
{
    typedef typename coordinate_type<Geometry>::type coordinate_type;

    static inline coordinate_type get(Geometry const& geometry)
    {
        return geometry::get<Dimension>(geometry);
    }

    static inline void set(Geometry& geometry, coordinate_type const& radians)
    {
        geometry::set<Dimension>(geometry, radians);
    }
};

// Specialize, any "degree" coordinate system will convert to radian
// but only for dimension 0,1 (so: dimension 2 and heigher are untouched)

template
<
    typename Geometry,
    template<typename> class CoordinateSystem
>
struct radian_access<0, Geometry, CoordinateSystem<degree> >
    : degree_radian_converter<0, Geometry>
{};


template
<
    typename Geometry,
    template<typename> class CoordinateSystem
>
struct radian_access<1, Geometry, CoordinateSystem<degree> >
    : degree_radian_converter<1, Geometry>
{};


} // namespace detail
#endif // DOXYGEN_NO_DETAIL


/*!
    \brief get coordinate value of a point, result is in Radian
    \details Result is in Radian, even if source coordinate system
        is in Degrees
    \return coordinate value
    \ingroup access
    \tparam Dimension dimension
    \tparam Geometry geometry
    \param geometry geometry to get coordinate value from
    \note Only applicable to coordinate systems templatized by units,
        e.g. spherical or geographic coordinate systems
*/
template <std::size_t Dimension, typename Geometry>
inline typename coordinate_type<Geometry>::type get_as_radian(const Geometry& geometry)
{
    return detail::radian_access<Dimension, Geometry,
            typename coordinate_system<Geometry>::type>::get(geometry);
}


/*!
    \brief set coordinate value (in radian) to a point
    \details Coordinate value will be set correctly, if coordinate system of
        point is in Degree, Radian value will be converted to Degree
    \ingroup access
    \tparam Dimension dimension
    \tparam Geometry geometry
    \param geometry geometry to assign coordinate to
    \param radians coordinate value to assign
    \note Only applicable to coordinate systems templatized by units,
        e.g. spherical or geographic coordinate systems
*/
template <std::size_t Dimension, typename Geometry>
inline void set_from_radian(Geometry& geometry,
            const typename coordinate_type<Geometry>::type& radians)
{
    detail::radian_access<Dimension, Geometry,
            typename coordinate_system<Geometry>::type>::set(geometry, radians);
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_CORE_RADIAN_ACCESS_HPP
