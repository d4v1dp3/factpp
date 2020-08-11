'use strict';

// DN 09.12.2012

// this script tests, if FEEDBACK will really time out 
// when started freshly, when one tries to get the CALIBRATION Service.

// so this script will run fine, when FEEDBACK already had a CALIBRATION in the past
// and it will throw an exception after the long time of 50sec, when 
// FEEDBACK had never ever had a CALIBRATION (e.g. becauce it was started just a minute ago)
var service_calibration = new Subscription("FEEDBACK/CALIBRATION");
var data_calibration = service_calibration.get(50000);

