/*    Copyright (c) 2010-2016, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 */

#ifndef TUDAT_COMPOSITEEPHEMERIS_H
#define TUDAT_COMPOSITEEPHEMERIS_H

#include <iostream>
#include <vector>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <Eigen/Core>

#include "Tudat/Astrodynamics/Ephemerides/ephemeris.h"

#include "Tudat/Astrodynamics/Ephemerides/rotationalEphemeris.h"
#include "Tudat/Astrodynamics/Ephemerides/constantEphemeris.h"


namespace tudat
{

namespace ephemerides
{


//! Class that combines a series of translational and rotational ephemeris functions
/*!
 *  Class that combines a series of translational and rotational ephemeris functions to yield a
 *  single translational ephemeris.  By using this class, a single object can be used for calling a
 *  series of translations and rotations.  The option is provided for both adding or subtracting a
 *  given translational state.
 */
template< typename TimeType = double, typename StateScalarType = double >
class CompositeEphemeris : public Ephemeris
{
public:

    using Ephemeris::getCartesianLongStateFromEphemeris;
    using Ephemeris::getCartesianStateFromEphemeris;

    typedef Eigen::Matrix< StateScalarType, 6, 1 > StateType;

    //! Constructor from series of translational and rotational ephemeris functions.
    /*!
     *  Constructor from series of translational and rotational ephemeris functions. Input is
     *  provided as maps, with key being the index providing the position in the order where the
     *  given function should be applied. Each index must be present only once (i.e. either in
     *  translation or rotation), must start with 0 (which must be a translational ephemeris), and
     *  must be increasing by 1.
     *  \param translationalEphemerides List of translational ephemerides.
     *  \param rotationalEphemerides List of rotational ephemerides.
     *  \param referenceFrameOrigin Origin of reference frame in which state is defined.
     *  \param referenceFrameOrientation Orientation of reference frame in which state is defined.
     */
    CompositeEphemeris(
            const std::map< int, boost::function< StateType( const TimeType& ) > >
                translationalEphemerides,
            const std::map< int, boost::function< StateType( const double, const StateType& ) > >
                rotationalEphemerides,
            const std::string referenceFrameOrigin = "SSB",
            const std::string referenceFrameOrientation = "ECLIPJ2000" ):
        Ephemeris( referenceFrameOrigin, referenceFrameOrientation )
    {
        // Create iterators over ephemeris functions.
        typename std::map< int, boost::function< StateType( const TimeType& ) > >::const_iterator
                translationIterator = translationalEphemerides.begin( );
        typename std::map< int, boost::function< StateType( const double, const StateType& ) > >::const_iterator
                rotationIterator = rotationalEphemerides.begin( );

        // Check whether chain starts with translation.
        if( translationIterator->first != 0 )
        {
            std::string errorMessage = "Error, composite ephemeris must start with translation";
            throw std::runtime_error( errorMessage );
        }

        // Run over all indices and set order.
        int currentIndex = 0;
        while( currentIndex < static_cast< int >( translationalEphemerides.size( )
                                                  + rotationalEphemerides.size( ) ) )
        {
            // If current ephemeris is translational, add to translations list (set as addition) and
            // set translation flag at current index to true.
            if( translationIterator != translationalEphemerides.end( )
                && translationIterator->first == currentIndex )
            {
                translationalEphemerides_.push_back( std::make_pair( translationIterator->second, 1 ) );
                isCurrentEphemerisTranslational_.push_back( 1 );
                translationIterator++;
            }
            // If current ephemeris is rotational, add to rotations list and set translation flag at
            // current index to false.
            else if( rotationIterator != rotationalEphemerides.end( )
                     && rotationIterator->first == currentIndex )
            {
                rotationalEphemerides_.push_back( rotationIterator->second );
                isCurrentEphemerisTranslational_.push_back( 0 );
                rotationIterator++;
            }
            // If index is not found, display error message.
            else
            {
                std::string errorMessage = "Error when  making composite ephemeris, input indices inconsistentn";
                throw std::runtime_error( errorMessage );
            }
            currentIndex++;
        }

    }

