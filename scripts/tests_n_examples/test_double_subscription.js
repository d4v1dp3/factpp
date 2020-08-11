'use strict';

// Subscribe to the the services (returns a handle to each of them)
var w = new Subscription("MAGIC_WEATHER/DATA");

// Name which corresponds to handle
console.out(w.name);

// Wait until a valid service object is in the internal buffer
while (!w.get(-1))
    v8.sleep(100);

// Make sure that the service description for this service is available
// This allows to access the service values by name (access by index
// is always possible)
while (!w.get().obj)
    v8.sleep(100);

console.out("have data");


console.out("double subscription");
var ww = new Subscription("MAGIC_WEATHER/DATA");

console.out("closing 1st subscription.");

var rc = w.close();

console.out(" closing worked? "+rc);
console.out("is 2nd subscription still open? "+ww.isOpen());

if (ww.isOpen())
{
    console.out("was still open?! ... closing it ...");
    ww.close();
}
else
{
    console.out("what if we close an already closed subscription?");
    ww.close();
}

