#ifndef FACT_FAD_H
#define FACT_FAD_H

//---------------------------------------------------------------
//
// FAD internal structures
//
//---------------------------------------------------------------

#define NTemp          4
#define NDAC           8

typedef struct {

  uint16_t start_package_flag;
  uint16_t package_length;
  uint16_t version_no;
  uint16_t PLLLCK;

  uint16_t trigger_crc;
  uint16_t trigger_type;
  uint32_t trigger_id;

  uint32_t fad_evt_counter;
  uint32_t REFCLK_frequency;

  uint16_t board_id;
  uint8_t  zeroes;
   int8_t  adc_clock_phase_shift;
  uint16_t number_of_triggers_to_generate;
  uint16_t trigger_generator_prescaler;

  uint64_t DNA;

  uint32_t time;
  uint32_t runnumber;

  int16_t  drs_temperature[NTemp];

  uint16_t  dac[NDAC];

} __attribute__((__packed__)) PEVNT_HEADER;

typedef struct {
uint16_t id;
uint16_t start_cell;
uint16_t roi;
uint16_t filling;
 int16_t adc_data[];
} __attribute__((__packed__)) PCHANNEL;


typedef struct {
uint16_t package_crc;
uint16_t end_package_flag;
} __attribute__((__packed__)) PEVNT_FOOTER;

#define NBOARDS      40      // max. number of boards
#define NPIX       1440      // max. number of pixels
#define NTMARK      160      // max. number of timeMarker signals

//---------------------------------------------------------------
//
// Data structures
//
//---------------------------------------------------------------

typedef struct _EVENT {
  uint16_t Roi ;            // #slices per pixel (same for all pixels)
  uint16_t RoiTM ;          // #slices per pixel (same for all tmarks) [ 0 or Roi ]
  uint32_t EventNum ;       // EventNumber as from FADs
  uint32_t TriggerNum ;     // EventNumber as from FTM
  uint16_t TriggerType ;    // Trigger Type from FTM

  uint32_t NumBoards ;      // number of active boards included

  uint32_t PCTime ;         // epoch
  uint32_t PCUsec ;         // micro-seconds

  uint32_t BoardTime[NBOARDS];//

   int16_t StartPix[NPIX];  // First Channel per Pixel (Pixels sorted according Software ID)  ; -1 if not filled

   int16_t StartTM[NTMARK]; // First Channel for TimeMark (sorted Hardware ID) ; -1 if not filled

   int16_t Adc_Data[];     // final length defined by malloc ....

} __attribute__((__packed__)) EVENT ;

//---------------------------------------------------------------

struct RUN_HEAD
{
  uint32_t Version ;
  uint32_t RunType ;
  uint32_t RunTime ;  //unix epoch for first event
  uint32_t RunUsec ;  //microseconds
  uint16_t NBoard  ;  //#boards (always 40)
  uint16_t NPix ;     //#pixels (always 1440)
  uint16_t NTm  ;     //#TM     (always 160)
  uint16_t Nroi ;     //roi for pixels
  uint16_t NroiTM ;   //roi for TM  <=0 if TM is empty 

//headers of all FAD-boards for first event ==> all FAD configs
  PEVNT_HEADER FADhead[NBOARDS];    // [ NBoards ] sorted Board Headers (according Hardware ID)

  RUN_HEAD() : Version(1), RunType(-1), NBoard(NBOARDS), NPix(NPIX), NTm(NTMARK)
  {
  }


//do we also have info about FTM config we want to add here ???
} __attribute__((__packed__));


//---------------------------------------------------------------

// FIXME: This doesn't neet to be here... it is inlcuded in all
//        data processors

#include <netinet/in.h>

typedef struct {
   struct sockaddr_in sockAddr ;
   int    sockDef ; //<0 not defined/ ==0 not to be used/ >0 used
} FACT_SOCK ;    //internal to eventbuilder


//---------------------------------------------------------------

typedef struct {
  //info about (current state of) the buffer 
   uint32_t bufNew ;            //# incomplete events in buffer (evtCtrl)
   uint32_t bufEvt ;            //# complete events in buffer  (primaryQueue)
   uint32_t bufWrite ;          //# events in write queue (secondaryQueue)
   uint32_t bufProc ;           //# events in processing queue (processingQueue1)
   uint32_t bufTot ;            //# total events currently in buffer (this corresponds to totMem)

   uint64_t totMem;             //# Bytes available in Buffer
   uint64_t usdMem;             //# Bytes currently used
   uint64_t maxMem;             //max # Bytes used during past cycle

  //rates
   int32_t  deltaT ;            //time in milli-seconds for rates
   int32_t  rateNew ;           //#New start events recieved
   int32_t  rateWrite ;         //#Complete events written (or flushed)

  //connections
   int8_t   numConn[NBOARDS] ;  //#connections per board (at the moment)
   uint32_t rateBytes[NBOARDS];  //#Bytes read (counter)
   int32_t  relBytes[NBOARDS];   //#Bytes read this cycle  **

  // ** // if counter and rates exist, do only update the rates in
  // ** // real time; 
  // ** // counters will be updated only once per cycle based on rates

}  __attribute__((__packed__)) GUI_STAT ;         //EventBuilder Status

#endif
