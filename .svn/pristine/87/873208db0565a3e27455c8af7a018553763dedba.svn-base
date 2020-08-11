'use strict';

// Subscribe to the the services (returns a handle to each of them)
var w = new Subscription("MAGIC_WEATHER/DATA");
var x = new Subscription("TNG_WEATHER/DUST");
var y = new Subscription("TNG_WEATHER/CLIENT_LIST");

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

// get the current service data
var d = w.get();

// Here is a summary:
//    d.obj===undefined: no data received yet
//    d.obj!==undefined, d.obj.length==0: valid names are available, received data empty (d.data===null)
//    obj!==undefined, obj.length>0: valid names are available, data received
//
//    d.data===undefined: no data received yet
//    d.data===null: event received, but contains no data
//    d.data.length>0: event received, contains data

console.out("Format: "+d.format); // Dim format string
console.out("Counter: "+d.counter); // How many service object have been received so far?
console.out("Time: "+d.time); // Which time is attached to the data?
console.out("QoS: "+d.qos); // Quality-of-Service parameter
console.out("Length: "+d.data.length); // Number of entries in data array
console.out("Data: "+d.data); // Print array

// Or to plot the whole contents, you can do
console.out(JSON.stringify(d));
console.out(JSON.stringify(d.data));
console.out(JSON.stringify(d.obj));

// Loop over all service properties by name
for (var name in d.obj)
{
    console.out("obj." + name + "=" + d.obj[name]);
}

// Loop over all service properties by index
for (var i=0; i<d.data.length; i++)
{
    console.out("data["+ i +"]="+ d.data[i]);
}

// Note that in case of formats like F:160, the entries in data
// might be arrays themselves

var cnt = d.counter;

// Print counter and time
console.out("Time: "+d.counter+" - "+d.time);

// Wait until at least one new event has been received
while (cnt==d.counter)
{
    v8.sleep(1000);
    d = w.get();
}

// Print counter and time of new event (usually counter+1, but this is
// not guranteed) We can have missed service objects
console.out("Time: "+d.counter+" - "+d.time);

// Access the Wind property of the weather data
console.out("Wind: "+d.obj.v);
console.out("Wind: "+d.obj['v']);

// Get the dust and client_list
var xx = x.get();
var yy = y.get();

// data contains only a single value. No array indexing required
console.out("Dust: "+xx.data);
console.out("CL:   "+yy.data);

// Service is still open
console.out(w.isOpen);

// Close the subscription (unsubscribe from the dim-service)
// Tells you if the service was still subscribed or not
var rc = w.close();

// Service is not subscribed anymore
console.out(w.isOpen);
console.out(rc)


rc = x.close();
console.out(rc)
rc = y.close();
console.out(rc)


