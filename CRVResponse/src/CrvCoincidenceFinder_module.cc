//
// A module to find clusters of coincidences of CRV pulses
//
// 
// Original Author: Ralf Ehrlich

#include "Offline/CosmicRayShieldGeom/inc/CosmicRayShield.hh"
#include "Offline/DataProducts/inc/CRSScintillatorBarIndex.hh"

#include "Offline/GeometryService/inc/GeomHandle.hh"
#include "Offline/GeometryService/inc/GeometryService.hh"
#include "Offline/RecoDataProducts/inc/CrvRecoPulse.hh"
#include "Offline/RecoDataProducts/inc/CrvCoincidenceCluster.hh"
#include "Offline/CRVResponse/inc/CrvHelper.hh"

#include "canvas/Persistency/Common/Ptr.h"
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/Table.h"
#include "fhiclcpp/types/Sequence.h"

#include <string>

namespace mu2e 
{
  class CrvCoincidenceFinder : public art::EDProducer 
  {
    public:
    struct SectorConfig
    {
      using Name=fhicl::Name;
      using Comment=fhicl::Comment;
      fhicl::Atom<std::string> CRVSector{Name("CRVSector"), Comment("CRV sector")};
      fhicl::Atom<int> PEthreshold{Name("PEthreshold"), Comment("PE threshold required for a coincidence")};
      fhicl::Atom<double> maxTimeDifferenceAdjacentPulses{Name("maxTimeDifferenceAdjacentPulses"), Comment("maximum time difference of pulses of adjacent channels considered for coincidences")};
      fhicl::Atom<double> maxTimeDifference{Name("maxTimeDifference"), Comment("maximum time difference of a coincidence hit combination")};
      fhicl::Atom<double> maxSlope{Name("maxSlope"), Comment("maximum slope allowed for a coincidence")};
      fhicl::Atom<double> maxSlopeDifference{Name("maxSlopeDifference"), Comment("maximum slope difference between layers allowed for a coincidence")};
      fhicl::Atom<int> coincidenceLayers{Name("coincidenceLayers"), Comment("number of layers required for a coincidence")};
    };
    struct Config
    {
      using Name=fhicl::Name;
      using Comment=fhicl::Comment;
      fhicl::Atom<int> verboseLevel{Name("verboseLevel"), Comment("verbose level")};
      fhicl::Atom<std::string> crvRecoPulsesModuleLabel{Name("crvRecoPulsesModuleLabel"), Comment("module label of the input CrvRecoPulses")};
      //cluster settings
      fhicl::Atom<double> clusterMaxDistance{Name("clusterMaxDistance"), Comment("maximum distances between hits to be considered for a hit cluster")};
      fhicl::Atom<double> clusterMaxTimeDifference{Name("clusterMaxTimeDifference"), Comment("maximum time difference between hits to be considered for a hit cluster (when overlap option is not used)")};
      fhicl::Atom<double> clusterMinOverlapTime{Name("clusterMinOverlapTime"), Comment("minimum overlap time between hits to be considered for a hit cluster (when overlap option is used)")};
      //sector-specific coincidence settings
      fhicl::Sequence<fhicl::Table<SectorConfig> > sectorConfig{Name("sectorConfig"), Comment("sector-specific settings")};
      //coincidence settings for overlap option
      fhicl::Atom<bool> usePulseOverlaps{Name("usePulseOverlaps"), Comment("use pulse overlaps instead of peak times to determine coincidences")};
      fhicl::Atom<double> minOverlapTimeAdjacentPulses{Name("minOverlapTimeAdjacentPulses"), Comment("minimum overlap time between pulses of adjacent channels to be considered for coincidences")};
      fhicl::Atom<double> minOverlapTime{Name("minOverlapTime"), Comment("minimum overlap time between pulses of a coincidence hit combination")};
      //other settings
      fhicl::Atom<bool> useNoFitReco{Name("useNoFitReco"), Comment("use pulse reco results not based on a Gumbel fit")};
      fhicl::Atom<bool> usePEsPulseHeight{Name("usePEsPulseHeight"), Comment("use PEs determined by pulse height instead of pulse area")};
    };

    typedef art::EDProducer::Table<Config> Parameters;

    explicit CrvCoincidenceFinder(const Parameters& config);
    void produce(art::Event& e);
    void beginJob();
    void beginRun(art::Run &run);
    void endJob();

