// @(#)root/reflex:$Name: merge_reflex $:$Id$
// Author: Axel Naumann, 2008

// Copyright CERN, CH-1211 Geneva 23, 2004-2008, All rights reserved.
//
// Permission to use, copy, modify, and distribute this software for any
// purpose is hereby granted without fee, provided that this copyright and
// permissions notice appear in all copies and derivatives.
//
// This software is provided "as is" without express or implied warranty.

#ifndef Reflex_ContainerImplBase
#define Reflex_ContainerImplBase

#include <vector>
#include <string>

#include "Reflex/Kernel.h"

#include "RWLock.h"
#include "ContainerBucket.h"

// re-hash level
#ifndef REFLEX_CONTAINER_REHASH_LEVEL
#define REFLEX_CONTAINER_REHASH_LEVEL 3
#endif

namespace Reflex {
   //-------------------------------------------------------------------------------

   // Hash type used by Reflex::Container
   typedef unsigned long Hash_t;

   //-------------------------------------------------------------------------------
   namespace Internal {

      // Simple and fast hashing routine for std::string
      inline Hash_t StringHash(const char* str) {
         Hash_t hash = 5381;
         while (*str) {
            hash *= 5;
            hash += *(str++);
         }
         return hash;
      }

      // Simple and fast hashing routine for std::string
      inline Hash_t StringHash(const std::string& s) {
         return StringHash(s.c_str());
      }

      class ContainerImplBase_iterator;
      namespace ContainerTools {
         class Link;
         class NodeArena;
         class INodeHelper;
      }

      //-------------------------------------------------------------------------------
      //
      // Base class for Reflex::Container.
      // Contains the non-templated part of Reflex::Container (against code bloat).
      // ContainerImplBase implements a hash map. It containes elements deriving from
      // class ContainerImplBase::Link (e.g. Container::Node<T>).
      //
      // It provides memory management optimized for a large number of small elements,
      // fast retrieval, and non-fragmentation of memory. Thread safety will be
      // implemented soon.

      class ContainerImplBase: public IContainerImpl {
      //-------------------------------------------------------------------------------
      //-------------------------------------------------------------------------------
      protected:
         typedef ContainerTools::Link Link;
         typedef ContainerTools::NodeArena NodeArena;
         typedef ContainerTools::BucketVector BucketVector;
         typedef ContainerTools::INodeHelper INodeHelper;

         // Initialize a ContainerImplBase object giving its node size and an initialial 
         // number of buckets. The number of buckets should be a prime for performance
         // reasons; check fgPrimeArraySqrt3 for precalculated, possibly optimal values.
         ContainerImplBase(size_t nodeSize, size_t size = 17);

         // Destruct a ContainerImplBase
         virtual ~ContainerImplBase();

         void InsertNode(Link* node, Hash_t hash);

         void PauseRehash(bool pause = true) {
            // Prevent the container from rehashing.
            // PauseRehash() should be called when inserting a large number of elements;
            // afterwards, rehashing should be turned on again by calling PausRehash(false).
            REFLEX_RWLOCK_R(fLock);
            if (fRehashPaused == pause)
                  return;
            REFLEX_RWLOCK_R_RELEASE(fLock);
            {
               REFLEX_RWLOCK_W(fLock);
               fRehashPaused = pause;
            }
            if (!pause && NeedRehash())
               Rehash();
         }

         // Virtual method, giving access to a node's hash.
         virtual Hash_t GetHash(const Link* node) const = 0;
         // Whether the node can be deleted
         virtual bool IsNodeInvalidated(const Link* link) const = 0;

         static size_t GetBucketSize(size_t requested);

      public:
         typedef ContainerImplBase_iterator iterator;

         iterator Begin(const INodeHelper& helper, const ContainerImplBase_iterator& nextContainer) const;
         iterator End() const;

         iterator RBegin(const INodeHelper& helper, const ContainerImplBase_iterator& prevContainer) const;
         iterator REnd() const;

         virtual const IConstIteratorImpl& ProxyBegin() const;
         virtual const IConstIteratorImpl& ProxyEnd() const;

         virtual const IConstReverseIteratorImpl& ProxyRBegin() const;
         virtual const IConstReverseIteratorImpl& ProxyREnd() const;

         // Reset the elements
         void Clear();

         // Set the next container (for extending containers)
         void SetNext(ContainerImplBase* next) const { fNext = next; }
         // Get the next container (for extending containers)
         ContainerImplBase* Next() const { return fNext; }

         // Arena used for the collection's nodes
         NodeArena* Arena() const { return fNodeArena; }

         // Vector of buckets
         const BucketVector& Buckets() const { return fBuckets; }
         BucketVector& Buckets() { return fBuckets; }

