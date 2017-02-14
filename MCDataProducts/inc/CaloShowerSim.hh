#ifndef MCDataProducts_CaloShowerSim_hh
#define MCDataProducts_CaloShowerSim_hh

//
// Compress information of the shower generated in the crystal / readout in the calorimeter
// A CaloShowerSimStep records the position, time and energy of the fraction of an electromagnetic shower 
// generated by a SimParticle in a small longitudinal slice of a crystal (simple, right). 
//
// The Particle Id refers to the type of particle leaving the energy deposit, not the Particle originating the shower
//
// Original author Bertrand Echenard
//

#include <iostream>
#include "MCDataProducts/inc/CaloShowerStep.hh"
#include "MCDataProducts/inc/SimParticle.hh"


namespace mu2e {


   class CaloShowerSim {


       public:
          
          typedef std::vector<art::Ptr<CaloShowerStep> > CaloStepPtrs;
          typedef art::Ptr<SimParticle>                  SimPtr;
                    
          CaloShowerSim(): crystalId_(-1),sim_(),steps_(),time_(0),energy_(0),energyMC_(0),pIn_(0) {}
          
          CaloShowerSim(int crystalId, const SimPtr& sim, const CaloStepPtrs& steps, double time, double energy, double energyMC, double pIn): 
            crystalId_(crystalId),
            sim_(sim),
            steps_(steps),
            time_(time),
            energy_(energy),
            energyMC_(energyMC),
            pIn_(pIn) 
          {}


          int                 crystalId()       const {return crystalId_;}
          const SimPtr&       sim()             const {return sim_;}
          const CaloStepPtrs& caloShowerSteps() const {return steps_;}
          double              time()            const {return time_;}
          double              energy()          const {return energy_;}
          double              energyMC()        const {return energyMC_;}
          double              momentumIn()      const {return pIn_;}



       private:
            
            int           crystalId_;      
            SimPtr        sim_;          
            CaloStepPtrs  steps_;          
            double        time_;          
            double        energy_;
            double        energyMC_;
            double        pIn_;
   };

} 

#endif