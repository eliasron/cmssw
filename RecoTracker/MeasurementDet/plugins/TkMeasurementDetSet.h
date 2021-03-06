#ifndef StMeasurementDetSet_H
#define StMeasurementDetSet_H

#include<vector>
class TkStripMeasurementDet;
class TkGluedMeasurementDet;
class SiStripRecHitMatcher;
class StripClusterParameterEstimator;

#include "DataFormats/Common/interface/DetSetVector.h"
#include "DataFormats/Common/interface/DetSetVectorNew.h"
#include "DataFormats/SiStripCluster/interface/SiStripCluster.h"
#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/Common/interface/RefGetter.h"

#include "CondFormats/SiStripObjects/interface/SiStripBadStrip.h"
#include "CalibFormats/SiStripObjects/interface/SiStripQuality.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"

/* Struct of arrays supporting "members of Tk...MeasurementDet
 * implemented with vectors, to be optimized...
 */
class StMeasurementDetSet {
public:

  typedef edmNew::DetSet<SiStripCluster> StripDetset;
  typedef StripDetset::const_iterator new_const_iterator;
  
  typedef std::vector<SiStripCluster>::const_iterator const_iterator;
  
  typedef edm::LazyGetter<SiStripCluster> LazyGetter;
  typedef edm::RefGetter<SiStripCluster> RefGetter;

   enum QualityFlags { BadModules=1, // for everybody
                       /* Strips: */ BadAPVFibers=2, BadStrips=4, MaskBad128StripBlocks=8 }; 


  struct BadStripCuts {
    BadStripCuts() : maxBad(9999), maxConsecutiveBad(9999) {}
    BadStripCuts(const edm::ParameterSet &pset) :
      maxBad(pset.getParameter<uint32_t>("maxBad")),
      maxConsecutiveBad(pset.getParameter<uint32_t>("maxConsecutiveBad")) {}
    uint16_t maxBad, maxConsecutiveBad;
  };
  
  struct BadStripBlock {
    short first;
    short last;
    BadStripBlock(const SiStripBadStrip::data &data) : first(data.firstStrip), last(data.firstStrip+data.range-1) { }
  };
  
  
  StMeasurementDetSet(const SiStripRecHitMatcher* matcher,
		      const StripClusterParameterEstimator* cpe,
		      bool regional):
    theMatcher(matcher), theCPE(cpe), regional_(regional){}
  
  
  void init(std::vector<TkStripMeasurementDet> & stripDets);

   
  std::vector<bool>  & clusterToSkip() const { return theStripsToSkip; }
  
  void setLazyGetter( edm::Handle<LazyGetter> const & lg) { regionalHandle_=lg;}
 
  void update(int i,const StripDetset & detSet ) { 
    detSet_[i] = detSet;     
    empty_[i] = false;
  }
  
  void update(int i, std::vector<SiStripCluster>::const_iterator begin ,std::vector<SiStripCluster>::const_iterator end) { 
    clusterI_[2*i] = begin - regionalHandle_->begin_record();
    clusterI_[2*i+1] = end - regionalHandle_->begin_record();
    
    empty_[i] = false;
    activeThisEvent_[i] = true;
  }
  
  
  const SiStripRecHitMatcher*  matcher() const { return theMatcher;}
  const StripClusterParameterEstimator*  stripCPE() const { return theCPE;}


  int nDet() const { return id_.size();}

  unsigned int id(int i) const { return id_[i]; }
  unsigned char subId(int i) const { return subId_[i];}

  int find(unsigned int jd, int i=0) {
    return std::lower_bound(id_.begin()+i,id_.end(),jd)-id_.begin();
  }
  
  bool isRegional() const { return regional_;}
  bool empty(int i) const { return empty_[i];}  
  bool isActive(int i) const { return activeThisEvent_[i] && activeThisPeriod_[i]; }

  void setEmpty(int i) {empty_[i] = true; activeThisEvent_[i] = true; }
  
  void setEmpty() {
    std::fill(empty_.begin(),empty_.end(),true);
    std::fill(activeThisEvent_.begin(), activeThisEvent_.end(),true);
  }
  
