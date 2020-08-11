throw new Error("Description for built in functions. Must not be included!");
/**
 * @fileOverview
 *    Documentation of Sky class built into dimctrl.
 */


/**
 * @class
 *
 * This class represents a set of sky coordinates.
 *
 * If the data was the result of a coordinate transformation, the
 * corresponding time is stored in addition. A function to convert
 * to local coordinates is included.
 *
 * @param {Number} rightAscension
 *    Right ascension in hours
 *
 * @param {Number} declination
 *    Declination in degree
 *
 * @example
 *     var sky   = new Sky(12, 45);
 *     var local = sky.toLocal();
 *     var date = new Date("2012-10-25 16:30 GMT"); // Date in UTC
 *     var local = sky.toLocal(date);
 *     var local = sky.toLocal("ORM");
 *     var local = sky.toLocal(date, "HAWC");
 *     var local = sky.toLocal("SPM", date);
 *
 * @author <a href="mailto:tbretz@physik.rwth-aachen.de">Thomas Bretz</a>
 *
 */
function Sky()
{

    /**
     * Right ascension in hours
     *
     * @constant
     * @type Number
     */
    this.ra = rightAscension

    /**
     * Declination in degrees
     *
     * @constant
     * @type Number
     */
    this.dec = declination;

    /**
     * Time corresponding to ra and dec if they are the result of
     * a conversion.
     *
     * @constant
     * @type Date
     */
    this.time = undefined;

    /**
     * Convert sky coordinates to celestial coordinates.
     * As observatory location the FACT telescope is assumed.
     * The conversion is done using libnova's ln_get_hrz_from_equ.
     *
     * @param {Date} [time=new Date()]
     *    Time for which the coordinates will be tranfromed.
     *    The order of time and observatory is arbitrary.
     *
     * @param {String} [observatory='ORM']
     *    Observatory location as defined in nova.h as a string for which
     *    the coordinate conversion is done The default is the
     *    FACT site ('ORM'). The order of time and observatory is arbitrary.
     *
     * @type Local
     *
     * @returns
     *     A Local object with the converted coordinates,
     *     the conversion time and the observatory location.
     */
    this.toLocal = function() { /* [native code] */  }
}

/**
 * Calculate the distance between two sky positions.
 *
 * The distance between the two provided objects is calculated.
 * The returned value is an absolute distance (angle) between
 * the two positions.
 *
 * @constant
 *
 * @param {Sky} sky1
 *     Celestial coordinates for one of the two objects for which
 *     the distance on the sky should be calculated. In principle
 *     every object with the properties 'ra' and 'dec' can be provided.
 *
 * @param {Sky} sky2
 *     Celestial coordinates for one of the two objects for which
 *     the distance on the sky should be calculated. In principle
 *     every object with the properties 'ra' and 'dec' can be provided.
 *
 * @returns {Number}
 *     Absolute distance between both positions on the sky in degrees.
 */
Sky.dist = function() { /* [native code] */}