    //! Constructor from series of translational and rotational ephemeris functions
    /*!
     *  Constructor from series of translational and rotational ephemeris functions, allowing either
     *  addition or subtraction of translation ep Input is provided as maps, with key being the
     *  index providing the position in the order where the given function should be applied. Each
     *  index must be present only once (i.e. either in translation or rotation), must start with 0
     *  (which must be a translational ephemeris), and must be increasing by 1.  The second element
     *  of the pair that is the translational map value denotes whether to add (1) or subtract(0)
     *  it.
     *  \param translationalEphemerides List of translational ephemerides, with subtraction/addition indicator.
     *  \param rotationalEphemerides List of rotational ephemerides.
     *  \param referenceFrameOrigin Origin of reference frame in which state is defined.
     *  \param referenceFrameOrientation Orientation of reference frame in which state is defined.
     */
    CompositeEphemeris(
            const std::map< int, std::pair< boost::function< StateType( const TimeType& ) >, bool > >
                translationalEphemerides,
            const std::map< int, boost::function< StateType( const double, const StateType& ) > >
                rotationalEphemerides,
            const std::string referenceFrameOrigin = "SSB",
            const std::string referenceFrameOrientation = "ECLIPJ2000" ):
        Ephemeris( referenceFrameOrigin, referenceFrameOrientation )
    {
        // Create iterators over ephemeris functions.
        typename std::map< int, std::pair< boost::function< StateType( const TimeType& ) >, bool > >
            ::const_iterator translationIterator = translationalEphemerides.begin( );
        typename std::map< int, boost::function< StateType( const double, const StateType& ) > >
            ::const_iterator rotationIterator = rotationalEphemerides.begin( );

        // Check whether chain starts with translation.
        if( translationIterator->first != 0 )
        {
            std::string errorMessage = "Error, composite ephemeris must start with translation";
            throw std::runtime_error( errorMessage );
        }

        // Run over all indices and set order.
        int currentIndex = 0;
        while( currentIndex < static_cast< int >( translationalEphemerides.size( )
                                                  + rotationalEphemerides.size( ) ) )
        {
            // If current ephemeris is translational, add to translations list and set translation
            // flag at current index to true.
            if( translationIterator != translationalEphemerides.end( )
                && translationIterator->first == currentIndex )
            {
                int addCurrentEphemeris = ( translationIterator->second.second == true ) ? 1 : -1;

                translationalEphemerides_.push_back( std::make_pair( translationIterator->second.first,
                                                                     addCurrentEphemeris ) );
                isCurrentEphemerisTranslational_.push_back( 1 );
                translationIterator++;
            }
            // If current ephemeris is rotational, add to rotations list and set translation flag at
            // current index to false.
            else if( rotationIterator != rotationalEphemerides.end( )
                     && rotationIterator->first == currentIndex )
            {
                rotationalEphemerides_.push_back( rotationIterator->second );
                isCurrentEphemerisTranslational_.push_back( 0 );
                rotationIterator++;
            }
            // If index is not found, display error message.
            else
            {
                std::string errorMessage = "Error when  making composite ephemeris, input indices inconsistent";
                throw std::runtime_error( errorMessage );
            }
            currentIndex++;
        }
    }

    //! Destructor
    ~CompositeEphemeris( ){ }

    //! Get state from ephemeris.
    /*!
     * Returns state from ephemeris at given time.
     * \param secondsSinceEpoch Seconds since epoch at which ephemeris is to be evaluated.
     * \param julianDayAtEpoch Reference epoch in Julian day.
     * \return Constant state given by combined rotations and translations.
     */
    basic_mathematics::Vector6d getCartesianStateFromEphemeris(
            const double secondsSinceEpoch,
            const double julianDayAtEpoch = basic_astrodynamics::JULIAN_DAY_ON_J2000 );

    //! Get state from ephemeris (with long double as state scalar).
    /*!
     * Returns state from ephemeris with long double as state scalar at given time.
     * \param secondsSinceEpoch Seconds since epoch at which ephemeris is to be evaluated.
     * \param julianDayAtEpoch Reference epoch in Julian day.
     * \return Constant state with long double as state scalar given by combined rotations and translations.
     */
    Eigen::Matrix< long double, 6, 1 > getCartesianLongStateFromEphemeris(
                const double secondsSinceEpoch,
                const double julianDayAtEpoch = basic_astrodynamics::JULIAN_DAY_ON_J2000 );

    //! Add an additional translational ephemeris at the end of the chain.
    /*!
     *  Function to add an additional translational ephemeris at the end of the chain.
     *  \param stateFunction Translational ephemeris function to add.
     *  \param add Identifier setting whether to add (1) or subtract (-1) translation.
     */
    void addTranslationalEphemeris( const boost::function< StateType( const TimeType& ) > stateFunction,
                                    const int add = 1 )
    {
        //Check validity of input.
        if( add != 1 && add != -1 )
        {
            std::string errorMessage = "Error when adding to composite ephemeris";
            throw std::runtime_error( errorMessage );
        }

        // Add translational ephemeris to end of chain
        isCurrentEphemerisTranslational_.push_back( 1 );
        translationalEphemerides_.push_back( std::make_pair( stateFunction, add ) );
    }

private:

    //! Vector of translational ephemeris functions.
    /*!
     *  Vector of translational ephemeris functions and addition (1) or subtraction (-1) indicator.
     */
    std::vector< std::pair< boost::function< StateType( const TimeType& ) >, int > > translationalEphemerides_;

    //! Vector of rotational ephemeris functions.
    /*!
     *  Vector of rotational ephemeris functions.
     */
    std::vector< boost::function< StateType( const double, const StateType& ) > > rotationalEphemerides_;

    //! Vector indicating order of translational and rotational ephemeris.
    /*!
     *  Vector indicating order of translational and rotational ephemeris (0 is first, highest is last)
     */
    std::vector< bool > isCurrentEphemerisTranslational_;
};

//! Interface function used to change the state scalar type of the output of a state function
/*!
 *  Interface function used to change the state scalar type of the output of a state function
 *  \param originalStateFunction State function with original scalar type
 *  \param currentTime Time at which state function is to be evaluated.
 *  \return State from originalStateFunction at currentTime, cast to NewStateScalarType.
 */
template< typename OldStateScalarType, typename NewStateScalarType, typename TimeType, int StateSize >
Eigen::Matrix< NewStateScalarType, StateSize, 1 > convertStateFunctionStateScalarOutput(
        const boost::function< Eigen::Matrix< OldStateScalarType, StateSize, 1 >( const double& ) >
            originalStateFunction,
        const TimeType currentTime )
{
    return originalStateFunction( currentTime ).template cast< NewStateScalarType >( );
}

//! Function to create the state function of a reference point.
/*!
 *  Function to create the state function of a reference point, for instance the state of a ground station in a
 *  barycentric frame.
 *  \param bodyEphemeris Global ephemeris of body on which reference point is located
 *  \param bodyRotationModel Rotation model between global and body-fixed frame
 *  \param referencePointRelativeStateFunction Function returning the body-fixed state of the reference point as
 *  a function of time
 *  \return Ephemeris describbing the state of the local reference point in the global frame (expressed global frame's
 *  coordinates, i.e. same as bodyEphemeris).
 */
template< typename TimeType = double, typename StateScalarType = double >
boost::shared_ptr< Ephemeris > createReferencePointEphemeris(
        boost::shared_ptr< Ephemeris > bodyEphemeris,
        boost::shared_ptr< RotationalEphemeris > bodyRotationModel,
        boost::function< basic_mathematics::Vector6d( const double& ) > referencePointRelativeStateFunction )
{
    typedef Eigen::Matrix< StateScalarType, 6, 1 > StateType;

    // Cast state fucntion of body (global) and reference point (local) into correct form.
    std::map< int, boost::function< StateType( const TimeType& ) > > referencePointEphemerisVector;
    referencePointEphemerisVector[ 2 ] = boost::bind(
                &Ephemeris::getTemplatedStateFromEphemeris< StateScalarType, TimeType >, bodyEphemeris, _1 );
    referencePointEphemerisVector[ 0 ] = boost::bind(
                &convertStateFunctionStateScalarOutput< double, StateScalarType, TimeType, 6 >,
                                               referencePointRelativeStateFunction, _1 );


    // Crate rotation functions from local to global frame.
    boost::function< Eigen::Quaterniond( const double ) > rotationToFrameFunction =
            boost::bind( &RotationalEphemeris::getRotationToBaseFrame, bodyRotationModel, _1,
                         basic_astrodynamics::JULIAN_DAY_ON_J2000 );
    boost::function< Eigen::Matrix3d( const double ) > rotationMatrixToFrameDerivativeFunction =
            boost::bind( &RotationalEphemeris::getDerivativeOfRotationToBaseFrame, bodyRotationModel, _1,
                         basic_astrodynamics::JULIAN_DAY_ON_J2000 );

    // Create ephemeris
    std::map< int, boost::function< StateType( const double, const StateType& ) > > referencePointRotationVector;
    referencePointRotationVector[ 1 ] = boost::bind(
        transformStateToFrameFromRotationTimeFunctions< StateScalarType >,
                _2, _1, rotationToFrameFunction, rotationMatrixToFrameDerivativeFunction );

    return boost::make_shared< CompositeEphemeris< TimeType, StateScalarType > >(
                referencePointEphemerisVector, referencePointRotationVector, "SSB", "ECLIPJ2000" );
}

} // namespace ephemerides

} // namespace tudat
#endif // TUDAT_COMPOSITEEPHEMERIS_H
