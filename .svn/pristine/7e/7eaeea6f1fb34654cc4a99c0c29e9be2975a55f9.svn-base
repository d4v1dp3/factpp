throw new Error("Description for built in functions. Must not be included!");
/**
 * @fileOverview
 *    Documentation of dim namespace.
 */

/**
 * @namespace
 *
 * Namespace for general extension functions
 *
 * @author <a href="mailto:thomas.bretz@epfl.ch">Thomas Bretz</a>
 */
var v8 = { };

/**
 * Sleep for a while. This can be used to just wait or give time
 * back to the operating system to produce less CPU load if the
 * main purpose of a loop is, e.g., to wait for something to happen.
 *
 * @param {Integer} [milliseconds=0]
 *     Number of millliseconds to sleep. Note that even 0 will always
 *     sleep at least one millisecond.
 *
 */
v8.sleep = function() { /* [native code] */ }

/**
 * This function implements a simple timeout functionality. 
 * back to the operating system to produce less CPU load if the
 * main purpose of a loop is, e.g., to wait for something to happen.
 *
 * @param {Integer} milliseconds
 *     Number of millliseconds until the timeout. Note that even 0
 *     will execute the function at least once. If the timeout
 *     is negative no exception will be thrown by undefined will
 *     be returned in case of a timeout.
 *
 * @param {Function} func
 *     A function. The function defines when the conditional to end
 *     the timeout will be fullfilled. As soon as the function returns
 *     a defined value, i.e. something else than undefined, the
 *     timeout is stopped and its return value is returned.
 *
 * @param {Object} [_this]
 *    An object which will be the reference for 'this' in the function call.
 *    If none is given, the function itself will be the 'this' object.
 *
 * @param [. . .]
 *     Any further argument will be passed as argument to the function.
 *
 * @returns
 *     Whatever is returned by the function. undefined in case of timeout
 *     and a negative timeout value.
 *
 * @throws
 *     <li> When the number or type of argument is wrong
 *     <li> In case the timeout is positive and the timeout condition occurs
 *
 */
v8.timeout = function() { /* [native code] */ }

/**
 * Version number of the V8 JavaScript engine.
 *
 * @constant
 * @type String
 */
v8.version = "";
