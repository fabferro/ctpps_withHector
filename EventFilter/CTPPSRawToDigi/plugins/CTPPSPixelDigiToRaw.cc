#include "CTPPSPixelDigiToRaw.h"

#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Framework/interface/ESHandle.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "DataFormats/Common/interface/DetSetVector.h"
#include "DataFormats/CTPPSDigi/interface/CTPPSPixelDigi.h"
#include "DataFormats/FEDRawData/interface/FEDRawDataCollection.h"
#include "DataFormats/FEDRawData/interface/FEDRawData.h"

#include "CondFormats/CTPPSReadoutObjects/interface/CTPPSPixelDAQMapping.h"
#include "CondFormats/DataRecord/interface/CTPPSPixelDAQMappingRcd.h"

#include "EventFilter/CTPPSRawToDigi/interface/CTPPSPixelDataFormatter.h"

//raw test
#include "DataFormats/DetId/interface/DetIdCollection.h"
#include "DataFormats/FEDRawData/interface/FEDNumbering.h"
#include "EventFilter/CTPPSRawToDigi/interface/CTPPSPixelRawToDigi.h"

using namespace std;

CTPPSPixelDigiToRaw::CTPPSPixelDigiToRaw( const edm::ParameterSet& pset ) :
  config_(pset)
{

  tCTPPSPixelDigi = consumes<edm::DetSetVector<CTPPSPixelDigi> >(config_.getParameter<edm::InputTag>("InputLabel")); 

  // Define EDProduct type
  produces<FEDRawDataCollection>();

  // start the counters
  eventCounter = 0;
  allDigiCounter = 0;
  allWordCounter = 0;

}

// -----------------------------------------------------------------------------
CTPPSPixelDigiToRaw::~CTPPSPixelDigiToRaw() {
  edm::LogInfo("CTPPSPixelDigiToRaw")  << " CTPPSPixelDigiToRaw destructor!";

}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
void CTPPSPixelDigiToRaw::produce( edm::Event& ev,
                              const edm::EventSetup& es)
{
  eventCounter++;
  edm::LogInfo("CTPPSPixelDigiToRaw") << "[CTPPSPixelDigiToRaw::produce] "
                                   << "event number: " << eventCounter;

  edm::Handle< edm::DetSetVector<CTPPSPixelDigi> > digiCollection;
  label_ = config_.getParameter<edm::InputTag>("InputLabel");
  ev.getByToken( tCTPPSPixelDigi, digiCollection);

  CTPPSPixelDataFormatter::RawData rawdata;
  CTPPSPixelDataFormatter::Digis digis;
  typedef vector< edm::DetSet<CTPPSPixelDigi> >::const_iterator DI;

  int digiCounter = 0; 
  for (DI di=digiCollection->begin(); di != digiCollection->end(); di++) {
    digiCounter += (di->data).size(); 
    digis[ di->id] = di->data;
  }
  allDigiCounter += digiCounter;
   edm::ESHandle<CTPPSPixelDAQMapping> mapping;
//  if (recordWatcher.check( es )) {
    //es.get<CTPPSPixelDAQMappingRcd>().get(mapping);
    es.get<CTPPSPixelDAQMappingRcd>().get("RPix",mapping);
    for (const auto &p : mapping->ROCMapping)    {
        const uint32_t piD = p.second.iD;  
        short unsigned int pROC   = p.second.roc;
        short unsigned int pFediD = p.first.getFEDId();
        short unsigned int pFedcH = p.first.getChannelIdx();
	edm::LogInfo("-----------------------------------------------------------+- ") << " id roc, fed ch = " << piD << " "<< pROC << " "<< pFediD << " "<< pFedcH  ;
	std::map<const uint32_t,short unsigned int> mapDetRoc; 
        mapDetRoc.insert(std::pair<const uint32_t,short unsigned int>(piD,pROC)); 
	std::map<short unsigned int,short unsigned int> mapFedIdCh; 
        mapFedIdCh.insert(std::pair<short unsigned int,short unsigned int>(pFediD,pFedcH)); 

	iDdet2fed_.insert(std::pair<std::map<const uint32_t,short unsigned int> ,std::map<short unsigned int,short unsigned int>>(mapDetRoc,mapFedIdCh));
    }
    fedIds_ = mapping->fedIds();
//  }
  //debug = edm::MessageDrop::instance()->debugEnabled;


  CTPPSPixelDataFormatter formatter(mapping->ROCMapping);

  // create product (raw data)
  auto buffers = std::make_unique<FEDRawDataCollection>();

  // convert data to raw
  //if(digis.size() != 0 )

  formatter.formatRawData( ev.id().event(), rawdata, digis, iDdet2fed_);
  bool data_exist = false; 
  // pack raw data into collection
  for (auto it = fedIds_.begin(); it != fedIds_.end(); it++) { 
    FEDRawData& fedRawData = buffers->FEDData( *it );
    CTPPSPixelDataFormatter::RawData::iterator fedbuffer = rawdata.find( *it );
    if( fedbuffer != rawdata.end() ) fedRawData = fedbuffer->second;

    int nWords = fedRawData.size()/sizeof(Word64);
    if (nWords!=0) data_exist = true; 
  }
  if(data_exist) { 
	allWordCounter += formatter.nWords();

	if (debug) LogDebug("CTPPSPixelDigiToRaw") 
	        << "Words/Digis this ev: "<<digiCounter<<"(fm:"<<formatter.nDigis()<<")/"
        	<<formatter.nWords()
	        <<"  all: "<< allDigiCounter <<"/"<<allWordCounter;

	ev.put(std::move(buffers));
  }
}
