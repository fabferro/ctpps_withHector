#include "EventFilter/CTPPSRawToDigi/interface/CTPPSPixelDataFormatter.h"

#include "DataFormats/FEDRawData/interface/FEDRawData.h"
#include "DataFormats/FEDRawData/interface/FEDHeader.h"
#include "DataFormats/FEDRawData/interface/FEDTrailer.h"

#include "CondFormats/CTPPSReadoutObjects/interface/CTPPSPixelROC.h" //KS

#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include <bitset>
#include <sstream>
#include <iostream>

using namespace std;
using namespace edm;
using namespace ctppspixelobjects;

namespace {
  constexpr int m_LINK_bits = 6;
  constexpr int m_ROC_bits  = 5;
  constexpr int m_DCOL_bits = 5;
  constexpr int m_PXID_bits = 8;
  constexpr int m_ADC_bits  = 8;
  constexpr int min_Dcol = 0;
  constexpr int max_Dcol = 25;
  constexpr int min_Pixid = 2;
  constexpr int max_Pixid = 161;
}

CTPPSPixelDataFormatter::CTPPSPixelDataFormatter(std::map<CTPPSPixelFramePosition, CTPPSPixelROCInfo> const &mapping)  :  theWordCounter(0), mapping_(mapping)
{
  int s32 = sizeof(Word32);
  int s64 = sizeof(Word64);
  int s8  = sizeof(char);
  if ( s8 != 1 || s32 != 4*s8 || s64 != 2*s32) {
    LogError("UnexpectedSizes")
      <<" unexpected sizes: "
      <<"  size of char is: " << s8
      <<", size of Word32 is: " << s32
      <<", size of Word64 is: " << s64
      <<", send exception" ;
  }


  m_ADC_shift  = 0;
  m_PXID_shift = m_ADC_shift + m_ADC_bits;
  m_DCOL_shift = m_PXID_shift + m_PXID_bits;
  m_ROC_shift  = m_DCOL_shift + m_DCOL_bits;


  m_LINK_shift = m_ROC_shift + m_ROC_bits;
  m_LINK_mask = ~(~CTPPSPixelDataFormatter::Word32(0) << m_LINK_bits);
  m_ROC_mask  = ~(~CTPPSPixelDataFormatter::Word32(0) << m_ROC_bits);    

  maxROCIndex=3; 

  m_DCOL_mask = ~(~CTPPSPixelDataFormatter::Word32(0) << m_DCOL_bits);
  m_PXID_mask = ~(~CTPPSPixelDataFormatter::Word32(0) << m_PXID_bits);
  m_ADC_mask  = ~(~CTPPSPixelDataFormatter::Word32(0) << m_ADC_bits);

  allDetDigis = 0;
  hasDetDigis = 0;
}

