throw new Error("Description for built in functions. Must not be included!");
/**
 * @fileOverview
 *    A class which allows to issue simple http requests through 'curl'
 */


/**
 * @class
 *
 * This class represents an interface to the program 'curl'.
 *
 * Note that it currently only implements the minimum required
 * interface but it can easily be extended.
 *
 * To send a http request, create an instance with the address
 * and (if required) username and password as argument.
 *
 * @example
 *     var curl = new Curl("user:password@www.server.com/path/index.html");
 *
 *     // You can add data with
 *     curl.data.push("argument1=value1");
 *     curl.data.push("argument2=value3");
 *
 *     // Issue the request
 *     var ret = curl.send();
 *
 * @author <a href="mailto:tbretz@physik.rwth-aachen.de">Thomas Bretz</a>
 *
 */
function Curl()
{

    /**
     * Data of the post/get request
     *
     * @type Array[String]
     */
    this.data = data;

    /**
     * Send the request. This calles the 'curl' program. For further
     * details, e.g. on the return value, see the corresponding man page.
     *
     * @param {Boolean} [block=true]
     *    This parameter specifies whether the pipe should be closed,
     *    which means that a blocking wait is performed until the 'mail'
     *    program returns, or the pipe will be closed automatically
     *    in the background when the 'curl' program has finished.
     *    Note, that if the calling program terminates, maybe this
     *    call will never succeed.
     *
     * @returns {Object}
     *    An object with three properties is returned.
     *    'cmd'  contains the command issued
     *    'data' contains the data returned from the server in case of
     *           success, some error string returned by curl otherwise.
     *    'rc'   is an integer and the return code of 'curl'
     *
     */
    this.send = function() { /* [native code] */ }
}