    private:
    int                      _verboseLevel;
    std::string              _crvRecoPulsesModuleLabel;

    double                   _clusterMaxDistance;
    double                   _clusterMaxTimeDifference;
    double                   _clusterMinOverlapTime;

    std::vector<SectorConfig> _sectorConfig;

    bool        _usePulseOverlaps;
    double      _minOverlapTimeAdjacentPulses;
    double      _minOverlapTime;
    bool        _useNoFitReco;
    bool        _usePEsPulseHeight;

    int         _totalEvents;
    int         _totalEventsCoincidence;

    struct sectorCoincidenceProperties
    {
      int  precedingCounters;
      int  nCountersPerModule;
      int  sectorType;
      bool sipmsAtSide0;
      bool sipmsAtSide1;
      int  widthDirection, thicknessDirection;
      int         PEthreshold;
      double      maxTimeDifferenceAdjacentPulses;
      double      maxTimeDifference;
      double      maxSlope, maxSlopeDifference;
      int         coincidenceLayers;
    };
    std::map<int,sectorCoincidenceProperties> _sectorMap;

    struct CrvHit
    {
      art::Ptr<CrvRecoPulse> _crvRecoPulse; 
      CLHEP::Hep3Vector      _pos;
      double _x, _y;
      double _time, _timePulseStart, _timePulseEnd;
      double _PEs;
      int    _layer;
      int    _counter;
      int    _SiPM;
      int    _PEthreshold;
      double _maxTimeDifferenceAdjacentPulses;
      double _maxTimeDifference;
      double _maxSlope, _maxSlopeDifference;
      int    _coincidenceLayers;

      CrvHit(const art::Ptr<CrvRecoPulse> crvRecoPulse, const CLHEP::Hep3Vector &pos, 
             double x, double y, double time, double timePulseStart, double timePulseEnd,
             double PEs, int layer, int counter, int SiPM, int PEthreshold,
             double maxTimeDifferenceAdjacentPulses, double maxTimeDifference,
             double maxSlope, double maxSlopeDifference, int coincidenceLayers) :
               _crvRecoPulse(crvRecoPulse), _pos(pos),  
               _x(x), _y(y), _time(time), _timePulseStart(timePulseStart), _timePulseEnd(timePulseEnd), 
               _PEs(PEs), _layer(layer), _counter(counter), _SiPM(SiPM), _PEthreshold(PEthreshold), 
               _maxTimeDifferenceAdjacentPulses(maxTimeDifferenceAdjacentPulses), _maxTimeDifference(maxTimeDifference),
               _maxSlope(maxSlope), _maxSlopeDifference(maxSlopeDifference), _coincidenceLayers(coincidenceLayers) {}

      bool operator <(const CrvHit &other) const {return _x < other._x;}  //used for ordering in multisets
    };

    void clusterProperties(int crvSectorType, const std::vector<std::vector<CrvHit> > &clusters, 
                           std::unique_ptr<CrvCoincidenceClusterCollection> &crvCoincidenceClusterCollection);
    void filterHits(const std::vector<CrvHit> &hits, std::multiset<CrvHit> &hitsFiltered);
    void findClusters(std::multiset<CrvHit> &hits, std::vector<std::vector<CrvHit> > &clusters);
    void checkCoincidence(const std::vector<CrvHit> &hits, std::multiset<CrvHit> &coincidenceHits);
    bool checkCombination(const std::vector<CrvHit>::const_iterator layerIterators[], int n);

    constexpr static const int nLayers = 4;
  };

  CrvCoincidenceFinder::CrvCoincidenceFinder(const Parameters& conf) :
    art::EDProducer(conf),
    _verboseLevel(conf().verboseLevel()),
    _crvRecoPulsesModuleLabel(conf().crvRecoPulsesModuleLabel()),
    _clusterMaxDistance(conf().clusterMaxDistance()),
    _clusterMaxTimeDifference(conf().clusterMaxTimeDifference()),
    _clusterMinOverlapTime(conf().clusterMinOverlapTime()),
    _sectorConfig(conf().sectorConfig()),
    _usePulseOverlaps(conf().usePulseOverlaps()),
    _minOverlapTimeAdjacentPulses(conf().minOverlapTimeAdjacentPulses()),
    _minOverlapTime(conf().minOverlapTime()),
    _useNoFitReco(conf().useNoFitReco()),
    _usePEsPulseHeight(conf().usePEsPulseHeight()),
    _totalEvents(0),
    _totalEventsCoincidence(0)
  {
    produces<CrvCoincidenceClusterCollection>();
  }

