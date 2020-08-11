throw new Error("Description for built in functions. Must not be included!");
/**
 * @fileOverview
 *    Documentation of dim namespace.
 */

/**
 * @namespace
 *
 * Namespace for extension functions dealing with the DIM network.
 *
 * @author <a href="mailto:thomas.bretz@epfl.ch">Thomas Bretz</a>
 */
var dim = { };

/**
 *
 * Post a message into the dim log stream.
 *
 * It will be logged by the datalogger, displayed on the console
 * and in the smartfact web-gui.
 *
 * @param argument
 *     Any kind of argument can be given. If it is not a String, it
 *     is converted using the toString() member function.
 *
 * @param [. . .]
 *     Any number of additional arguments. Each argument will appear in
 *     a new line.
 *
 * @example
 *     dim.log("Five="+5, "--- new line ---");
 *
 */
dim.log = function() { /* [native code] */ }

/**
 *
 * Posts a message to the dim network with alarm severity.
 *
 * Similar to dim.log, but the message is posted to the network
 * with alarm severity. This means that it is displayed in red
 * and the smartfact web-gui will play an alarm sound.
 * The alarm state will stay valid (displayed in the web-gui) until it
 * is reset.
 *
 * @param argument
 *     Any kind of argument can be given. If it is not a String, it
 *     is converted using the toString() member function.
 *
 * @param [. . .]
 *     Any number of additional arguments. Each argument will appear as
 *     individual alarm.
 *
 * @example
 *     dim.alarm("Alarm for 30 seconds!");
 *     v8.sleep(30000);
 *     dim.alarm();
 */
dim.alarm = function() { /* [native code] */ }

/**
 *
 * Send a dim command to a dim client.
 *
 * @param {String} commandId
 *     The command id is a string and usually compiles like
 *     'SERVER/COMMAND'
 *
 * @param argument
 *     Any kind of argument can be given. Arguments are internally
 *     converted into strings using toString() and processed as
 *     if they were typed on th console.
 *
 * @param [. . .]
 *     Any number of additional arguments.
 *
 * @example
 *     dim.send('DRIVE_CONTROL/TRACK_SOURCE 0.5 180 "Mrk 421"');
 *     dim.send('DRIVE_CONTROL/TRACK_SOURCE', 0.5, 180, 'Mrk 421');
 *
 * @returns
 *     A boolean value is returned whether the command was succesfully
 *     posted into the network or not. Note that true does by no means
 *     mean that the command was sucessfully received or even processed.
 */
dim.send = function() { /* [native code] */ }

/**
 * Returns the state of the given server.
 *
 * @param {String} name
 *     The name of the server of which you want to get the state.
 *
 * @throws
 *    If number or type of arguments is wrong
 *
 * @returns {Object}
 *     An object with the properties 'index' {Integer} and 'name' {String}
 *     is returned if a connection to the server is established and
 *     state information have been received, 'undefined' otherwise. If
 *     the time of the last state change is available, it is stored
 *     in the 'property'. If a server disconnected, a valid object will
 *     be returned, but without any properties.
 */
dim.state = function() { /* [native code] */ }

/**
 * Wait for the given state of a server.
 *
 * Note that the function is internally asynchornously checking the
 * state, that means that theoretically, a state could be missed if
 * it changes too fast. If this can happen callbacks have to be used.
 *
 * @param {String} name
 *     The name of the server of which you want to wait for a state.
 *     The name must not contain quotation marks. To wait for
 *     "not the given state", prefix the server name with an
 *     exclamation mark, e.g. "!DIM_CONTROL"
 *
 * @param {String,Integer} state
 *     The state you want to wait for. It can either be given as an Integer
 *     or as the corresponding short name. If given as String it must
 *     not contain quotation marks.
 *
 * @param {Integer} [timeout]
 *     An optional timeout. If no timeout is given or a timeout of undefined,
 *     waiting will not stop until the condition is fulfilled. A timeout
 *     of 0 is allowed and will essentially just check if the server is
 *     in this state or not. If a negative value is given, exceptions are
 *     suppressed and false is returned in case of timeout. As timeout
 *     the absolute value is used.
 *
 * @throws
 *    <li> If number or type of arguments is wrong
 *    <li> If no connection to the server is established or no state
 *         has been received yet. This is identical to dim.state()
 *         returning 'undefined' (only in case a positive timeout 
 *         is given)
 *
 * @returns {Boolean}
 *     true if the state was achived within the timeout, false otherwise.
 */
