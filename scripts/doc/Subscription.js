throw new Error("Description for built in functions. Must not be included!");
/**
 * @fileOverview
 *    Documentation of a DIM service Subscription
 */

/**
 * @class
 *
 * Subscription to a DIM service.
 *
 * This class represents the subscription to a DIM service. Received
 * events are first copied to an even queue internally, to avoid
 * any processing blocking the DIM thread (which could block the
 * whole network as a result). Then the events are processed.
 * If a callback is installed, the processing will take place in
 * another JavaScript thread. Physically it will run synchronously
 * with the other JavaScript threads. However, the processing blocks
 * event processing. Consequetly, processing should on average be
 * faster than the frequency with which events arrive to ensure they
 * will not fill up the memory and possible reactions to there
 * contents will happen within a reasonable time and not delayed
 * too much.
 *
 * Each subscription must exist only once, therefore the function-call
 * can be used to check for an open subscription.  
 *
 * @param {String} service
 *    Name of the DIM service to which a subscription should be made.
 *    Usully of the form SERVER/SUBSCRIPTION.
 *
 * @param {Function} [callback]
 *    An optional function which is set as 'onchange' property.
 *    This can avoid to loose th first event after the subscription
 *    before the callback is set by the 'onchange' property (which
 *    is very unlikely).
 *
 * @throws
 *    <li>If number or type of arguments is wrong
 *    <li>If an open subscription to the same service already exists.
 *
 * @example
 *    var handle1 = Subscription("MAGIC_WEATHER/DATA");
 *    if (!handle1)
 *        handle1 = new Subscription("MAGIC_WEATHER/DATA");
 *    var handle2 = new Subscription("TNG_WEATHER/DATA", function(evt) { console.out(JSON.stringify(evt)); });
 *    ...
 *    handle2.close();
 *    handle1.close();
 */
function Subscription(service, callback)
{
    /**
     *
     * The name of the service subscribed to.
     *
     * @constant
     * @type String
     *
     */
    this.name = service;

    /**
     *
     * Boolean value which is set to false if the Subscription was closed.
     *
     * @type Boolean
     *
     */
    this.isOpen = false;

    /**
     *
     * Callback in case of event reception.
     *
     * To install a callback in case a new event of this Subscription
     * was received, set this property to a function. The argument
     * provided to the function is identical with the object returned
     * by Subscription.get(). For the code executed, the same rules apply
     * than for a thread created with Thread.
     *
     * @type Function
     *
     * @example
     *     handle.onchange = function(event) { console.out(JSON.stringify(event); };
     *
     */
    this.onchange = callback;

    /**
     *
     * Returns the last received event of this subscription.
     *
     * @param {Integer} [timeout=0]
     *     A timeout in millisecond to wait for an event to arrive.
     *     This timeout only applied if no event has been received yet
     *     after a new Subscription has been created. If an event
     *     is already available, the event is returned. If the timeout
     *     is 'null', waiting will never timeout until an event was received.
     *     If the timeout is less than zero, no exception will be thrown,
     *     but 'undefined' returned in case of timeout. The corresponding
     *     timeout is then Math.abs(timeout).
     *
     * @param {Boolean} [requireNamed=true]
     *     Usually an event is only considered complete, if also the
     *     corresponding decription is available distributed through
     *     the service SERVER/SERVICE_DESC. If an event has no
     *     description or access to the data by name is not important,
     *     requireNamed can be set to false.
     *
     * @throws
     *    <li> If number or type of arguments is wrong
     *    <li> After a timeout, if the timeout value was greater or equal zero
     *    <li> If conversion of the received data to an event object has failed
     *
     * @returns {Event}
     *     A valid event is returned, undefined in the case waiting for an
     *     event has timed out and exceptions are supressed by a negative
     *     timeout.
     *
     * @example
     *     var a = new Subscription("...service does not exist...");
     *     a.get( 3000, true);  // throws an exception
     *     a.get( 3000, false); // throws an exception
     *     a.get(-3000, true);  // returns undefined
     *     a.get(-3000, false); // returns undefined
     *
     *     var a = new Subscription("...service with valid description but no valid data yet...");
     *     a.get( 3000, true);  // throws an exception
     *     a.get( 3000, false); // returns Event.data==null, Event.obj valid but empty
     *     a.get(-3000, true);  // return undefined
     *     a.get(-3000, false); // returns Event.data==null, Event.obj valid but empty
     *
     *     // Assuming that now valid description is available but data
     *     var a = new Subscription("...service without valid description but valid data...");
     *     a.get( 3000, true);  // throws an exception
     *     a.get( 3000, false); // returns Event.data valid, Event.obj==undefined
     *     a.get(-3000, true);  // returns undefined
     *     a.get(-3000, false); // returns Event.data valid, Event.obj==undefined
     *
     */
    this.get = function() { /* [native code] */ }

    /**
     *
     * Unsubscribe from an existing subscription. Note that all open
     * subscription produce network traffic and should be omitted if
     * not needed.
     *
     * @returns {Boolean}
     *     true if the subscription was still open, false if it was
     *     already closed.
     *
     */
    this.close = function() { /* [native code] */ }
}