  /** \brief Turn on/off the module for reconstruction, for the full run or lumi (using info from DB, usually).
      This also resets the 'setActiveThisEvent' to true */
  void setActive(int i, bool active) { activeThisPeriod_[i] = active; activeThisEvent_[i] = true; if (!active) empty_[i] = true; }
  /** \brief Turn on/off the module for reconstruction for one events.
      This per-event flag is cleared by any call to 'update' or 'setEmpty'  */
  void setActiveThisEvent(int i, bool active) { activeThisEvent_[i] = active;  if (!active) empty_[i] = true; }
  
  
  edm::Handle<edmNew::DetSetVector<SiStripCluster> > & handle() {  return handle_;}
  StripDetset & detSet(int i) { return detSet_[i];}
  
  edm::Handle<edm::LazyGetter<SiStripCluster> > & regionalHandle() { return regionalHandle_;}
  unsigned int beginClusterI(int i) const {return clusterI_[2*i];}
  unsigned int endClusterI(int i) const {return clusterI_[2*i+1];}
  
  int totalStrips(int i) const { return totalStrips_[i];}
  
  
  
  void setMaskBad128StripBlocks(bool maskThem) { maskBad128StripBlocks_ = maskThem; }
  BadStripCuts & badStripCuts(int i) { return  badStripCuts_[subId_[i]];}
  
  
  bool maskBad128StripBlocks() const { return maskBad128StripBlocks_;}
  bool hasAny128StripBad(int i) const { return  hasAny128StripBad_[i];}
  
  std::vector<BadStripBlock> & getBadStripBlocks(int i) { return badStripBlocks_[i]; }
  std::vector<BadStripBlock> const & badStripBlocks(int i) const {return badStripBlocks_[i]; }


  bool isMasked(int i, const SiStripCluster &cluster) const {
    int offset =  nbad128*i;
    if ( bad128Strip_[offset+( cluster.firstStrip() >> 7)] ) {
      if ( bad128Strip_[offset+( (cluster.firstStrip()+cluster.amplitudes().size())  >> 7)] ||
	   bad128Strip_[offset+( static_cast<int32_t>(cluster.barycenter()-0.499999) >> 7)] ) {
	return true;
      }
    } else {
      if ( bad128Strip_[offset+( (cluster.firstStrip()+cluster.amplitudes().size())  >> 7)] &&
	   bad128Strip_[offset+( static_cast<int32_t>(cluster.barycenter()-0.499999) >> 7)] ) {
	return true;
      }
    }
    return false;
  }
  
  
  void set128StripStatus(int i, bool good, int idx=-1);  

  void initializeStripStatus(const SiStripQuality *quality, int qualityFlags, int qualityDebugFlags, const edm::ParameterSet& cutPset);

private:
  
  friend class  MeasurementTrackerImpl;
  
  // globals
  const SiStripRecHitMatcher*       theMatcher;
  const StripClusterParameterEstimator* theCPE;
  
  edm::Handle<edmNew::DetSetVector<SiStripCluster> > handle_;
  edm::Handle<edm::LazyGetter<SiStripCluster> > regionalHandle_;
  
  mutable std::vector<bool> theStripsToSkip;
  
  bool regional_;
  
  mutable bool maskBad128StripBlocks_;
  BadStripCuts badStripCuts_[4];
  
  // members of TkStripMeasurementDet
  std::vector<unsigned int> id_;
  std::vector<unsigned char> subId_;
  
  std::vector<int> totalStrips_;
  
  static const int nbad128 = 6;
  std::vector<bool> bad128Strip_;
  std::vector<bool> hasAny128StripBad_;
  
  std::vector<std::vector<BadStripBlock>> badStripBlocks_;  


  std::vector<bool> empty_;
  
  std::vector<bool> activeThisEvent_,activeThisPeriod_;
  
  // full reco
  std::vector<StripDetset> detSet_;
  
  // --- regional unpacking

  // begin,end "pairs"
  std::vector<unsigned int> clusterI_;
  
  
};


#endif // StMeasurementDetSet_H
