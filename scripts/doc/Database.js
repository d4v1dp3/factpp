throw new Error("Description for built in functions. Must not be included!");
/**
 * @fileOverview
 *    Documentation of Database connection object
 */

/**
 * @class
 *
 * Returns a connection to a MySQL server or a specific database.
 *
 * For connection the MySQL++ library is used. MySQL++ throws exceptions
 * in case of errors, e.g. connection timeout.<P>
 *
 * Note that although the object is created with 'new' and there
 * is a 'delete' is JavaScript, it will not call any kind of
 * destructor. To close a Subscription you have to explicitly call
 * the close() member function. 'delete' in JavaScript is only
 * to remove a property from an Object.
 *
 * @param {String} database
 *    The databse argument is of this form (optional parts ar given in brackets):<br>
 *    <tt>user:password@server.domain.com[:port]/database</tt>
 *
 * @throws
 *    <li> If number or type of arguments is wrong
 *    <li> If no connection could be opened, an exception with the reason is
 *    thrown.
 *
 * @example
 *    var db = new Database("thomas@sql.at-home.com/database");
 */
function Database()
{
    /**
     * User connected to the database
     * @constant
     */
    this.user = user;

    /**
     * Server which is connected
     * @constant
     */
    this.server = server;

    /**
     * Database which is connected
     * @constant
     */
    this.database = database;

    /**
     * Port connected (if no port was given 'undefined')
     * @constant
     */
    this.port = port;

    /**
     * Returns the result of an sql query sent to the database.
     *
     * @param arguments
     *    The arguments specify the query to be sent
     *
     * @throws
     *    If no connection could be opened, an exception with the reason is
     *    thrown.
     *
     * @returns
     *    An array is returned. Each entry in the array corresponds to one
     *    row of the table and is expressed an an associative array (object).
     *    The names of the entries (columns) in each row are stored in
     *    a property cols which is an array itself. For convenience,
     *    table and query are stored in identically names properties.
     *
     * @example
     *    var table = db.query("SELECT * FROM table WHERE value BETWEEN", 5, "AND 20");
     *    for (var row=0; row&lt;table.length; row++)
     *        for (var col in table.cols)
     *            console.out("table["+row+"]['"+col+"']="+table[row][col]);
     *
     */
    this.query = function() { /* [native code] */ }

    /**
     *
     * Close the connection to the database.
     *
     * The connection is automaically closed at cript termination.
     *
     * @returns {Boolean}
     *     If the connection was successfully closed, i.e. it
     *     was still open, true is returned, false otherwise.
     *
     */
    this.close = function() { /* [native code] */ }
};
