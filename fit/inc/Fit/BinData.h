// @(#)root/fit:$Id$
// Author: L. Moneta Wed Aug 30 11:15:23 2006

/**********************************************************************
 *                                                                    *
 * Copyright (c) 2006  LCG ROOT Math Team, CERN/PH-SFT                *
 *                                                                    *
 *                                                                    *
 **********************************************************************/

// Header file for class BinData

#ifndef ROOT_Fit_BinData
#define ROOT_Fit_BinData

#ifndef ROOT_Fit_DataVector
#include "Fit/DataVector.h"
#endif


#ifdef USE_BINPOINT_CLASS

#ifndef ROOT_Fit_BinPoint
#include "Fit/BinPoint.h"
#endif

#endif


namespace ROOT { 

   namespace Fit { 



/** 
   BinData : class describing the binned data : 
              vectors of  x coordinates, y values and optionally error on y values and error on coordinates 
              The dimension of the coordinate is free
              There are three different options: 
              - only coordinates and values  (for binned likelihood fits) 
              - coordinate, values and error on  values (for normal least square fits) 
              - coordinate, values, error on values and coordinates (for effective least square fits)

              In addition there is the option to construct Bindata copying the data in (using the DataVector class) 
              or using pointer to external data (DataWrapper) class. 
              In general is found to be more efficient to copy the data. 
              In case of really large data sets for limiting memory consumption then the other option can be used
              Specialized constructor exists for data up to 3 dimensions. 

              When the data are copying in the number of points can be set later (or re-set) using Initialize and 
              the data are pushed in (one by one) using the Add method. 

             @ingroup  FitData  
*/ 


class BinData  : public FitData  { 

public : 

   enum ErrorType { kNoError, kValueError, kCoordError };

   static unsigned int GetPointSize(ErrorType err, unsigned int dim) { 
      if (err == kNoError) return dim + 1;   // no errors
      if (err == kValueError) return dim + 2;  // error only on the value
      return 2 * (dim + 1);   // error on value and coordinate
    }

      

   /**
      constructor from dimension of point  and max number of points (to pre-allocate vector)
      Give a zero value and then use Initialize later one if the size is not known
    */

   explicit BinData(unsigned int maxpoints = 0, unsigned int dim = 1, ErrorType err = kValueError) : 
//      DataVector( opt, GetPointSize(useErrorX,dim)*maxpoints ), 
      FitData(),
      fDim(dim),
      fPointSize(GetPointSize(err,dim) ),
      fNPoints(0),
      fDataVector(0),
      fDataWrapper(0)
   { 
      if (maxpoints > 0) fDataVector = new DataVector(fPointSize*maxpoints);
   } 

   /**
      constructor from option and default range
    */
   explicit BinData (const DataOptions & opt, unsigned int maxpoints = 0, unsigned int dim = 1, ErrorType err = kValueError) : 
      // DataVector( opt, (dim+2)*maxpoints ), 
      FitData(opt),
      fDim(dim),
      fPointSize(GetPointSize(err,dim) ),
      fNPoints(0),
      fDataVector(0),
      fDataWrapper(0)
   { 
      if (maxpoints > 0) fDataVector = new DataVector(fPointSize*maxpoints);
   } 

   /**
      constructor from options and range
    */
   BinData (const DataOptions & opt, const DataRange & range, unsigned int maxpoints = 0, unsigned int dim = 1, ErrorType err = kValueError ) : 
      //DataVector( opt, range, (dim+2)*maxpoints ), 
      FitData(opt,range),
      fDim(dim),
      fPointSize(GetPointSize(err,dim) ),
      fNPoints(0),
      fDataVector(0),
      fDataWrapper(0)
   { 
      if (maxpoints > 0) fDataVector = new DataVector(fPointSize*maxpoints);
   } 

   /** constructurs using external data */
   

   BinData(unsigned int n, const double * dataX, const double * val, const double * ex , const double * eval ) : 
      fDim(1), 
      fPointSize(0),
      fNPoints(n),
      fDataVector(0)
   { 
      fDataWrapper  = new DataWrapper(dataX, val, eval, ex);
   } 

   
   BinData(unsigned int n, const double * dataX, const double * dataY, const double * val, const double * ex , const double * ey, const double * eval  ) : 
      fDim(2), 
      fPointSize(0),
      fNPoints(n),
      fDataVector(0)
   { 
      fDataWrapper  = new DataWrapper(dataX, dataY, val, eval, ex, ey);
   } 