dim.wait = function() { /* [native code] */ }

/**
 *
 * Returns a list of all known state of a given server.
 *
 * The returned object has all states with their index as property.
 * Each state is returned as a String object with the property description.
 *
 * @param {String} [server='DIM_CONTROL']
 *     Name of the server for which states should be listed.
 *     The states of the DIM_CONTROl server are the default.
 *
 * @throws
 *    If number or type of arguments is wrong
 *
 * @type Object[StringObject]
 *
 * @example
 *     var states = dim.getStates("SERVER");
 *     console.out(JSON.stringify(states));
 *     for (var index in states)
 *         console.out(index+"="+states[index]+": "+states[index].description);
 */
dim.getStates = function() { /* [native code] */ }

/**
 *
 * Returns a description for one service
 *
 * The returned array has objects with the properties: name, description and unit. The last
 * two are optional. The array itself has the properties name, server, service, isCommand
 * and optionally format.
 *
 * @param {String} service
 *     String specifying the name of the service to be returned.
 *
 * @throws
 *    If number or type of arguments is wrong
 *
 * @type {Description}
 *
 * @example
 *     var s = dim.getDescription("TNG_WEATHER/DATA");
 *     console.out("Name="+s.name);
 *     console.out("Server="+s.server);
 *     console.out("Service="+s.service);
 *     console.out("Format="+s.format);
 *     console.out("Description="+s.name);
 *     console.out("IsCommand="+s.isCommand);
 *     console.out(JSON.stringify(s));
 */
dim.getDescription = function() { /* [native code] */ }

/**
 *
 * Returns a list of all known services
 *
 * The returned array has objects with the properties: name, server, service, command
 * and (optional) format.
 *
 * @param {String} [service='*']
 *     String a service has to start with to be returned. The default is to return
 *     all available services. An empty string or '*' are wildcards for all
 *     services.
 *
 * @param {Boolean} [isCommand=undefined]
 *     If no second argument is specified, data services and commands are returned.
 *     With true, only commands, with false, only data-services are returned.
 *
 * @throws
 *    If number or type of arguments is wrong
 *
 * @type Array[Object]
 *
 * @example
 *     // Return all services of the FAD_CONTROL starting with E
 *     var services = dim.getServices("FAD_CONTROL/E");
 *     console.out(JSON.stringify(services));
 */
dim.getServices = function() { /* [native code] */ }

/**
 *
 * Callback in case of state changes.
 *
 * To install a callback in case the state of a server changes. set
 * the corresponding property of this array to a function. The argument
 * provided to the function is identical with the object returned
 * by dim.state(). In addition the name of the server is added
 * as the 'name' property and the comment sent with the state change
 * as 'comment' property. For the code executed, the same rules apply
 * than for a thread created with Thread.<P>
 *
 * If a state change is defined for a server for which no callback
 * has been set, the special entry '*' is checked.
 *
 *
 * @type Array[Function]
 *
 * @example
 *     dim.onchange['*'] = function(state) { console.out("State change from "+state.name+" received"); }
 *     dim.onchange['DIM_CONTROL'] = function(state) { console.out(JSON.stringify(state); }
 *     ...
 *     delete dim.onchange['DIM_CONTROL']; // to remove the callback
 *
 */
dim.onchange = [];


/**
 * DIM version number
 *
 * @constant
 * @type Integer
 */
dim.version = 0;