void CTPPSPixelDataFormatter::interpretRawData(  bool& errorsInEvent, int fedId, const FEDRawData& rawData, Collection & digis)
{

  int nWords = rawData.size()/sizeof(Word64);
  if (nWords==0) return;

/// check CRC bit
  const Word64* trailer = reinterpret_cast<const Word64* >(rawData.data())+(nWords-1);  
  if(!errorcheck.checkCRC(errorsInEvent, fedId, trailer)) return;

/// check headers
  const Word64* header = reinterpret_cast<const Word64* >(rawData.data()); header--;
  bool moreHeaders = true;
  while (moreHeaders) {
    header++;
    LogTrace("")<<"HEADER:  " <<  print(*header);
    bool headerStatus = errorcheck.checkHeader(errorsInEvent, fedId, header);
    moreHeaders = headerStatus;
  }

/// check trailers
  bool moreTrailers = true;
  trailer++;
  while (moreTrailers) {
    trailer--;
    LogTrace("")<<"TRAILER: " <<  print(*trailer);
    bool trailerStatus = errorcheck.checkTrailer(errorsInEvent, fedId, nWords, trailer);
    moreTrailers = trailerStatus;
  }

/// data words
  theWordCounter += 2*(nWords-2);
  LogTrace("")<<"data words: "<< (trailer-header-1);

  int link = -1;
  int roc  = -1;

  bool skipROC=false;

  edm::DetSet<CTPPSPixelDigi> * detDigis=nullptr;

  const  Word32 * bw =(const  Word32 *)(header+1);
  const  Word32 * ew =(const  Word32 *)(trailer);
  if ( *(ew-1) == 0 ) { ew--;  theWordCounter--;}
  for (auto word = bw; word < ew; ++word) {
    LogTrace("")<<"DATA: " <<  print(*word);

    auto ww = *word;
    if unlikely(ww==0) { theWordCounter--; continue;}
    int nlink = (ww >> m_LINK_shift) & m_LINK_mask; 
    int nroc  = (ww >> m_ROC_shift) & m_ROC_mask;

    int FMC = 0;

    int convroc = nroc-1;
    CTPPSPixelFramePosition fPos(fedId, FMC, nlink, convroc);
    edm::LogInfo("CTPPSPixelDataFormatter:interpreting") << " fedId, FMC, nlink, convroc " << fedId<< " " << FMC<< " " << nlink<< " " << convroc;
    std::map<CTPPSPixelFramePosition, CTPPSPixelROCInfo>::const_iterator mit;
    mit = mapping_.find(fPos);

    if (mit == mapping_.end()){      
      if((nroc-1)>=maxROCIndex){
	errorcheck.checkROC(errorsInEvent, fedId,  ww); // check kind of error
      }else{
	edm::LogError("")<< " CTPPS Pixel DAQ map error " ;
      }
      continue; //skip word
    }

    CTPPSPixelROCInfo rocInfo = (*mit).second;

    CTPPSPixelROC rocp(rocInfo.iD, rocInfo.roc, convroc);

    if ( (nlink!=link) | (nroc!=roc) ) {  // new roc
      link = nlink; roc=nroc;

      skipROC = likely((roc-1)<maxROCIndex) ? false : !errorcheck.checkROC(errorsInEvent, fedId,  ww); 
      if (skipROC) continue;

      auto rawId = rocp.rawId();

      detDigis = &digis.find_or_insert(rawId);
      if ( (*detDigis).empty() ) (*detDigis).data.reserve(32); // avoid the first relocations

    }
  
    int adc  = (ww >> m_ADC_shift) & m_ADC_mask;
 

    int dcol = (ww >> m_DCOL_shift) & m_DCOL_mask;
    int pxid = (ww >> m_PXID_shift) & m_PXID_mask;

    if(dcol<min_Dcol || dcol>max_Dcol || pxid<min_Pixid || pxid>max_Pixid){
      edm::LogError("CTPPSPixelDataFormatter")<< " unphysical dcol and/or pxid "  << " nllink=" << nlink 
					      << " nroc="<< nroc << " adc=" << adc << " dcol=" << dcol << " pxid=" << pxid;
      continue;
    }

    std::pair<int,int> rocPixel;
    std::pair<int,int> modPixel;

    rocPixel = std::make_pair(dcol,pxid);

    modPixel = rocp.toGlobalfromDcol(rocPixel);

    CTPPSPixelDigi testdigi(modPixel.first, modPixel.second, adc);

    if(detDigis)
    (*detDigis).data.emplace_back( modPixel.first, modPixel.second, adc); 
 
  }

}


