throw new Error("Description for built in functions. Must not be included!");
/**
 * @fileOverview
 *    Documentation of Sun class built into dimctrl.
 */

/**
 * @namespace
 *
 * Namespace for functions returning astrometry information about the Sun.
 *
 * @author <a href="mailto:tbretz@physik.rwth-aachen.de">Thomas Bretz</a>
 */
var Sun = { };

/**
 * Calculate sun rise, set and transit times.
 *
 * Calculates the sun's rise and set time, above and below horizon,
 * and the time of culmination (transit time) for the given time.
 * As a second argument the angle abov or below horizon of interest
 * can be provided. The calculation is done using libnova's
 * ln_get_solar_rst_horizon and is always performed for the FACT
 * site at La Palma.
 *
 * @param {Number,String} [angle=null]
 *    Angle above (positive) or below (negative) horizon. The
 *    angle can be given as Number in degree or as string referring to
 *    "horizon" (0deg), "civil" (-6deg), "nautical" (-12deg),
 *    "fact" (-15deg), "astronomical" (-18deg). Strings can be abbreviated
 *    down to "h", "c", "n", "f" and "a". If the argument is omitted or
 *    null, a value referring to the appearance or the disappearance of
 *    the Sun's disk at horizon is chosen (~-0.8deg).
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
 *    the provided time; rise, transit, set {Date} times of rise, set and
 *    transit; horizon {Number} the angle used for the calculation;
 *    isUp {Boolean} whether the sun is above or below the given horizon
 *    at th given time. If the sun does not rise or set, the properties
 *    rise, transit and set will be undefined, isUp will refer to
 *    the fact whether the sun is the whole day above or below the
 *    horizon (0deg).
 *
 * @example
 *    var date = new Date("2012-10-25 16:30 GMT"); // Date in UTC
 *    console.out(JSON.stringify(Sun.horizon());
 *    console.out(JSON.stringify(Sun.horizon("astro"));
 *    console.out(JSON.stringify(Sun.horizon(-12, date); // nautical
 *    console.out(JSON.stringify(Sun.horizon(-12, "SPM");
 *    console.out(JSON.stringify(Sun.horizon(-12, "HAWC", date);
 *    console.out(JSON.stringify(Sun.horizon(-12, date, "ORM");
 */
Sun.horizon = function() { /* [native code] */ }