  void CrvCoincidenceFinder::beginJob()
  {
  }

  void CrvCoincidenceFinder::endJob()
  {
    if(_verboseLevel>0)
    {
      std::cout<<"SUMMARY "<<moduleDescription().moduleLabel()<<"    "<<_totalEventsCoincidence<<" / "<<_totalEvents<<" events satisfied coincidence requirements"<<std::endl;
    }
  }

  void CrvCoincidenceFinder::beginRun(art::Run &run)
  {
    //first get some properties of each CRV sector
    //-from the geometry
    //-from the fcl parameters
    GeomHandle<CosmicRayShield> CRS;
    const std::vector<CRSScintillatorShield> &sectors = CRS->getCRSScintillatorShields();
    for(size_t i=0; i<sectors.size(); ++i)
    {
      sectorCoincidenceProperties s;
      s.precedingCounters=0;
      int precedingSector=sectors[i].getPrecedingSector();
      while(precedingSector!=-1)
      {
        int nModules=sectors[precedingSector].nModules();
        int nCountersPerModule=sectors[precedingSector].getCountersPerModule();
        s.precedingCounters+=nModules*nCountersPerModule;
        precedingSector=sectors[precedingSector].getPrecedingSector();
      }

      s.nCountersPerModule=sectors[i].getCountersPerModule();

      s.sectorType=sectors[i].getSectorType();

      s.sipmsAtSide0=sectors[i].getCRSScintillatorBarDetail().hasCMB(0);
      s.sipmsAtSide1=sectors[i].getCRSScintillatorBarDetail().hasCMB(1);

      s.widthDirection=sectors[i].getCRSScintillatorBarDetail().getWidthDirection();
      s.thicknessDirection=sectors[i].getCRSScintillatorBarDetail().getThicknessDirection();

      std::string sectorName = sectors[i].getName().substr(4); //removes the "CRV_" part
      std::vector<SectorConfig>::iterator sectorConfigIter=std::find_if(_sectorConfig.begin(), _sectorConfig.end(),
                                                                        [sectorName](const SectorConfig &s){return s.CRVSector()==sectorName;});
      if(sectorConfigIter==_sectorConfig.end())
        throw std::logic_error("CrvCoincidenceFinder: The geometry has a CRV sector for which no coincidence properties were defined in the fcl file.");

      s.PEthreshold                     = sectorConfigIter->PEthreshold();
      s.maxTimeDifferenceAdjacentPulses = sectorConfigIter->maxTimeDifferenceAdjacentPulses();
      s.maxTimeDifference               = sectorConfigIter->maxTimeDifference();
      s.maxSlope                        = sectorConfigIter->maxSlope();
      s.maxSlopeDifference              = sectorConfigIter->maxSlopeDifference();
      s.coincidenceLayers               = sectorConfigIter->coincidenceLayers();

      _sectorMap[i]=s;
    }
  }

