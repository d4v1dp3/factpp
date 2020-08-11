throw new Error("Description for built in functions. Must not be included!");
/**
 * @fileOverview
 *    Documentation of Moon class built into dimctrl.
 */

/**
 * @class
 *
 * Calculates the moon's sky position at a given time.
 *
 * When instantiated, the class members are set to the sky position
 * of the moon at the given time. The calculation is done using
 * libnova's ln_get_lunar_equ_coords. A different time can be provided
 * by the user. The sky coordinates are stored together with the time.
 * A function is provided to convert the moon's position to celestial
 * coordinates. A function to calculate the illuminated fraction of
 * the moon's disk.
 *
 * @param {Date} [time=new Date()]
 *    Reference time for the calculation of the moon's position.
 *
 * @example
 *    var date = new Date("2012-10-25 16:30 GMT"); // Date in UTC
 *    var moon = new Moon(date);
 *    var moon = new Moon();
 *    var local = moon.toLocal();
 *    var local = moon.toLocal("HAWC");
 *
 * @author <a href="mailto:tbretz@physik.rwth-aachen.de">Thomas Bretz</a>
 */
function Moon(time)
{
    /**
     * Right ascension of the moon in hours.
     *
     * @type Number
     * @constant
     *
     */
    this.ra = 0;

    /**
     * Declination of the moon in degrees.
     *
     * @type Number
     * @constant
     *
     */
    this.dec = 0;

    /**
     * Time corresponding to the calculated sky coordinates of the moon.
     *
     * @type Date
     * @constant
     */
    this.time = time;

    /**
     * Converts the moon's sky coordinates to celestial coordinates.
     * As observatory location the FACT telescope is assumed. For the
     * time, the time corresponding to the stored sky coordinates is used.
     * The conversion is done using libnova's ln_get_hrz_from_equ.
     *
     * @param {String} [observatory='ORM']
     *    Observatory location as defined in nova.h as a string. The default
     *    is the FACT site ('ORM').
     *
     * @returns {Local}
     *     A Local object with the converted coordinates and
     *     the corresponding time and observatory.
     */
    this.toLocal = function() { /* [native code] */  }
}

/**
 * Calculates the illuminated fraction of the moon's disk.
 *
 * Calculates the illuminated fraction of the moon's disk for the
 * provided time. If no time is provided, the current system time
 * is used. The calculation is done using libnova's ln_get_lunar_disk.
 *
 * @param {Date} [time=new Date()]
 *    Time for which the moon disk should be calculated. If no time is
 *    given, the current time is used.
 *
 * @type Number
 *
 * @returns
 *    The illuminated fraction of the moon's disk corresponding
 *    to the time argument
 *
 */
Moon.disk = function() { /* [native code] */ }

/**
 * Calculate moon rise, set and transit times.
 *
 * Calculates the moon rise and set time, above and below horizon,
 * and the time of culmination (transit time) for the given time.
 * The calculation is done using libnova's ln_get_lunar_rst and is
 * always performed for the FACT site at La Palma.
 *
 * @param {Date} [time=new Date()]
 *    Date for which the times should be calculated. Note that the date
 *    is converted to UTC and the times are calculated such that the
 *    Date (yy/mm/dd) is identical for all returned values.
 *    The order of time and observatory is arbitrary.
 *
 * @param {String} [observatory='ORM']
 *    Observatory location as defined in nova.h as a string. The default
 *    is the FACT site ('ORM'). The order of time and observatory is
 *    arbitrary.
 *
 * @type {Object}
 *
 * @returns
 *    An object with the following properties is returned: time {Date}
 *    the provided time; observatory {Date} the provided observatory location;
 *    rise, transit, set {Date} times of rise, set and transit;
 *    isUp {Boolean} whether the moon is above or below horizon
 *    at the provided time. If the moon does not rise or set, the properties
 *    rise, transit and set will be undefined.
 *
 * @example
 *    var date = new Date("2012-10-25 16:30 GMT"); // Date in UTC
 *    console.out(JSON.stringify(Moon.horizon());
 *    console.out(JSON.stringify(Moon.horizon(date));
 *    console.out(JSON.stringify(Moon.horizon(date, "SPM"));
 *    console.out(JSON.stringify(Moon.horizon("ORM, date);
 */
Moon.horizon = function() { /* [native code] */ }
