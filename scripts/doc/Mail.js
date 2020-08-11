throw new Error("Description for built in functions. Must not be included!");
/**
 * @fileOverview
 *    A class which allows to send mails through the 'mail' program
 */


/**
 * @class
 *
 * This class represents an interface to the program 'mail'.
 *
 * To send a mail, create an instance and fill the properties
 * (see reference) with proper data.
 *
 * @example
 *     var mail = new Mail("This is the subject");
 *
 *     // At least one recipient is mandatory
 *     mail.recipients.push("max.mustermann@musterstadt.com");
 *     // To add several recipients
 *     mail.recipients.push("max.mustermann@musterstadt.com", "erika.mustermann@musterstadt.com");
 *
 *     // similar to the property recipient you can use the properties 'cc' and 'bcc'
 *
 *     // If you want to add attachments [optional]
 *     mail.attachments.push("logfile.txt");
 *     // or for several attachments
 *     mail.attachments.push("logfile1.txt", "logfile2.txt");
 *
 *     // The text of the message is set in the property text...
 *     // ...either as single string
 *     mail.text.push("This is line1\nThis is line2");
 *     mail.text.push("This is line1");
 *     mail.text.push("This is line2");
 *
 *     // Send the message
 *     mail.send();
 *
 * @author <a href="mailto:thomas.bretz@epfl.ch">Thomas Bretz</a>
 *
 */
function Mail()
{

    /**
     * Subject of the mail
     *
     * @type String
     * @constant
     */
    this.subject = subject;

    /**
     * Recipient(s) of the mail. One recipient is mandatory.
     *
     * @type Array[String]
     */
    this.recipients = recipient;

    /**
     * Carbon copy [optional]. Adresses who should receive a copy of the
     * mail. All entries in the array which are not a string are silently ignored.
     *
     * @type Array[String]
     */
    this.cc = undefined;

    /**
     * Blind carbon copy [optional]. Adresses who should receive a copy of the
     * mail. All entries in the array which are not a string are silently ignored.
     *
     * @type Array[String]
     */
    this.bcc = undefined;

    /**
     * Attachments [optional]. File to be attached to the mail.
     * All entries in the array which are not a string are silently ignored.
     *
     * @type Array[String]
     */
    this.attachments = undefined;

    /**
     * Message body. At least one line in the message is mandatory.
     * Undefined or null entries in the array are silently ignored.
     *
     * @type Array[String]
     */
    this.text = text;

    /**
     * Send the message. This calles the 'mailx' program. For further
     * details, e.g. on the return value, see the corresponding man page.
     *
     * @param {Boolean} [block=true]
     *    This parameter specifies whether the pipe should be closed,
     *    which means that a blocking wait is performed until the 'mail'
     *    program returns, or the pipe will be closed automatically
     *    in the background when the 'mail' program has finished.
     *    Note, that if the calling program terminates, maybe this
     *    call will never succeed.
     *
     * @returns {Integer}
     *    The return code of the 'mail' program is returned (0
     *    usually means success), undefined if block was set to false.
     *
     * @throws
     *    An exception is thrown if any validity check for the
     *    properties or the arguments fail.
     *
     */
    this.send = function() { /* [native code] */ }
}
