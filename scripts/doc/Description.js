throw new Error("Description for built in functions. Must not be included!");
/**
 * @fileOverview
 *    Documentation of the Event object returned by Subscription.get()
 */

/**
 * @class
 *
 * The object stores the returned data of dim.getDescription(). This object
 * is essentially an array. Each entry can store the property name, description
 * and unit.
 *
 */
function Description()
{
    /**
     * The name of the service the object belongs to, e.g. RATE_CONTROL/THRESHOLD
     *
     * @type String
     * @constant
     */
    this.name = name;

    /**
     * The server name of the service the object belongs to, e.g. RATE_CONTROL.
     * (if no description data is available this might be undefined)
     *
     * @type String
     * @constant
     * @default undefined
     */
    this.server = server;

    /**
     * The service of the service the object belongs to, e.g. THRESHOLD
     * (if no description data is available this might be undefined)
     *
     * @type String
     * @constant
     * @default undefined
     */
    this.service = service;

    /**
     * If available, a global description of this service.
     *
     * @type String
     * @constant
     * @default undefined
     */
    this.description = description;

    /**
     * If available, the format string corresponding to this service.
     *
     * @see <A HREF="dim.cern.ch">DIM</A> for more details
     * @type String
     * @constant
     * @default undefined
     */
    this.format = format;

    /**
     * true if this service is a command, false if it is a data-service
     *
     * @type Boolean
     * @constant
     * @default undefined
     */
    this.isCommand = is_command;

    /**
     * The number of entries, i.e. descriptions of individual arguments of the service
     * (if no description data is available this might be undefined)
     *
     * @type Integer
     * @constant
     */
    this.length = length;

}
