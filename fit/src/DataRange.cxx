// @(#)root/fit:$Id$
// Author: L. Moneta Wed Aug 30 11:05:02 2006

/**********************************************************************
 *                                                                    *
 * Copyright (c) 2006  LCG ROOT Math Team, CERN/PH-SFT                *
 *                                                                    *
 *                                                                    *
 **********************************************************************/

// Implementation file for class DataRange

#include "Fit/DataRange.h"

#include <algorithm>

namespace ROOT { 

   namespace Fit { 

DataRange::DataRange(double xmin, double xmax) : 
   fRanges( std::vector<RangeSet> (1) )
{
   // construct a range for [xmin, xmax] 
   if (xmin < xmax) { 
      RangeSet rx(1); 
      rx[0] = std::make_pair(xmin, xmax); 
      fRanges[0] = rx; 
   }
}


DataRange::DataRange(double xmin, double xmax, double ymin, double ymax) : 
   fRanges( std::vector<RangeSet> (2) )
{
   // construct a range for [xmin, xmax] , [ymin, ymax] 
   if (xmin < xmax) { 
      RangeSet rx(1); 
      rx[0] = std::make_pair(xmin, xmax); 
      fRanges[0] = rx; 
   }
   
   if (ymin < ymax) { 
      RangeSet ry(1); 
      ry[0] = std::make_pair(ymin, ymax); 
      fRanges[1] = ry; 
   }
}

DataRange::DataRange(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax) : 
   fRanges( std::vector<RangeSet> (3) )
{
   // construct a range for [xmin, xmax] , [ymin, ymax] , [zmin, zmax] 
   if (xmin < xmax) { 
      RangeSet rx(1); 
      rx[0] = std::make_pair(xmin, xmax); 
      fRanges[0] = rx; 
   }
   if (ymin < ymax) {    
      RangeSet ry(1); 
      ry[0] = std::make_pair(ymin, ymax); 
      fRanges[1] = ry; 
   }
   if (zmin < zmax) {    
      RangeSet rz(1); 
      rz[0] = std::make_pair(zmin, zmax); 
      fRanges[2] = rz; 
   }
}

void DataRange::AddRange(double xmin, double xmax, unsigned  int  icoord  ) { 
   // add a range [xmin,xmax] for the new coordinate icoord 

   if (xmin >= xmax) return;  // no op in case of bad values

   // case the  coordinate is larger than the current allocated vector size
   if (icoord >= fRanges.size() ) { 
      RangeSet rx(1); 
      rx[0] = std::make_pair(xmin, xmax); 
      fRanges.resize(icoord+1);
      fRanges[icoord] = rx; 
      return;
   } 
   RangeSet & rs = fRanges[icoord]; 
   // case the vector  of the ranges is empty in the given coordinate
   if ( rs.size() == 0) { 
      rs.push_back(std::make_pair(xmin,xmax) ); 
      return;
   } 
   // case of  an already existing range
   CleanRangeSet(icoord,xmin,xmax); 
   // add the new one
   rs.push_back(std::make_pair(xmin,xmax) ); 
   // sort range in increasing values 
   std::sort( rs.begin(), rs.end() );

}

bool DataRange::IsInside(double x, unsigned int icoord ) const { 
   // check if a point is in range

   if (Size(icoord) == 0) return true;  // no range existing (is like -inf, +inf)  
    const RangeSet & ranges = fRanges[icoord];
    for (RangeSet::const_iterator itr = ranges.begin(); itr != ranges.end(); ++itr) { 
       if ( x < (*itr).first ) return false; 
       if ( x <= (*itr).second) return true; 
    }
    return false; // point is larger than last xmax
} 

void DataRange::Clear(unsigned int icoord ) { 
   // remove all ranges for coordinate icoord
   if (Size(icoord) == 0) return;  // no op in this case 
   fRanges[icoord].clear(); 
}


void DataRange::CleanRangeSet(unsigned int icoord, double xmin, double xmax) { 
   //  remove all the existing ranges between xmin and xmax 
   //  called when a new range is inserted

   // loop on existing ranges 
   RangeSet & ranges = fRanges[icoord]; 
   for (RangeSet::iterator itr = ranges.begin(); itr != ranges.end(); ++itr) { 
      // delete included ranges
      if ( itr->first >= xmin && itr->second <= xmax) { 
         ranges.erase(itr);
         // itr goes to next element, so go back before adding
         --itr;
      }
   }
   
}



   } // end namespace Fit

} // end namespace ROOT

