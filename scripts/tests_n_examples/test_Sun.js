'use strict';
var date = new Date(); // Date in UTC
console.out(date);
console.out("------------------------------");
console.out(" - no params: -");
console.out( JSON.stringify( Sun.horizon()  ) );
console.out('params: ("astro") ');
console.out( JSON.stringify( Sun.horizon("astro") ) );
console.out('params: (-12, date) ');
console.out(JSON.stringify(Sun.horizon(-12, date ) ) ); // nautical
console.out('params: ("FACT") ');
console.out(JSON.stringify(Sun.horizon("FACT") ) );
console.out("------------------------------");
console.out("-----------time when LidOpen() will work-----------");
console.out('nautical');
console.out(JSON.stringify(Sun.horizon("nautical" ) ) ); // nautical
