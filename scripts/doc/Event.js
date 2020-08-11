throw new Error("Description for built in functions. Must not be included!");
/**
 * @fileOverview
 *    Documentation of the Event object returned by Subscription.get()
 */

/**
 * @class
 *
 * The object returned by Subscription.get(). It contains
 * all data received with the event.
 *
 */
function Event()
{
    /**
     * The name of the Subscription this event belongs to.
     *
     * @type String
     * @constant
     */
    this.name = name;

    /**
     * The format string corresponding to this event.
     *
     * @see <A HREF="dim.cern.ch">DIM</A> for more details
     * @type String
     * @constant
     */
    this.format = format;

    /**
     * The Quality-of-Service transmitted by this event.
     *
     * @type Integer
     * @constant
     */
    this.qos = qos;

    /**
     * The size in bytes of the event received
     *
     * @type Integer
     * @constant
     */
    this.size = size;

    /**
     * An counter of events received since the Subscription has
     * been created. The first event received is 1. 0 corresponds
     * to no event received yet.
     *
     * @type Integer
     * @constant
     */
    this.counter = counter;

    /**
     * The time transmitted with this event, if transmitted. If nonw
     * was transmitted, this might just be the time the event was
     * received.
     *
     * @type Date
     * @constant
     */
    this.time = time;

    /**
     * Array with the data received.
     *
     * The contents of the array are sorted in the order of the event format
     * string. The contents of the array can be all kind of objects
     * defined by the format string. If a format described several entries
     * (e.g. I:5) and array will be added.<P>
     *
     * In the special case that the format string contains only a single
     * format, e.g. "I", "F:5" or "C", data will not be an array,
     * but contain the object data (or the array) directly.
     *
     * If valid data was received, but the size was zero, then
     * null is returned as data
     *
     *    <li> data===undefined: no data received (no connection)
     *    <li> data===null:      an event was received, but it was empty
     *    <li> data.length>0:    an event was received and it contains data
     *
     * @type Array
     * @constant
     *
     */
    this.data = [ ];

    /**
     * Object with the data received.
     *
     * The object contains essentially the same information than the
     * data memeber, but the received data are added as properties
     * instead of enumerable lements. This allows to access
     * the received data by names as specified by the SERVICE_DESC
     * service.<P>
     *
     * If an empty event was received, but names are available,
     * the object will be empty. Otherwise 'obj' will be undefined.
     *
     *     <li> obj===undefined: no names are available
     *     <li> obj!==undefined, length==0: names are available, but no data (no connection)
     *     <li> obj!==undefined, length>0: names are available, data has been received
     *
     * <P>
     * Note that to get the number of properties (length) you have to call
     * Object.keys(obj).length;
     *
     * @type Object
     * @constant
     *
     */
    this.obj = { };
}