  void CrvCoincidenceFinder::produce(art::Event& event) 
  {
    std::unique_ptr<CrvCoincidenceClusterCollection> crvCoincidenceClusterCollection(new CrvCoincidenceClusterCollection);

    GeomHandle<CosmicRayShield> CRS;

    art::Handle<CrvRecoPulseCollection> crvRecoPulseCollection;
    event.getByLabel(_crvRecoPulsesModuleLabel,"",crvRecoPulseCollection);

    if(crvRecoPulseCollection.product()==NULL) return;

    //loop through all reco pulses
    //distribute them into the crv sector types
    std::map<int, std::vector<CrvHit> > sectorTypeMap;
    for(size_t recoPulseIndex=0; recoPulseIndex<crvRecoPulseCollection->size(); ++recoPulseIndex)
    {
      const art::Ptr<CrvRecoPulse> crvRecoPulse(crvRecoPulseCollection, recoPulseIndex);

      //get information about the counter
      const CRSScintillatorBarIndex &crvBarIndex = crvRecoPulse->GetScintillatorBarIndex();
      int sectorNumber, moduleNumber, layerNumber, barNumber;
      CrvHelper::GetCrvCounterInfo(CRS, crvBarIndex, sectorNumber, moduleNumber, layerNumber, barNumber);

      //sector properties
      std::map<int,sectorCoincidenceProperties>::const_iterator sIter = _sectorMap.find(sectorNumber);
      if(sIter==_sectorMap.end()) throw std::logic_error("CrvCoincidenceFinder: Found a CRV hit at a CRV sector without properties.");
      const sectorCoincidenceProperties &sector = sIter->second;

      //counter number within the entire sector type (like CRV-T, CRV-R, ...)
      int counterNumber = sector.precedingCounters + sector.nCountersPerModule*moduleNumber + barNumber;

      CLHEP::Hep3Vector crvCounterPos=CrvHelper::GetCrvCounterPos(CRS, crvBarIndex);
      double x=crvCounterPos[sector.widthDirection];
      double y=crvCounterPos[sector.thicknessDirection];

      //get the reco pulses information
      int SiPM = crvRecoPulse->GetSiPMNumber();

      //ignore SiPMs on counter sides which don't have SiPMs according to the geometry file
      int counterSide=SiPM%2;
      if(counterSide==0 && !sector.sipmsAtSide0) continue;
      if(counterSide==1 && !sector.sipmsAtSide1) continue;

      double time=crvRecoPulse->GetPulseTime();
      double timePulseStart=crvRecoPulse->GetPulseStart();
      double timePulseEnd=crvRecoPulse->GetPulseEnd();
      float  PEs =crvRecoPulse->GetPEs();
      if(_usePEsPulseHeight) PEs=crvRecoPulse->GetPEsPulseHeight();
      if(_useNoFitReco) PEs=crvRecoPulse->GetPEsNoFit();
      if(_useNoFitReco) time=crvRecoPulse->GetPulseTimeNoFit();

      //don't split counter sides for the purpose of finding clusters
      sectorTypeMap[sector.sectorType].emplace_back(crvRecoPulse, crvCounterPos,
                                                    x, y, time, timePulseStart, timePulseEnd, PEs,
                                                    layerNumber, counterNumber, SiPM, sector.PEthreshold,
                                                    sector.maxTimeDifferenceAdjacentPulses, sector.maxTimeDifference,
                                                    sector.maxSlope, sector.maxSlopeDifference, sector.coincidenceLayers);
    }

    //loop through all crv sectors types
    std::map<int, std::vector<CrvHit> >::const_iterator sectorTypeMapIter;
    for(sectorTypeMapIter=sectorTypeMap.begin(); sectorTypeMapIter!=sectorTypeMap.end(); ++sectorTypeMapIter)
    {
      int crvSectorType = sectorTypeMapIter->first;
      const std::vector<CrvHit> &hitsUnfiltered = sectorTypeMapIter->second; 

      //filter hits, i.e. remove all hits below PE threshold
      //and orders the remaining hits by their x position
      std::multiset<CrvHit> posOrderedHits; 
      filterHits(hitsUnfiltered, posOrderedHits);

      //distribute the set of hits into clusters
      std::vector<std::vector<CrvHit> > clusters;
      findClusters(posOrderedHits, clusters);

      //loop through all clusters
      for(size_t iCluster=0; iCluster<clusters.size(); ++iCluster)
      {
        const std::vector<CrvHit> &cluster=clusters[iCluster];
        std::vector<CrvHit> cluster0;  //cluster for SiPMs at negative end
        std::vector<CrvHit> cluster1;  //cluster for SiPMs at positive end
        for(auto hit=cluster.begin(); hit!=cluster.end(); ++hit)
        {
          if(hit->_SiPM%2==0) cluster0.push_back(*hit); else cluster1.push_back(*hit);
        }

        //check whether this hit cluster has coincidences
        //(separately for both readout sides)
        //and fill all hits belonging to a coincidence group into a new multiset.
        //this new multiset may have several entries of the same hit, 
        //if the hit belongs to multiple coincidence groups.
        //this gives more weight to such hits when calculating 
        //cluster average times/positions below.
        std::multiset<CrvHit> coincidenceHits;
        checkCoincidence(cluster0,coincidenceHits);
        checkCoincidence(cluster1,coincidenceHits);
        if(coincidenceHits.empty()) continue; //no coincidences found

        //using the set of coincidence hits instead of the original cluster 
        //removes hits that do not belong to any coincidence
        //distributing the remaining set of coincidence hits into sub clusters
        //reduces the lengths of the veto windows
        std::vector<std::vector<CrvHit> > subClusters;
        findClusters(coincidenceHits, subClusters);

        clusterProperties(crvSectorType, subClusters, crvCoincidenceClusterCollection);
      }//loop over all cluster in sector type
    }//loop over all sector types

    ++_totalEvents;
    if(crvCoincidenceClusterCollection->size()>0) ++_totalEventsCoincidence;

    if(_verboseLevel>1)
    {
      std::cout<<moduleDescription().moduleLabel()<<"   run "<<event.id().run()<<"  subrun "<<event.id().subRun()<<"  event "<<event.id().event();
      std::cout<<"   Coincidence clusters found: "<<crvCoincidenceClusterCollection->size()<<std::endl;
    }

    event.put(std::move(crvCoincidenceClusterCollection));
  } // end produce