void CTPPSPixelDataFormatter::formatRawData(unsigned int lvl1_ID, RawData & fedRawData, const
Digis & digis, std::map<std::map<const uint32_t,short unsigned int>, std::map<short unsigned int,short unsigned int>> iDdet2fed)
{
  std::map<int, vector<Word32> > words;

  // translate digis into 32-bit raw words and store in map indexed by Fed
  //

  for (Digis::const_iterator im = digis.begin(); im != digis.end(); im++) {
    //if (mapping_.size== 0 ) return; 
    allDetDigis++;
    cms_uint32_t rawId = im->first;
      edm::LogInfo("--- RPix") << " \t\t digi rawId = " << rawId;

    hasDetDigis++;
    const DetDigis & detDigis = im->second;
    for (DetDigis::const_iterator it = detDigis.begin(); it != detDigis.end(); it++) {
        theDigiCounter++;

        const CTPPSPixelDigi & digi = (*it);
        int matchRoc = 999, matchfedId = 999, matchfedCh = 999, nroc = 999, nlink = 999;
        int rocPixelRow = -1, rocPixelColumn = -1, rocID = -1;
        int modulePixelColumn = digi.column();
        int modulePixelRow = digi.row();

        theIndices.transformToROC(modulePixelColumn, modulePixelRow, rocID, rocPixelColumn, rocPixelRow);
        const int dcol = theIndices.DColumn(rocPixelColumn);
        const int pxid =  2*(ROCSizeInX-rocPixelRow)+ (rocPixelColumn%2);
        for (auto &p : iDdet2fed) {
           for (auto &pf : p.first){
              cms_uint32_t prawId = pf.first;
              if (prawId == rawId){
                matchRoc = pf.second;
                if (matchRoc == rocID){
                   for (auto &ps : p.second){
                      matchfedId = ps.first;
                      matchfedCh = ps.second;
                      nlink = matchfedCh;
                      int nnlink = 0;
                      if((nlink==9)||(nlink==10)) nnlink = -1;

                      if (matchRoc < 3) nroc = matchRoc+1;
                      if (matchRoc > 2) nroc = matchRoc+nnlink-2;

                      CTPPSElectronicIndex cabling = {nlink, nroc, dcol, pxid};

                      cms_uint32_t word =
                               (cabling.link << m_LINK_shift)
                             | (cabling.roc  << m_ROC_shift)
                             | (cabling.dcol << m_DCOL_shift)
                             | (cabling.pxid << m_PXID_shift)
                             | (digi.adc() << m_ADC_shift);
		      edm::LogInfo("CTPPSPixelDataFormatter:formatting") << " fedId, FMC, nlink, convroc " << matchfedId<< " " << 0<< " " << cabling.link<< " " << cabling.roc << " " << cabling.dcol <<" "<<cabling.pxid ;
                      words[matchfedId].push_back(word);
                      theWordCounter++;
                   }
                 } //if rocID
              } // if prawId
           }
        }
    } // for DetDigis
  } // for Digis
  LogTrace(" allDetDigis/hasDetDigis : ") << allDetDigis<<"/"<<hasDetDigis;
  typedef std::map<int, vector<Word32> >::const_iterator RI;
  for (RI feddata = words.begin(); feddata != words.end(); feddata++) {
    int fedId = feddata->first;

    // since raw words are written in the form of 64-bit packets
    // add extra 32-bit word to make number of words even if necessary
    if (words.find(fedId)->second.size() %2 != 0) words[fedId].push_back( Word32(0) );

    // size in Bytes; create output structure
    int dataSize = words.find(fedId)->second.size() * sizeof(Word32);
    int nHeaders = 1;
    int nTrailers = 1;
    dataSize += (nHeaders+nTrailers)*sizeof(Word64);

    FEDRawData * rawData = new FEDRawData(dataSize);

    // get begining of data;
    Word64 * word = reinterpret_cast<Word64* >(rawData->data());

    // write one header
    FEDHeader::set(  reinterpret_cast<unsigned char*>(word), 0, lvl1_ID, 0, fedId);
    word++;

    // write data
    unsigned int nWord32InFed = words.find(fedId)->second.size();
    for (unsigned int i=0; i < nWord32InFed; i+=2) {
      *word = (Word64(words.find(fedId)->second[i]) << 32 ) | words.find(fedId)->second[i+1];
      LogDebug("CTPPSPixelDataFormatter")  << print(*word);
      word++;
    }

    // write one trailer
    FEDTrailer::set(  reinterpret_cast<unsigned char*>(word), dataSize/sizeof(Word64), 0,0,0);
    word++;

    // check memory
    if (word != reinterpret_cast<Word64* >(rawData->data()+dataSize)) {
      string s = "** PROBLEM in CTPPSPixelDataFormatter !!!";
      LogError("CTPPSPixelDataFormatter") << "** PROBLEM in CTPPSPixelDataFormatter !!!";
      throw cms::Exception(s);
    } // if (word !=
    fedRawData[fedId] = *rawData;
    delete rawData;
  } // for (RI feddata 
}

std::string CTPPSPixelDataFormatter::print(const  Word64 & word) const
{
  ostringstream str;
  str  <<"word64:  " << reinterpret_cast<const bitset<64>&> (word);
  return str.str();
}
