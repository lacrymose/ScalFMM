// ===================================================================================
// Copyright ScalFmm 2011 INRIA, Olivier Coulaud, B√©renger Bramas, Matthias Messner
// olivier.coulaud@inria.fr, berenger.bramas@inria.fr
// This software is a computer program whose purpose is to compute the FMM.
//
// This software is governed by the CeCILL-C and LGPL licenses and
// abiding by the rules of distribution of free software.  
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public and CeCILL-C Licenses for more details.
// "http://www.cecill.info". 
// "http://www.gnu.org/licenses".
// ===================================================================================
#ifndef FSPHERICAL_HPP
#define FSPHERICAL_HPP

#include "FGlobal.hpp"
#include "FMath.hpp"
#include "FPoint.hpp"
#include "FLog.hpp"

/**
* This class is a Spherical position
*
* @brief Spherical coordinate system
*
* We consider the spherical coordinate system \f$(r, \theta, \varphi)\f$ commonly used in physics. r is the radial distance, \f$\theta\f$ the polar/inclination angle and \f$\varphi\f$ the azimuthal angle.<br>
* The <b>radial distance</b> is the Euclidean distance from the origin O to P.<br>
* The <b>inclination (or polar angle) </b>is the angle between the zenith direction and the line segment OP.<br>
* The <b>azimuth (or azimuthal angle)</b> is the signed angle measured from the azimuth reference direction to the orthogonal projection of the line segment OP on the reference plane.<br>
*
* The spherical coordinates of a point can be obtained from its Cartesian coordinates (x, y, z) by the formulae
* \f$ \displaystyle r = \sqrt{x^2 + y^2 + z^2}\f$<br>
* \f$ \displaystyle \theta = \displaystyle\arccos\left(\frac{z}{r}\right) \f$<br>
* \f$ \displaystyle \varphi = \displaystyle\arctan\left(\frac{y}{x}\right) \f$<br>
*and \f$\varphi\in[0,2\pi[ \f$ \f$ \theta\in[0,\pi]\f$<br>
*
* The spherical coordinate system  is retrieved from the the spherical coordinates by <br>
* \f$x = r \sin(\theta) \cos(\varphi)\f$ <br>
* \f$y = r \sin(\theta) \sin(\varphi)\f$ <br>
* \f$z = r \cos(\theta) \f$<br>
* with  \f$\varphi\in[-\pi,\pi[ \f$ \f$ \theta\in[0,\pi]\f$<br>
*
* This system is defined in p 872 of the paper of Epton and Dembart, SIAM J Sci Comput 1995.<br>
*
* Even if it can look different from usual expression (where theta and phi are inversed),
* such expression is used to match the SH expression.
*  See http://en.wikipedia.org/wiki/Spherical_coordinate_system
*/
class FSpherical {
    // The attributes of a sphere
    FReal r;         //!< the radial distance
    FReal theta;     //!< the inclination angle [0, pi] - colatitude, polar angle
    FReal phi;       //!< the azimuth angle [-pi,pi] - longitude - around z axis
    FReal cosTheta;
    FReal sinTheta;
public:
    /** Default Constructor, set attributes to 0 */
    FSpherical()
        : r(0.0), theta(0.0), phi(0.0), cosTheta(0.0), sinTheta(0.0) {
    }

    /** From now, we just need a constructor based on a 3D position */
    explicit FSpherical(const FPoint& inVector){
        const FReal x2y2 = (inVector.getX() * inVector.getX()) + (inVector.getY() * inVector.getY());
        this->r          = FMath::Sqrt( x2y2 + (inVector.getZ() * inVector.getZ()));

        this->phi        = FMath::Atan2(inVector.getY(),inVector.getX());

        this->cosTheta = inVector.getZ() / r;
        this->sinTheta = FMath::Sqrt(x2y2) / r;
        this->theta    = FMath::ACos(this->cosTheta);
        // if r == 0 we cannot divide!
        FLOG(if( r < FMath::Epsilon ) FLog::Controller << "!!! In FSpherical, r == 0!\n"; )
    }

    /** Get the radius */
    FReal getR() const{
        return r;
    }

    /** Get the inclination angle theta = acos(z/r) [0, pi] */
    FReal getTheta() const{
        return theta;
    }
    /** Get the azimuth angle phi = atan2(y,x) [-pi,pi] */
    FReal getPhi() const{
        return phi;
    }

    /** Get the inclination angle [0, pi] */
    FReal getInclination() const{
        return theta;
    }
    /** Get the azimuth angle [0,2pi]. You should use this method in order to obtain (x,y,z)*/
    FReal getPhiZero2Pi() const{
        return (phi < 0 ? FMath::FTwoPi + phi : phi);
    }

    /** Get the cos of theta = z / r */
    FReal getCosTheta() const{
        return cosTheta;
    }

    /** Get the sin of theta = sqrt(x2y2) / r */
    FReal getSinTheta() const{
        return sinTheta;
    }
};

#endif // FSPHERICAL_HPP
