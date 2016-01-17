#ifndef TrackerGeom_PanelId_hh
#define TrackerGeom_PanelId_hh

//
// Identifier for a panel.
//

//
// $Id: PanelId.hh,v 1.9 2013/03/08 04:31:45 brownd Exp $
// $Author: brownd $
// $Date: 2013/03/08 04:31:45 $
//
// Original author Rob Kutschke
//

#include <ostream>
#include "DataProducts/inc/PlaneId.hh"
#include <string>

namespace mu2e {

  struct PanelId;
  inline std::ostream& operator<<(std::ostream& ost,
                                  const PanelId& s );

  class PanelId{

  public:
// qualify how close 2 panels are by their Z separation.  This needs to be a logical
// separation, in case there are alignment constants applied
    enum isep{same=0,plane,station1,station2,station3,apart};

    PanelId():
      _planeId(-1),
      _panel(-1){
    }

    PanelId(std::string const&);

    PanelId( PlaneId plane,
              int panel
              ):
      _planeId(plane),
      _panel(panel){
    }

    // Use compiler-generated copy c'tor, copy assignment, and d'tor.

    const PlaneId& getPlaneId() const {
      return _planeId;
    }

    int getPlane() const {
      return _planeId;
    }

    int getPanel() const {
      return _panel;
    }

    int getStation() const{
      return _planeId/2;
    }

    bool operator==(PanelId const& rhs) const{
      return ( _planeId == rhs._planeId && _panel == rhs._panel );
    }

    bool operator!=(PanelId const& rhs) const{
      return !( *this == rhs);
    }

    bool operator < (PanelId const& rhs) const {
      return _planeId < rhs._planeId || (_planeId == rhs._planeId && _panel < rhs._panel);
    }

    isep separation(PanelId const& other) const;

    friend std::ostream& operator<<(std::ostream& ost,
                                    const PanelId& s ){
      ost << s._planeId << "_" << s._panel;
      return ost;
    }

  private:

    PlaneId _planeId;
    int     _panel;

  };

}  //namespace mu2e

#endif /* TrackerGeom_PanelId_hh */