   BinData(unsigned int n, const double * dataX, const double * dataY, const double * dataZ, const double * val, const double * ex , const double * ey , const double * ez , const double * eval   ) : 
      fDim(3), 
      fPointSize(0),
      fNPoints(n),
      fDataVector(0)
   { 
      fDataWrapper  = new DataWrapper(dataX, dataY, dataZ, val, eval, ex, ey, ez);
   } 

private: 
   /// copy constructor (private) 
   BinData(const BinData &) : FitData() {}
   /// assignment operator  (private) 
   BinData & operator= (const BinData &) { return *this; } 
public:  


//    /**
//       Create from a compatible BinData set
//     */
   
//    BinData (const BinData & data , const DataOptions & opt, const DataRange & range) : 
//       DataVector(opt,range, data.DataSize() ), 
//       fDim(data.fDim),
//       fNPoints(data.fNPoints) 
//    {
// //       for (Iterator itr = begin; itr != end; ++itr) 
// //          if (itr->IsInRange(range) )
// //             Add(*itr); 
//    } 

   ~BinData() {
      if (fDataVector) delete fDataVector; 
      if (fDataWrapper) delete fDataWrapper; 
   }

   /**
      preallocate a data set given size and dimension
      need to be initialized with the with the right dimension before
    */
   void Initialize(unsigned int maxpoints, unsigned int dim = 1, ErrorType err = kValueError ) { 
      if (fDataWrapper) delete fDataWrapper;
      fDataWrapper = 0; 
      fDim = dim; 
      fPointSize = GetPointSize(err,dim);  
      if (fDataVector) { 
         // resize vector by adding the extra points on top of the previously existing ones 
         (fDataVector->Data()).resize( fDataVector->Size() + maxpoints * fPointSize );
      }
      else 
         fDataVector = new DataVector(fPointSize*maxpoints);
   }

//    /**
//       re-initialize adding some additional set of points keeping the previous ones
//     */
//    void ReInitialize(unsigned int nexpoints) { 
//       if (!fDataVector) return; 
//       (fDataVector->Data()).resize( fDataVector->Size() + nexpoints * fPointSize );      
//    }
      
   unsigned int PointSize() const { 
      return fPointSize; 
   }

   unsigned int DataSize() const { 
      if (fDataVector) return fDataVector->Size(); 
      return 0; 
   }

   bool UseCoordErrors() const { 
      if (fPointSize > fDim +2) return true; 
      return false;
   }


   /**
      add one dim data with only coordinate and values
   */
   void Add(double x, double y ) { 
      int index = fNPoints*PointSize();
      assert (fDataVector != 0);
      assert (PointSize() == 2 ); 
      assert (index + PointSize() <= DataSize() ); 

      double * itr = &((fDataVector->Data())[ index ]);
      *itr++ = x; 
      *itr++ = y; 

      fNPoints++;
   }
   
   /**
      add one dim data with no error in x
      in this case store the inverse of the error in y
   */
   void Add(double x, double y, double ey) { 
      int index = fNPoints*PointSize(); 
      //std::cout << "this = " << this << " index " << index << " fNPoints " << fNPoints << "  ds   " << DataSize() << std::endl; 
      assert( fDim == 1);
      assert (fDataVector != 0);
      assert (PointSize() == 3 ); 
      assert (index + PointSize() <= DataSize() ); 

      double * itr = &((fDataVector->Data())[ index ]);
      *itr++ = x; 
      *itr++ = y; 
      *itr++ =  (ey!= 0) ? 1.0/ey : 0; 

      fNPoints++;
   }
   /**
      add one dim data with  error in x
      in this case store the y error and not the inverse 
   */
   void Add(double x, double y, double ex, double ey) { 
      int index = fNPoints*PointSize(); 
      assert (fDataVector != 0);
      assert( fDim == 1);
      assert (PointSize() == 4 ); 
      assert (index + PointSize() <= DataSize() ); 

      double * itr = &((fDataVector->Data())[ index ]);
      *itr++ = x; 
      *itr++ = y; 
      *itr++ = ex; 
      *itr++ = ey; 

      fNPoints++;
   }


