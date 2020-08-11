throw new Error("Description for built in functions. Must not be included!");
/***************************************************************************/
/***                                                                     ***/
/***        JsDoc: http://code.google.com/p/jsdoc-toolkit/w/list         ***/
/***                                                                     ***/
/***                 jsdoc -d=html/dimctrl scripts/doc/                  ***/
/***                                                                     ***/
/***************************************************************************/
/**
 * @fileOverview
 *    Documentation of the native functions built into dimctrl's
 *    global namespace.
 */

/**
 * An associative array containing the user supplied arguments identical to arg.
 * Positional arguments (arguments without value) are stored as 'n'="value"
 * and can be accessed via $[n] with n being an integer.
 *
 * @static
 * @type Array
 *
 * @example
 *    var value = $['name'];
 */
_global_.$ = [];

/**
 * An associative array containing the user supplied arguments identical to $.
 * Positional arguments (arguments without value) are listed as 'n'="value"
 * and can be accessed via arg[n] with n being an iteger.
 *
 * @static
 * @type Array
 *
 * @example
 *    // List all arguments
 *    for (var key in arg)
 *        console.out("arg["+key+"]="+arg[key]);
 *
 *    // List only positional arguments
 *    for (var i=0; i<arg.length; i++)
 *        console.out("arg["+i+"]="+arg[i]);
 */
_global_.arg = [];

/**
 * A magic variable which is always set to the filename of the
 * JavaScript file currently executed, if any.
 *
 * @static
 * @type String
 *
 * @example
 *    console.out(__FILE__);
 */
_global_.__FILE__ = filename;

/**
 * A magic variable which is always set to the modification time of the
 * JavaScript file currently executed, if any.
 *
 * @static
 * @type Date
 *
 * @example
 *    console.out(__DATE__);
 */
_global_.__DATE__ = filedate;

/**
 * A magic variable which is always set to the start time when the
 * current JavaScript session was started.
 *
 * @static
 * @constant
 * @type Date
 *
 * @example
 *    console.out(__START__);
 */
_global_.__START__ = starttime;


/**
 * Includes another java script.
 *
 * Note that it is literally included,
 * i.e. its code is executed as if it were at included at this
 * place of the current file.
 *
 * @param {String} [name="test"]
 *    Name of the file to be included. The base directory is
 *    the directory in which dimctrl was started.
 *
 * @param {String} [. . . ]
 *    More files to be included
 *
 * @type Array
 *
 * @static
 *
 */
_global_.include = function() { /* [native code] */  }

/**
 * Forecefully exit the current script. This function can be called
 * from anywhere and will terminate the current script.
 *
 * The effect is the same than throwing a null expecption ("throw null;")
 * in the main thread. In every other thread or callback, the whole script
 * will terminate which is different from the behaviour of a null exception
 * which only terminates the corresponding thread.
 *
 * @static
 *
 */
_global_.exit = function() { /* [native code] */  }

/**
 * Reads a file as a whole.
 *
 * Files can be split into an array when reading the file. It is
 * important to note that no size check is done. So trying to read
 * a file larger than the available memory will most probably crash
 * the program. Strictly speaking only reading ascii fils make sense.
 * Also gzip'ed files are supported.
 *
 * Note that this is only meant for debugging purpose and should
 * not be usd in a production environment. Scripts should not
 * access any files by defaults. If external values have to be
 * provided arguments should be given to the script.
 *
 * @static
 *
 * @param {String} name
 *    Name of the file to read. The base directory is the current
 *    working directory
 *
 * @param {String} [delim=undefined]
 *    A delimiter used to split the file into an array. If provided
 *    it must be a String of length 1.
 *
 * @returns {String,Array[String]}
 *    If no delimiter is given, a StringObject with the file (read
 *    until \0) is returned. If a delimiter is given, an array
 *    of Strings is returned, one for each chunk. Both objects
 *    contain the property 'name' with the file name and the array
 *    contains the property 'delim' with the used delimiter.
 *
 * @throws
 *    <li> If number or type of arguments is wrong
 *    <li> If there was an error reading the file, the system error is thrown
 *
 * @example
 *    var string = File("fact++.rc");
 *    var array  = File("fact++.rc", "\n");
 */
_global_.File = function() { /* [native code] */ }
