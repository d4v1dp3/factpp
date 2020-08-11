throw new Error("Description for built in functions. Must not be included!");
/**
 * @fileOverview
 *    Documentation of the dimctrl namespace
 */

/**
 * @namespace
 *
 * Global namespace for functions dealing with the dimctrl state
 *
 * @author <a href="mailto:thomas.bretz@epfl.ch">Thomas Bretz</a>
 */
var dimctrl = { };

/**
 * Define a new internal state.
 *
 * States should be defined when a script is started.
 *
 * @param {Integer} index
 *    The intgeger number assigned to the new state. Only numbers
 *    in the range [10, 255] are allowed.
 *
 * @param {String} [name]
 *    A short name describing the state. According the the convention
 *    used throughout FACT++, it it must not contain whitespaces or
 *    underscores. Ever word should start with a capital letter,
 *    e.g. 'TriggerOn'
 *
 * @param {String} [decription]
 *    A user defined string which gives a more conscise explanation
 *    of the meaning of the state and can also be displayed in the GUI
 *    or anywhere else automatically,
 *    e.g. "System setup and trigger switched on"
 *
 * @throws
 *    <li> if something is wrong with the supplied arguments (type, number)
 *    <li> when the index is out of range [10,255]
 *    <li> the given state name is empty
 *    <li> the given state name contains a colon or an equal sign
 *    <li> when a state with the same name or index was already
 *    <li> set since the script was started.
 *
 * @returns {Boolean}
 *    A boolean whether the state was newly added (true) or an existing
 *    one overwritten (false).
 *
 * @example
 *     dim.defineState(10, "StateTen", "This is state number ten");
 */
dimctrl.defineState = function() { /* [native code] */ }

/**
 * Change the internal state.
 *
 * @param {Integer,String} state
 *    Either the name of the state to set or its index can be supplied.
 *
 * @throws
 *    <li> if something is wrong with the supplied arguments (type, number)
 *    <li> if a String is given and it is not found in the list of names
 *
 * @returns {Boolean}
 *     A boolean is returned whether setting the state wa sucessfull or
 *     not. If the function is not called at unexpected time, i.e.
 *     before the execution of the JavaScript has been started or
 *     after it has already be terminated, true should be returned
 *     always.
 *
 * @example
 *     dim.setState(10);
 *     dim.setState("StateTen");
 */
dimctrl.setState = function() { /* [native code] */ }

/**
 * Get the current internal state.
 *
 * @throws
 *    if arguments are supplied
 *
 * @returns {Object}
 *     An object with the properties index {Number}, name {String} and
 *     description {String}. Note that name and description might be
 *     an empty string.
 *
 * @example
 *     var state = dim.getState();
 *     console.out(JSON.stringify(state));
 */
dimctrl.getState = function() { /* [native code] */ }

/**
 * Set an interrupt handler, a function which is called if an
 * interrupt is received, e.g. via dim (dimctrl --interrupt).
 * Note that the interrupt handler is executed in its own JavaScript
 * thread. Thus it interrupts the execution of the script, but does
 * not stop its execution. Please also note that this is a callback
 * from the main loop. As long as the handler is executed, no other
 * event (dim or command interface) will be processed.
 *
 * If an interrupt was triggered by dimctrl (so not from within
 * the script) and a number between 10 and 255 is returned,
 * the state machine will change its state accordingly. Other returned
 * ojects or returned values outside of this range are ignored.
 *
 * @param {Function} [func]
 *    Function to be called when an interrupt is received. Null, undefined
 *    or no argument to remove the handler.
 *
 * @throws
 *    if number of type of arguments is wrong
 *
 * @example
 *     function handleIrq(irq, args, time, user)
 *     {
 *         console.out("IRQ received:   "+irq);
 *         console.out("Interrupt time: "+time);
 *         console.out("Issuing user:   "+user);
 *         console.out("Arguments:");
 *         for (var key in args)
 *             console.out(" args["+key+"="+args[key]);
 *
 *         var newState = 10;
 *         return newState;
 *     }
 *     dimctrl.setInterruptHandler(handleIrq);
 */
dimctrl.setInterruptHandler = function() { /* [native code] */ }

/**
 * You can also issue an interrupt from anywhere in your code.
 *
 * @param argument
 *     Any kind of argument can be given. If it is not a String, it
 *     is converted using the toString() member function. The result
 *     must not contain any line break.
 *
 * @param [. . .]
 *     Any number of additional arguments. Each argument will appear in
 *     a new line.
 *
 * @returns
 *    the return of the interrupt handler which is called is returned
 *
 * @throws
 *    if an argument contains a line break
 *
 * @example
 *     dimctrl.triggerInterrupt();
 *     dimctrl.triggerInterrupt("my_command");
 *     dimctrl.triggerInterrupt("my_command arg1 arg2=x arg3");
 *     dimctrl.triggerInterrupt("arg1=x arg2 arg3");
 *
 */
dimctrl.triggerInterrupt = function() { /* [native code] */ }