  void CrvCoincidenceFinder::clusterProperties(int crvSectorType, const std::vector<std::vector<CrvHit> > &clusters, 
                                               std::unique_ptr<CrvCoincidenceClusterCollection> &crvCoincidenceClusterCollection)
  {
    //loop through all clusters
    for(size_t iCluster=0; iCluster<clusters.size(); ++iCluster)
    {
      const std::vector<CrvHit> &cluster=clusters[iCluster];

      std::vector<art::Ptr<CrvRecoPulse> > crvRecoPulses;
      double startTime=cluster.front()._time;
      double endTime=cluster.front()._time;
      if(_usePulseOverlaps)
      {
        startTime=cluster.front()._timePulseStart;
        endTime=cluster.front()._timePulseEnd;
      }
      double PEs=0;
      CLHEP::Hep3Vector avgCounterPos;  //PE-weighted average position
      int nHits=cluster.size();
      std::set<int> layerSet;
      double sumX =0;
      double sumY =0;
      double sumYY=0;
      double sumXY=0;
      for(auto hit=cluster.begin(); hit!=cluster.end(); ++hit)
      {
        crvRecoPulses.push_back(hit->_crvRecoPulse);
        PEs+=hit->_PEs;
        avgCounterPos+=hit->_pos*hit->_PEs;
        layerSet.insert(hit->_layer);
        sumX +=hit->_x;
        sumY +=hit->_y;
        sumYY+=hit->_y*hit->_y;
        sumXY+=hit->_x*hit->_y;
        if(_usePulseOverlaps)
        {
          if(startTime>hit->_timePulseStart) startTime=hit->_timePulseStart;
          if(endTime<hit->_timePulseEnd) endTime=hit->_timePulseEnd;
        }
        else
        {
          if(startTime>hit->_time) startTime=hit->_time;
          if(endTime<hit->_time) endTime=hit->_time;
        }
      }

      //average counter position (PE weighted), slope, layers
      avgCounterPos/=PEs;
      double slope=(nHits*sumXY-sumX*sumY)/(nHits*sumYY-sumY*sumY);
      std::vector<int> layers(layerSet.begin(), layerSet.end());

      //insert the cluster information into the vector of the crv coincidence clusters
      crvCoincidenceClusterCollection->emplace_back(crvSectorType, avgCounterPos, startTime, endTime, PEs, crvRecoPulses, slope, layers);
    } //loop through all clusters
  } //end cluster properies