         virtual size_t Size() const {
            // Number of elements stored in the container
            REFLEX_RWLOCK_R(fLock);
            return fSize;
         }

         virtual bool Empty() const {
            // return whether the container is empty
            return !Size();
         }

         // Statistics holder for the collection
         struct Statistics {
            size_t fSize; // number of nodes stored in the collection
            size_t fCollisions; // number of nodes sharing the same bucket
            int    fMaxCollisionPerBucket; // maximum number of collisions seen in any of the collection's buckets
            double fCollisionPerBucketRMS; // root mean squared of the distribution of collisions of the buckets
            int    fNumBuckets; // the container's number of buckets
            int    fMaxNumRehashes; // upper limit of the (expensive) rehash operations the collection could have observed
            std::vector<const Link*> fCollidingNodes; // vector of nodes that have to share their bucket with other nodes
         };

         // Fill a Statistics object from the collection
         void GetStatistics(Statistics& stat) const;

      protected:
         // Empty the buckets, removing all nodes
         virtual void RemoveAllNodes() = 0;

      private:
         bool NeedRehash() const {
            // Determine whether the container needs to be rehashed, i.e.
            // whether the number of colliding nodes exceeds the number of nodes 
            // by a factor REFLEX_CONTAINER_REHASH_LEVEL (3 by default)
            REFLEX_RWLOCK_R(fLock);
            return fCollisions > fSize * REFLEX_CONTAINER_REHASH_LEVEL;
         }

         // Rehash the nodes
         void Rehash();

      protected:
         BucketVector  fBuckets; // collection of buckets
         bool          fRehashPaused; // whether insertions can cause a rehash to reduce the number of collisions
         size_t        fCollisions; // number of nodes sharing a bucket with other nodes
         size_t        fSize; // number of elements this container holds
         NodeArena*    fNodeArena; // the container's node storage manager
         mutable ContainerImplBase* fNext; // next container (used for chained searches)
         mutable RWLock fLock; // Read/Write lock for this container
         static const int fgPrimeArraySqrt3[19]; // a pre-computed array of prime numbers used for growing the buckets

         friend class ContainerImplBase_iterator;
      }; // class ContainerImplBase


      //-------------------------------------------------------------------------------
      class ContainerImplBase_iterator {
      public:
         typedef ContainerTools::Link Link;
         typedef ContainerTools::LinkIter LinkIter;
         typedef ContainerTools::LinkIter BucketIter;
         typedef ContainerTools::INodeHelper INodeHelper;

         ContainerImplBase_iterator(): fNextContainerBegin(0) {}

         ContainerImplBase_iterator(const LinkIter& linkiter, const BucketIter& bucketiter,
            const ContainerImplBase_iterator& nextContainer);

         ContainerImplBase_iterator(const ContainerImplBase& container, const INodeHelper& helper,
            const ContainerImplBase_iterator& nextContainer);

         ~ContainerImplBase_iterator();

         operator bool() const { return fLinkIter; }

         ContainerImplBase_iterator& operator++();

         ContainerImplBase_iterator operator++(int) {
            ContainerImplBase_iterator ret = *this;
            ++(*this);
            return ret;
         }

         bool operator == (const ContainerImplBase_iterator& rhs) const {
            return (rhs.fLinkIter == fLinkIter);
         }
         bool operator != (const ContainerImplBase_iterator& rhs) const {
            return (rhs.fLinkIter != fLinkIter);
         }

         const LinkIter& CurrentLink() const { return fLinkIter; }
         const BucketIter& CurrentBucket() const { return fBucketIter; }
         ContainerImplBase_iterator NextContainerBegin() const {
            if (fNextContainerBegin)
               return *fNextContainerBegin;
            return ContainerImplBase_iterator();
         }

      protected:
         LinkIter fLinkIter;
         BucketIter fBucketIter;
         ContainerImplBase_iterator* fNextContainerBegin; // Begin() of the next container
      }; // class ContainerImplBase_iterator

   } // namespace Internal

} // namespace Reflex


//-------------------------------------------------------------------------------
inline
Reflex::Internal::ContainerImplBase::iterator
Reflex::Internal::ContainerImplBase::Begin(const INodeHelper& helper,
                                       const ContainerImplBase_iterator& nextContainer) const {
//-------------------------------------------------------------------------------
   return iterator(*this, helper, nextContainer);
}


//-------------------------------------------------------------------------------
inline
Reflex::Internal::ContainerImplBase::iterator
Reflex::Internal::ContainerImplBase::End() const {
//-------------------------------------------------------------------------------
 return iterator();
}

#endif
