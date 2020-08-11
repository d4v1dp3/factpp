'use strict';

// ==========================================================================
// Reset Crate
// ==========================================================================

// call it with: .js doCrateReset.js crate0=true crate3=true
//           or: DIM_CONTROL/START doCrateReset.js crate0=true crate3=true

// -------------------------------------------------------------------------

include('scripts/CheckStates.js');

var crates =
[
     $['camera']=='true' || $['crate0']=='true',
     $['camera']=='true' || $['crate1']=='true',
     $['camera']=='true' || $['crate2']=='true',
     $['camera']=='true' || $['crate3']=='true'
];

include('scripts/crateReset.js');

crateReset(crates);