  //remove hits below the threshold
  void CrvCoincidenceFinder::filterHits(const std::vector<CrvHit> &hits, std::multiset<CrvHit> &hitsFiltered)
  {
    std::vector<CrvHit>::const_iterator iterHit;
    for(iterHit=hits.begin(); iterHit!=hits.end(); ++iterHit)
    {
      int    layer=iterHit->_layer;
      int    counter=iterHit->_counter;  //counter number in one layer counted from the beginning of the counter type
      double time=iterHit->_time;
      double timePulseStart=iterHit->_timePulseStart;
      double timePulseEnd=iterHit->_timePulseEnd;

      int    PEthreshold=iterHit->_PEthreshold;
      double maxTimeDifferenceAdjacentPulses=iterHit->_maxTimeDifferenceAdjacentPulses;

      //check other SiPM and the SiPMs at the adjacent counters
      double PEs_thisCounter=0;
      double PEs_adjacentCounter1=0;
      double PEs_adjacentCounter2=0;
      std::vector<CrvHit>::const_iterator iterHitAdjacent;
      for(iterHitAdjacent=hits.begin(); iterHitAdjacent!=hits.end(); ++iterHitAdjacent)
      {
        //use hits of the same layer only
        if(iterHitAdjacent->_layer!=layer) continue;

        //use hits within a certain time window only
        if(!_usePulseOverlaps)
        {
          if(fabs(iterHitAdjacent->_time-time)>maxTimeDifferenceAdjacentPulses) continue;
        }
        else
        {
          double overlapTime=std::min(iterHitAdjacent->_timePulseEnd,timePulseEnd)-std::max(iterHitAdjacent->_timePulseStart,timePulseStart);
          if(overlapTime<_minOverlapTimeAdjacentPulses) continue; //no overlap or overlap time too short
        }

        //collect all PEs of this and the adjacent counters
        int counterDiff=iterHitAdjacent->_counter-counter;
        if(counterDiff==0) PEs_thisCounter+=iterHitAdjacent->_PEs;   //add PEs from the same counter (i.e. the "other" SiPM),
                                                                     //if the "other" hit is within a certain time window (5ns)
                                                                     //this will include PEs from the current pulse
        if(counterDiff==-1) PEs_adjacentCounter1+=iterHitAdjacent->_PEs;  //add PEs from an adjacent counter,
                                                                          //if these hits are within a certain time window (5ns)
        if(counterDiff==1) PEs_adjacentCounter2+=iterHitAdjacent->_PEs;   //add PEs from an adjacent counter,
                                                                          //if these hits are within a certain time window (5ns)
      }

      //if the number of PEs of this hit (plus the number of PEs of the same or one of the adjacent counter, if their time 
      //difference is small enough) is above the PE threshold, add this hit to vector of filtered hits
      if(PEs_thisCounter+PEs_adjacentCounter1>=PEthreshold || PEs_thisCounter+PEs_adjacentCounter2>=PEthreshold)
         hitsFiltered.insert(*iterHit);
    }
  } //end filter hits


  void CrvCoincidenceFinder::findClusters(std::multiset<CrvHit> &hits, std::vector<std::vector<CrvHit> > &clusters)
  {
    while(!hits.empty()) //run through clustering processes until all hits are distributed into clusters
    {
      clusters.resize(clusters.size()+1); //add a new cluster
      std::vector<CrvHit> &cluster = clusters.back();
      double lastX=0;
      for(auto hitsIter=hits.begin(); hitsIter!=hits.end(); )
      {
        if(cluster.empty())
        {
          //first element of current cluster
          //gets moved from set of hits to the current cluster
          lastX=hitsIter->_x;
          cluster.push_back(*hitsIter);
          hitsIter=hits.erase(hitsIter);
        }
        else
        {
          if(hitsIter->_x-lastX>_clusterMaxDistance) break; //cannot expect any other entries to be close enough to current cluster

          //check whether current hit satisfies time and distance condition w.r.t. to any hit of current cluster
          bool erasedHit=false;
          for(auto clusterIter=cluster.begin(); clusterIter!=cluster.end(); ++clusterIter)
          {
            if(_usePulseOverlaps)
            {
              if((std::fabs(hitsIter->_x-clusterIter->_x)<_clusterMaxDistance) &&
                 (hitsIter->_timePulseEnd-clusterIter->_timePulseStart>_clusterMinOverlapTime) && 
                 (clusterIter->_timePulseEnd-hitsIter->_timePulseStart>_clusterMinOverlapTime))
              {
                //this hit satisfied the conditions
                //move it from set of hits to the current cluster
                if(hitsIter->_x>lastX) lastX=hitsIter->_x;
                cluster.push_back(*hitsIter);
                hitsIter=hits.erase(hitsIter);
                erasedHit=true;
                break;  //no need for more comparisons with other hits in current cluster, go to the next hit in set of hits
              }
            }
            else
            {
              if((std::fabs(hitsIter->_x-clusterIter->_x)<_clusterMaxDistance) &&
                 (std::fabs(hitsIter->_time-clusterIter->_time)<_clusterMaxTimeDifference))
              {
                //this hit satisfied the conditions
                //move it from set of hits to the current cluster
                if(hitsIter->_x>lastX) lastX=hitsIter->_x;
                cluster.push_back(*hitsIter);
                hitsIter=hits.erase(hitsIter);
                erasedHit=true;
                break;  //no need for more comparisons with other hits in current cluster, go to the next hit in set of hits
              }
            }
          } //loop over all hits in the cluster (for comparison with current hit)

          if(!erasedHit) ++hitsIter;

        } //cluster not empty 
      } //loop over all undistributed hits
    } //loop until all hits in set of hits distributed into clusters
  } //end finder clusters


