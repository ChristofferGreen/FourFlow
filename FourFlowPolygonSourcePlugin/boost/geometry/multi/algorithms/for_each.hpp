// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright Barend Gehrels 2007-2009, Geodan, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_FOR_EACH_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_FOR_EACH_HPP


#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/core/is_multi.hpp>
#include <boost/geometry/multi/core/point_type.hpp>


#include <boost/geometry/algorithms/for_each.hpp>



namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace for_each {

// Implementation of multi, for both point and segment,
// just calling the single version.
template
<
    typename MultiGeometry,
    typename Functor,
    bool IsConst,
    typename Policy
>
struct for_each_multi
{
    static inline Functor apply(
                    typename add_const_if_c<IsConst, MultiGeometry>::type& multi,
                    Functor f)
    {
        typedef typename range_iterator_const_if_c
            <
                IsConst,
                MultiGeometry
            >::type iterator_type;

        for(iterator_type it = boost::begin(multi);
            it != boost::end(multi);
            ++it)
        {
            f = Policy::apply(*it, f);
        }
        return f;
    }
};


}} // namespace detail::for_each
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename MultiTag,
    typename MultiGeometry,
    typename Functor,
    bool IsConst
>
struct for_each_point<MultiTag, true, MultiGeometry, Functor, IsConst>
    : detail::for_each::for_each_multi
        <
            MultiGeometry,
            Functor,
            IsConst,
            // Specify the dispatch of the single-version as policy
            for_each_point
                <
                    typename single_tag<MultiTag>::type,
                    false,
                    typename boost::range_value<MultiGeometry>::type,
                    Functor,
                    IsConst
                >
        >
{};


template
<
    typename MultiTag,
    typename MultiGeometry,
    typename Functor,
    bool IsConst
>
struct for_each_segment<MultiTag, true, MultiGeometry, Functor, IsConst>
    : detail::for_each::for_each_multi
        <
            MultiGeometry,
            Functor,
            IsConst,
            // Specify the dispatch of the single-version as policy
            for_each_segment
                <
                    typename single_tag<MultiTag>::type,
                    false,
                    typename boost::range_value<MultiGeometry>::type,
                    Functor,
                    IsConst
                >
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_FOR_EACH_HPP
