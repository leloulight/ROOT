// @(#)root/tmva $Id: DecisionTreeNode.h,v 1.11 2006/08/30 22:19:58 andreas.hoecker Exp $    
// Author: Andreas Hoecker, Joerg Stelzer, Helge Voss, Kai Voss 

/**********************************************************************************
 * Project: TMVA - a Root-integrated toolkit for multivariate data analysis       *
 * Package: TMVA                                                                  *
 * Class  : DecisionTreeNode                                                      *
 * Web    : http://tmva.sourceforge.net                                           *
 *                                                                                *
 * Description:                                                                   *
 *      Node for the Decision Tree                                                *
 *                                                                                *
 * Authors (alphabetical):                                                        *
 *      Andreas Hoecker <Andreas.Hocker@cern.ch> - CERN, Switzerland              *
 *      Xavier Prudent  <prudent@lapp.in2p3.fr>  - LAPP, France                   *
 *      Helge Voss      <Helge.Voss@cern.ch>     - MPI-KP Heidelberg, Germany     *
 *      Kai Voss        <Kai.Voss@cern.ch>       - U. of Victoria, Canada         *
 *                                                                                *
 * Copyright (c) 2005:                                                            *
 *      CERN, Switzerland,                                                        * 
 *      U. of Victoria, Canada,                                                   * 
 *      MPI-KP Heidelberg, Germany,                                               * 
 *      LAPP, Annecy, France                                                      *
 *                                                                                *
 * Redistribution and use in source and binary forms, with or without             *
 * modification, are permitted according to the terms listed in LICENSE           *
 * (http://tmva.sourceforge.net/LICENSE)                                          *
 **********************************************************************************/

#ifndef ROOT_TMVA_DecisionTreeNode
#define ROOT_TMVA_DecisionTreeNode

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DecisionTreeNode                                                     //
//                                                                      //
// Node for the Decision Tree                                           //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TROOT.h"
#ifndef ROOT_TMVA_NodeID
#include "TMVA/NodeID.h"
#endif

#ifndef ROOT_TMVA_Node
#include "TMVA/Node.h"
#endif

using std::string;

namespace TMVA {
   class Event;


   class DecisionTreeNode: public Node {

   public:

      // constructor of an essentially "empty" node floating in space
      DecisionTreeNode (Event* e =NULL);
      // constructor of a daughter node as a daughter of 'p'
      DecisionTreeNode (Node* p); 
      virtual ~DecisionTreeNode(){}

      // test event if it decends the tree at this node to the right  
      virtual Bool_t GoesRight( const Event & ) const;

      // test event if it decends the tree at this node to the left 
      virtual Bool_t GoesLeft ( const Event & ) const;

      // set the cut value applied at this node 
      void  SetCutValue ( Double_t c ) { fCutValue  = c; }
      // return the cut value applied at this node
      Double_t GetCutValue ( void ) const { return fCutValue;  }

      // set true: if event variable > cutValue ==> signal , false otherwise
      void  SetCutType( Bool_t t   ) { fCutType = t; }
      // return kTRUE: Cuts select signal, kFALSE: Cuts select bkg
      Bool_t    GetCutType( void ) const { return fCutType; }

      // set node type: 1 signal node, -1 bkg leave, 0 intermediate Node
      void  SetNodeType( Int_t t ) { fNodeType = t;} 
      // return node type: 1 signal node, -1 bkg leave, 0 intermediate Node 
      Int_t GetNodeType( void ) const { return fNodeType; }

      //set S/(S+B) at this node (from  training)
      void     SetSoverSB( Double_t ssb ){ fSoverSB =ssb ; }
      //return  S/(S+B) at this node (from  training)
      Double_t GetSoverSB( void ) const  { return fSoverSB; }

      // set the choosen index, measure of "purity" (separation between S and B) AT this node
      void     SetSeparationIndex( Double_t sep ){ fSeparationIndex =sep ; }
      // return the separation index AT this node
      Double_t GetSeparationIndex( void ) const  { return fSeparationIndex; }

      // set the separation, or information gained BY this nodes selection
      void     SetSeparationGain( Double_t sep ){ fSeparationGain =sep ; }
      // return the gain in separation obtained by this nodes selection
      Double_t GetSeparationGain( void ) const  { return fSeparationGain; }

      // set the number of events that entered the node (during training)
      void     SetNEvents( Double_t nev ){ fNEvents =nev ; }
      // return  the number of events that entered the node (during training)
      Double_t GetNEvents( void ) const  { return fNEvents; }

      //recursively print the node and its daughters (--> print the 'tree')
      virtual void        PrintRec( ostream&  os, const Int_t depth=0, const string pos="root" ) const;

      //recursively read the node and its daughters (--> read the 'tree')
      virtual NodeID ReadRec ( istream& is, NodeID nodeID, const Event&, Node* parent=NULL );
  
   private:
  
      Double_t fCutValue; // cut value appplied on this node to discriminate bkg against sig
      Bool_t   fCutType;  // true: if event variable > cutValue ==> signal , false otherwise
  
      Double_t fSoverSB;  // S/(S+B) at this node (from  training)
      Double_t fSeparationIndex; // measure of "purity" (separation between S and B) AT this node
      Double_t fSeparationGain;  // measure of "purity", separation, or information gained BY this nodes selection
      Double_t fNEvents;   // number of events in that entered the node (during training)
      Int_t    fNodeType;  // Type of node: -1 == Bkg-leaf, 1 == Signal-leaf, 0 = internal 
  
      ClassDef(DecisionTreeNode,0) //Node for the Decision Tree 
   };

} // namespace TMVA

#endif 