  void CrvCoincidenceFinder::checkCoincidence(const std::vector<CrvHit> &hits, std::multiset<CrvHit> &coincidenceHits)
  {
    std::vector<CrvHit> hitsLayers[nLayers];  //separated by layers 
    std::vector<CrvHit>::const_iterator iterHit;
    for(iterHit=hits.begin(); iterHit!=hits.end(); ++iterHit)
    {
      int    layer=iterHit->_layer;
      hitsLayers[layer].push_back(*iterHit);
    }

    //***************************************************
    //find coincidences using 2/4 coincidence requirement
    {
      std::vector<CrvHit>::const_iterator layerIterators[2];

      for(int layer1=0; layer1<4; ++layer1)
      for(int layer2=layer1+1; layer2<4; ++layer2)
      {
        const std::vector<CrvHit> &layer1Hits=hitsLayers[layer1];
        const std::vector<CrvHit> &layer2Hits=hitsLayers[layer2];
        std::vector<CrvHit>::const_iterator layer1Iter;
        std::vector<CrvHit>::const_iterator layer2Iter;

        //it will loop only, if both layers have hits
        //loops will exit, if all three hits require a 3/4 or 4/4 coincidence
        for(layer1Iter=layer1Hits.begin(); layer1Iter!=layer1Hits.end(); ++layer1Iter)
        for(layer2Iter=layer2Hits.begin(); layer2Iter!=layer2Hits.end(); ++layer2Iter)
        {
          if(layer1Iter->_coincidenceLayers>2 && layer2Iter->_coincidenceLayers>2) continue; //all hits require at least a 3/4 coincidence

          //add all both hits into one array
          layerIterators[0]=layer1Iter;
          layerIterators[1]=layer2Iter;
          if(checkCombination(layerIterators,2))
          {
            coincidenceHits.insert(*layer1Iter);
            coincidenceHits.insert(*layer2Iter);
          }
        }
      }
    }  //two layer coincidences
    if(!coincidenceHits.empty()) return;

    //***************************************************
    //find coincidences using 3/4 coincidence requirement
    {
      std::vector<CrvHit>::const_iterator layerIterators[3];

      for(int layer1=0; layer1<4; ++layer1)
      for(int layer2=layer1+1; layer2<4; ++layer2)
      for(int layer3=layer2+1; layer3<4; ++layer3)
      {
        const std::vector<CrvHit> &layer1Hits=hitsLayers[layer1];
        const std::vector<CrvHit> &layer2Hits=hitsLayers[layer2];
        const std::vector<CrvHit> &layer3Hits=hitsLayers[layer3];
        std::vector<CrvHit>::const_iterator layer1Iter;
        std::vector<CrvHit>::const_iterator layer2Iter;
        std::vector<CrvHit>::const_iterator layer3Iter;

        //it will loop only, if all 3 layers have hits
        //loops will exit, if all three hits require a 4/4 coincidence
        for(layer1Iter=layer1Hits.begin(); layer1Iter!=layer1Hits.end(); ++layer1Iter)
        for(layer2Iter=layer2Hits.begin(); layer2Iter!=layer2Hits.end(); ++layer2Iter)
        for(layer3Iter=layer3Hits.begin(); layer3Iter!=layer3Hits.end(); ++layer3Iter)
        {
          if(layer1Iter->_coincidenceLayers>3 && layer2Iter->_coincidenceLayers>3 && layer3Iter->_coincidenceLayers>3) continue; //all hits require at 4/4 coincidence

          //add all 3 hits into one array
          layerIterators[0]=layer1Iter;
          layerIterators[1]=layer2Iter;
          layerIterators[2]=layer3Iter;
          if(checkCombination(layerIterators,3))
          {
            coincidenceHits.insert(*layer1Iter);
            coincidenceHits.insert(*layer2Iter);
            coincidenceHits.insert(*layer3Iter);
          }
        }
      }
    }  //three layer coincidences
    if(!coincidenceHits.empty()) return;

    //***************************************************
    //find coincidences using 4/4 coincidence requirement
    {
      std::vector<CrvHit>::const_iterator layerIterators[4];

      const std::vector<CrvHit> &layer0Hits=hitsLayers[0];
      const std::vector<CrvHit> &layer1Hits=hitsLayers[1];
      const std::vector<CrvHit> &layer2Hits=hitsLayers[2];
      const std::vector<CrvHit> &layer3Hits=hitsLayers[3];
      std::vector<CrvHit>::const_iterator layer0Iter;
      std::vector<CrvHit>::const_iterator layer1Iter;
      std::vector<CrvHit>::const_iterator layer2Iter;
      std::vector<CrvHit>::const_iterator layer3Iter;

      //it will loop only, if all 4 layers have hits
      for(layer0Iter=layer0Hits.begin(); layer0Iter!=layer0Hits.end(); ++layer0Iter)
      for(layer1Iter=layer1Hits.begin(); layer1Iter!=layer1Hits.end(); ++layer1Iter)
      for(layer2Iter=layer2Hits.begin(); layer2Iter!=layer2Hits.end(); ++layer2Iter)
      for(layer3Iter=layer3Hits.begin(); layer3Iter!=layer3Hits.end(); ++layer3Iter)
      {
        //add all 4 hits into one array
        layerIterators[0]=layer0Iter;
        layerIterators[1]=layer1Iter;
        layerIterators[2]=layer2Iter;
        layerIterators[3]=layer3Iter;
        if(checkCombination(layerIterators,4))
        {
          coincidenceHits.insert(*layer0Iter);
          coincidenceHits.insert(*layer1Iter);
          coincidenceHits.insert(*layer2Iter);
          coincidenceHits.insert(*layer3Iter);
        }
      }
    } // four layer coincidences
  } //end check coincidence