   /**
      add multi dim data with only value (no errors)
   */
   void Add(const double *x, double val) { 
      int index = fNPoints*PointSize(); 
      assert (fDataVector != 0);
      assert (PointSize() == fDim + 1 ); 

      if (index + PointSize() > DataSize()) 
         std::cout << "Error - index is " << index << " point size is " << PointSize()  << "  allocated size = " << DataSize() << std::endl;
      assert (index + PointSize() <= DataSize() ); 

      double * itr = &((fDataVector->Data())[ index ]);

      for (unsigned int i = 0; i < fDim; ++i) 
         *itr++ = x[i]; 
      *itr++ = val; 

      fNPoints++;
   }

   /**
      add multi dim data with only error in value 
   */
   void Add(const double *x, double val, double  eval) { 
      int index = fNPoints*PointSize(); 
      assert (fDataVector != 0);
      assert (PointSize() == fDim + 2 ); 

      if (index + PointSize() > DataSize()) 
         std::cout << "Error - index is " << index << " point size is " << PointSize()  << "  allocated size = " << DataSize() << std::endl;
      assert (index + PointSize() <= DataSize() ); 

      double * itr = &((fDataVector->Data())[ index ]);

      for (unsigned int i = 0; i < fDim; ++i) 
         *itr++ = x[i]; 
      *itr++ = val; 
      *itr++ =  (eval!= 0) ? 1.0/eval : 0; 

      fNPoints++;
   }


   /**
      add multi dim data with error in coordinates and value 
   */
   void Add(const double *x, double val, const double * ex, double  eval) { 
      int index = fNPoints*PointSize(); 
      assert (fDataVector != 0);
      assert (PointSize() == 2*fDim + 2 ); 

      if (index + PointSize() > DataSize()) 
         std::cout << "Error - index is " << index << " point size is " << PointSize()  << "  allocated size = " << DataSize() << std::endl;
      assert (index + PointSize() <= DataSize() ); 

      double * itr = &((fDataVector->Data())[ index ]);

      for (unsigned int i = 0; i < fDim; ++i) 
         *itr++ = x[i]; 
      *itr++ = val; 
      for (unsigned int i = 0; i < fDim; ++i) 
         *itr++ = ex[i]; 
      *itr++ = eval; 

      fNPoints++;
   }

   const double * Coords(unsigned int ipoint) const { 
      if (fDataVector) 
         return &((fDataVector->Data())[ ipoint*fPointSize ] );
      
      return fDataWrapper->Coords(ipoint);
   }

   double Value(unsigned int ipoint) const { 
      if (fDataVector)       
         return (fDataVector->Data())[ ipoint*fPointSize + fDim ];
     
      return fDataWrapper->Value(ipoint);
   }

//#ifdef LATER
   /**
      return error on the value
    */ 
   double Error(unsigned int ipoint) const { 
      if (fDataVector) { 
         // error on the value is the last element in the point structure
         double eval =  (fDataVector->Data())[ (ipoint+1)*fPointSize - 1];
         //if (fWithCoordError) return eval; 
         // when error in the coordinate is not stored, need to invert it 
         return eval != 0 ? 1.0/eval : 0; 
      }

      return fDataWrapper->Error(ipoint);
   } 

   /**
      return the inverse of error on the value 
      useful when error in the coordinates are not stored and then this is used directly this as the weight in 
      the least square function
    */
   double InvError(unsigned int ipoint) const {
      if (fDataVector) { 
         // error on the value is the last element in the point structure
         double eval =  (fDataVector->Data())[ (ipoint+1)*fPointSize - 1];
         return eval; 
//          if (!fWithCoordError) return eval; 
//          // when error in the coordinate is stored, need to invert it 
//          return eval != 0 ? 1.0/eval : 0; 
      }
      //case data wrapper 

      double eval = fDataWrapper->Error(ipoint);
      return eval != 0 ? 1.0/eval : 0; 
   }
//#endif

   /**
      return a pointer to the errors in the coordinates
    */
   const double * CoordErrors(unsigned int ipoint) const {
      if (fDataVector) { 
         // error on the value is the last element in the point structure
         return  &(fDataVector->Data())[ (ipoint)*fPointSize + fDim + 1];
      }

      return fDataWrapper->CoordErrors(ipoint);
   }


