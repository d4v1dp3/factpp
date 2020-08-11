throw new Error("Description for built in functions. Must not be included!");
/**
 * @fileOverview
 *    Documentation of the Thread object
 */

/**
 * @class
 *
 * Creates a handle to a new thread.
 *
 * The handle can be used to
 * kill the thread or be ignored. The function provided is
 * executed after an initial timeout. Note that although this
 * looks similar to the setTimeout in web-browsers, after started,
 * the thread will not run until completion but run in parallel to
 * the executed script.<P>
 *
 * To stop the script from within a thread, use exit(). To stop only
 * execution of the thread (silently) throw a null exception
 * ("throw null;"). To terminate the script with an exception
 * throw a normal exception ("throw new Error("my error");").
 *
 * Note that a running thread might consume all CPU. Although it is
 * a seperated thread, JavaScript allows only one thread to run at
 * a time (thus it can make programming simpler, but is not really
 * consuming more CPU). In certain circumstances, it might be necessary
 * to give CPU time with v8.sleep(...) back to allow other threads to run.
 *
 * @param {Integer} timeout
 *    A positive integer given the initial delay in milliseconds before
 *    the thread is executed.
 *
 * @param {Function} function
 *    A function which is executed aftr the initial timeout.
 *
 * @param {Object} [_this]
 *    An object which will be the reference for 'this' in the function call.
 *    If none is given, the function itself will be the 'this' object.
 *
 * @throws
 *    <li> If number or type of arguments is wrong
 *
 * @example
 *    var handle = new Thread(100, function() { console.out("Hello world!"); });
 *    ...
 *    handle.kill();
 */
function Thread(timeout, function, _this)
{
    /**
     *
     * Kills a running thread
     *
     * @returns {Boolean}
     *     If the thread was still known, true is returned, false
     *     otherwise. If the thread terminated already, false is
     *     returned.
     *
     */
    this.kill = function() { /* [native code] */ }
};