  bool CrvCoincidenceFinder::checkCombination(const std::vector<CrvHit>::const_iterator layerIterators[], int n) 
  {
    double maxTimeDifferences[n];
    double times[n];
    double timesPulseStart[n], timesPulseEnd[n];
    double x[n], y[n];
    double maxSlopes[n], maxSlopeDifferences[n];
    for(int i=0; i<n; ++i)
    {
      const std::vector<CrvHit>::const_iterator &iter=layerIterators[i];
      maxTimeDifferences[i]=iter->_maxTimeDifference;
      times[i]=iter->_time;
      timesPulseStart[i]=iter->_timePulseStart;
      timesPulseEnd[i]=iter->_timePulseEnd;
      x[i]=iter->_x;
      y[i]=iter->_y;
      maxSlopes[i]=iter->_maxSlope;
      maxSlopeDifferences[i]=iter->_maxSlopeDifference;
    }

    if(!_usePulseOverlaps)
    {
      double maxTimeDifference=*std::max_element(maxTimeDifferences,maxTimeDifferences+n);
      double timeMin = *std::min_element(times,times+n);
      double timeMax = *std::max_element(times,times+n);
      if(timeMax-timeMin>maxTimeDifference) return false;  //hits don't fall within the time window
    }
    else
    {
      double timeMaxPulseStart = *std::max_element(timesPulseStart,timesPulseStart+n);
      double timeMinPulseEnd = *std::min_element(timesPulseEnd,timesPulseEnd+n);
      if(timeMinPulseEnd-timeMaxPulseStart<_minOverlapTime) return false;  //pulses don't overlap, or overlap time too short
    }

    double maxSlope=*std::max_element(maxSlopes,maxSlopes+n);
    double maxSlopeDifference=*std::max_element(maxSlopeDifferences,maxSlopeDifferences+n);
    std::vector<float> slopes;
    for(int d=0; d<n-1; ++d)
    {
      slopes.push_back((x[d+1]-x[d])/(y[d+1]-y[d]));   //width direction / thickness direction
      if(fabs(slopes.back())>maxSlope) return false;  //not more than maxSlope allowed for coincidence;
    }

    if(n>2)
    {
      if(fabs(slopes[0]-slopes[1])>maxSlopeDifference) return false;
      if(n>3)
      {
        if(fabs(slopes[0]-slopes[2])>maxSlopeDifference) return false;
        if(fabs(slopes[1]-slopes[2])>maxSlopeDifference) return false;
      }
    }

    return true; //all coincidence criteria for this combination satisfied
  } // end checkCombination

} // end namespace mu2e

using mu2e::CrvCoincidenceFinder;
DEFINE_ART_MODULE(CrvCoincidenceFinder)