   const double * GetPoint(unsigned int ipoint, double & value) const {
      if (fDataVector) { 
         unsigned int j = ipoint*fPointSize;
         const std::vector<double> & v = (fDataVector->Data());
         const double * x = &v[j];
         value = v[j+fDim];
         return x;
      } 
      value = fDataWrapper->Value(ipoint);
      return fDataWrapper->Coords(ipoint);
   }


   const double * GetPoint(unsigned int ipoint, double & value, double & invError) const {
      if (fDataVector) { 
         unsigned int j = ipoint*fPointSize;
         const std::vector<double> & v = (fDataVector->Data());
         const double * x = &v[j];
         value = v[j+fDim];
         invError = v[j+fDim+1];
         return x;
      } 
      value = fDataWrapper->Value(ipoint);
      double e = fDataWrapper->Error(ipoint);
      invError = ( e != 0 ) ? 1.0/e : 0; 
      return fDataWrapper->Coords(ipoint);
   }

   const double * GetPointError(unsigned int ipoint, double & errvalue) const {
// to be called only when coord errors are stored
      if (fDataVector) { 
         assert(fPointSize > fDim + 2); 
         unsigned int j = ipoint*fPointSize;
         const std::vector<double> & v = (fDataVector->Data());
         const double * ex = &v[j+fDim+1];
         errvalue = v[j + 2*fDim +1];
         return ex;
      } 
      errvalue = fDataWrapper->Error(ipoint);
      return fDataWrapper->CoordErrors(ipoint);
   }


#ifdef USE_BINPOINT_CLASS
   const BinPoint & GetPoint(unsigned int ipoint) const { 
      if (fDataVector) { 
         unsigned int j = ipoint*fPointSize;
         const std::vector<double> & v = (fDataVector->Data());
         const double * x = &v[j];
         double value = v[j+fDim];
         if (fPointSize > fDim + 2) {
            const double * ex = &v[j+fDim+1];
            double err = v[j + 2*fDim +1];
            fPoint.Set(x,value,ex,err);
         } 
         else {
            double invError = v[j+fDim+1];
            fPoint.Set(x,value,invError);
         }

      } 
      else { 
         double value = fDataWrapper->Value(ipoint);
         double e = fDataWrapper->Error(ipoint);
         if (fPointSize > fDim + 2) {
            fPoint.Set(fDataWrapper->Coords(ipoint), value, fDataWrapper->CoordErrors(ipoint), e);
         } else { 
            double invError = ( e != 0 ) ? 1.0/e : 0; 
            fPoint.Set(fDataWrapper->Coords(ipoint), value, invError);
         }
      }
      return fPoint; 
   }      


   const BinPoint & GetPointError(unsigned int ipoint) const { 
      if (fDataVector) { 
         unsigned int j = ipoint*fPointSize;
         const std::vector<double> & v = (fDataVector->Data());
         const double * x = &v[j];
         double value = v[j+fDim];
         double invError = v[j+fDim+1];
         fPoint.Set(x,value,invError);
      } 
      else { 
         double value = fDataWrapper->Value(ipoint);
         double e = fDataWrapper->Error(ipoint);
         double invError = ( e != 0 ) ? 1.0/e : 0; 
         fPoint.Set(fDataWrapper->Coords(ipoint), value, invError);
      }
      return fPoint; 
   }      
#endif

   /**
      resize the vector to the given npoints 
    */
   void Resize (unsigned int npoints) { 
      fNPoints = npoints; 
      (fDataVector->Data()).resize(PointSize() *npoints);
   }


   unsigned int NPoints() const { return fNPoints; } 

   /**
      return number of contained points 
      in case of integral option size is npoints -1 
    */ 
   unsigned int Size() const { 
      return (Opt().fIntegral) ? fNPoints-1 : fNPoints; 
   }

   unsigned int NDim() const { return fDim; } 

protected: 

   void SetNPoints(unsigned int n) { fNPoints = n; }

private: 


   unsigned int fDim;       // coordinate dimension
   unsigned int fPointSize; // total point size including value and errors (= fDim + 2 for error in only Y ) 
   unsigned int fNPoints;   // number of contained points in the data set (can be different than size of vector)

   DataVector * fDataVector; 
   DataWrapper * fDataWrapper;

#ifdef USE_BINPOINT_CLASS
   mutable BinPoint fPoint; 
#endif

}; 

  
   } // end namespace Fit

} // end namespace ROOT



#endif /* ROOT_Fit_BinData */


