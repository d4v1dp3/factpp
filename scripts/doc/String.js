throw new Error("Description for built in functions. Must not be included!");
/**
 * @fileOverview
 *    Documentation of the extension of the String class
 *    built into dimctrl.
 */

/**
 * Format a string (similar to printf in C).
 *
 * This function replaces modifiers (very similar to printf) with
 * a formated version of the argument. Since JavaScript does implicit
 * format conversion already, this is actually a very powerful tool,
 * because the type of th arguments to be formated is of no importance.
 * Implicit conversion means that also arrays and objects can be given
 * as arguments. A shortcut is available as $-extension of the native
 * String class.<P>
 *
 * Note that this function is completely written in JavaScript. It
 * can be found in InterpreterV8.cc.<P>
 *
 * The following modifiers are available (optional parts are put in
 * brackets:<br>
 *
 * <li><dt><B>c:</B> <tt>%[[-][0]N]c</tt></dt>
 * <dd>Extracts the first element from an array. In case of a String this
 * is the first character.</dd><p>
 *
 * <li><dt><B>s:</B> <tt>%[[-][0]N]s</tt></dt>
 * <dd>Converts the argument to a string using toString()</dd><p>
 *
 * <li><dt><B>f:</B> <tt>%[[-][0]N[.n]]f</tt></dt>
 * <dd>Converts to a Number value with n internal decimal places</dd><p>
 *
 * <li><dt><B>p:</B> <tt>%[[-][0]N[.n]]p</tt></dt>
 * <dd>Converts to a Number value with a precision of n decimal places</dd><P>
 *
 * <li><dt><b>e:</b> <tt>%[[-][0]N]e</tt></dt>
 * <dd>Converts to an exponential with a precision of n decimal places</dd><p>
 *
 * <li><dt><b>x:</b> <tt>%[[-][0]N[#b]x</tt></dt>
 * <dd>Converts to an integer value using the basis b for conversion
 * (default is 16 for hexadecimal)</dd><p>
 *
 * <li><dt><b>d:</b> <tt>%[[-][0]N[.n][#b]]d</tt></dt>
 * <dd>Converts from a value using the basis b for conversion (default
 * is 10 for decimal). The integer can be rounded to the given precision n.
 * </dd><p>
 *
 * The output string will have at least a length of N. If 0 is given,
 * it is filled with 0's instead of white spaces. If prefixed by a minus
 * the contents will be left aligned.
 *
 * @param {String} format
 *     The format string defining how the given argument elements are
 *     formated
 *
 * @param {Array} elements
 *     An array with the elements to be formated
 *
 * @returns {String}
 *     The formated string is returned.
 *
 * @see
 *     String.$
 *
 * @example
 *    var result;
 *    result = String.form("%5d %3d", [ 5, "2" ]);
 *    result = String.form("%5x", [ 12 ]);
 *    result = String.form("%#16d", [ "b" ]);
 *    result = String.form("%s", [ [ 1, 2, 3, ] ]);
 *    result = String.form("%s", [ { a:1, b:2, c:3, } ]);
 *    var abbrev = "%03d".$(42);
 *
 */
String.form = function() { /* [native code] */ }


/**
 * An abbreviation for String.form.
 *
 * Converts all arguments provided by the user into an array
 * which is passed to String.form. The contents of the String itself
 * is passed as format string. This allows a very compact syntax to
 * format any kind of object, array or number.
 * For details see String.form.
 *
 * @param   arg       An argument to be formated.
 * @param   [. . .]   An undefined number of additional optional arguments.
 *
 * @returns {String} see String.form
 * @throws  see String.form
 * @see     String.form
 *
 * @example
 *    var result = "%5d = %12s".$(5, "five");
 *
 * @author <a href="mailto:thomas.bretz@epfl.ch">Thomas Bretz</a>
 */
String.prototype.$ = function() { /* [native code] */ };

/**
 * Like String match, but return the number counts how often
 * the regular expression matches.
 *
 * @param {String} regex
 *     The regular expression to search for, e.g., "s" (to count the s's) or
 *     "As+A" (to count how often s's are surrounded by A's)
 *
 * @param {Boolean} [case=false]
 *     Search is case insensitive if set to true.
 *
 * @returns {Interger}
 *     The number of occurances of the regular expression
 *
 * @example
 *    var result = "Thomas Bretz".count("[hme]"); // returns 3
 *
 * @author <a href="mailto:thomas.bretz@epfl.ch">Thomas Bretz</a>
 */
String.prototype.count = function() { /* [native code] */ };
