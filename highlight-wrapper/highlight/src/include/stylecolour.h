/***************************************************************************
                          stylecolour.h  -  description
                             -------------------
    begin                : Die Nov 5 2002
    copyright            : (C) 2002-2026 by Andre Simon
    email                : a.simon@mailbox.org
 ***************************************************************************/


/*
This file is part of Highlight.

Highlight is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Highlight is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Highlight.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef STYLECOLOUR_H
#define STYLECOLOUR_H

#include "enums.h"

#include <string>


namespace highlight
{

/**\brief Stores colours and returns red, green and blue values in different formats
* @author Andre Simon
 */

struct RGBVal {
    int iRed,    ///< Red value
        iGreen,  ///< Green value
        iBlue;   ///< Blue value
};

class Colour
{
public:
    /** Constructor
        \param red Red value in hex notation
        \param green Blue value in hex notation
        \param blue Green value in hex notation
    */
    Colour ( const std::string & red, const std::string & green, const std::string & blue );

    /** Constructor
        \param ColourString String with rgb values
    */
    Colour ( const std::string & colourString );

    Colour();
    ~Colour() {};

    /** Sets red, green and blue values
      \param ColourString String containing colour attributes
    */
    void setRGB ( const std::string & colourString );

    /** Sets red value
        \param red New red value */
    void setRed ( const std::string & red );

    /** Sets green value
        \param green New green value */
    void setGreen ( const std::string & green );

    /** Sets blue value
        \param blue New blue value */
    void setBlue ( const std::string & blue );

    /**  @param type Output type
         @return Red value in color representation according to output type */
    std::string getRed ( OutputType type ) const;

    /**  @param type Output type
         @return Green value in color representation according to output type */
    std::string getGreen ( OutputType type ) const;

    /**  @param type Output type
         @return Blue value in color representation according to output type */
    std::string getBlue ( OutputType type ) const;

    /**  @return red value */
    int getRed () const;

    /**  @return green value */
    int getGreen () const;

    /**  @return blue value */
    int getBlue () const;

private:
    RGBVal rgb;
    std::string int2str ( int, std::ios_base& ( *f ) ( std::ios_base& ) ) const;
    std::string float2str ( double ) const;
};

}

#endif
