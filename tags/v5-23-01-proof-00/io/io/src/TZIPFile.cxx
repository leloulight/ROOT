// @(#)root/io:$Id$
// Author: Fons Rademakers and Lassi Tuura  30/6/04

/*************************************************************************
 * Copyright (C) 1995-2004, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TZIPFile                                                             //
//                                                                      //
// This class describes a ZIP archive file containing multiple          //
// sub-files. Typically the sub-files are ROOT files. Notice that       //
// the ROOT files should not be compressed when being added to the      //
// ZIP file, since ROOT files are normally already compressed.          //
// Such a ZIP file should be created like:                              //
//                                                                      //
//    zip -n root multi file1.root file2.root                           //
//                                                                      //
// which creates a ZIP file multi.zip.                                  //
//                                                                      //
// A ZIP archive consists of files compressed with the popular ZLIB     //
// compression algorithm. The archive format is used among others by    //
// PKZip and Info-ZIP. The compression algorithm is also used by        //
// GZIP and the PNG graphics standard. The format of the archives is    //
// explained briefly below. This class provides an interface to read    //
// such archives.                                                       //
//                                                                      //
// A ZIP archive contains a prefix, series of archive members           //
// (sub-files), and a central directory. In theory the archive could    //
// span multiple disks (or files) with the central directory of the     //
// whole archive on the last disk, but this class does not support      //
// such multi-part archives. The prefix is only used in self-extracting //
// executable archive files.                                            //
//                                                                      //
// The members are stored in the archive sequentially, each with a      //
// local header followed by the (optionally) compressed data; the local //
// header describes the member, including its file name and compressed  //
// and real sizes. The central directory includes the member details    //
// again, plus allows an extra member comment to be added. The last     //
// member in the central directory is an end marker that can contain    //
// a comment for the whole archive. Both the local header and the       //
// central directory can also carry extra member-specific data; the     //
// data in the local and global parts can be different.                 //
// The fact that the archive has a global directory makes it efficient  //
// and allows for only the reading of the desired data, one does not    //
// have to scan through the whole file to find the desired sub-file.    //
//                                                                      //
// Once the archive has been opened, the client can query the members   //
// and read their contents by asking the archive for an offset where    //
// the sub-file starts. The members can be accessed in any order.       //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TZIPFile.h"
#include "TFile.h"
#include "TObjArray.h"


ClassImp(TZIPFile)

//______________________________________________________________________________
TZIPFile::TZIPFile() : TArchiveFile()
{
   // Default ctor.

   fDirPos     = 0;
   fDirSize    = 0;
   fDirOffset  = 0;
}

//______________________________________________________________________________
TZIPFile::TZIPFile(const char *archive, const char *member, TFile *file)
   : TArchiveFile(archive, member, file)
{
   // Specify the archive name and member name. The member can be a decimal
   // number which allows to access the n-th member.

   fDirPos     = 0;
   fDirSize    = 0;
   fDirOffset  = 0;
}

//______________________________________________________________________________
Int_t TZIPFile::OpenArchive()
{
   // Open archive and read end-header and directory. Returns -1 in case
   // of error, 0 otherwise.

   if (ReadEndHeader(FindEndHeader()) == -1)
      return -1;
   return ReadDirectory();
}

//______________________________________________________________________________
Long64_t TZIPFile::FindEndHeader()
{
   // Find the end header of the ZIP archive. Returns 0 in case of error.

   const Int_t kBUFSIZE = 1024;
   Long64_t    size = fFile->GetSize();
   Long64_t    limit = TMath::Min(size, Long64_t(kMAX_VAR_LEN));
   char        buf[kBUFSIZE+4];

   // Note, this works correctly even if the signature straddles read
   // boundaries since we always read an overlapped area of four
   // bytes on the next read
   for (Long64_t offset = 4; offset < limit; ) {
      offset = TMath::Min(offset + kBUFSIZE, limit);

      Long64_t pos = size - offset;
      Int_t    n = TMath::Min(kBUFSIZE+4, Int_t(offset));

      fFile->Seek(pos);
      if (fFile->ReadBuffer(buf, n)) {
         Error("FindEndHeader", "error reading %d bytes at %ldd", n, pos);
         return 0;
      }

      for (Int_t i = n - 4; i > 0; i--)
         if (buf[i]   == 0x50 && buf[i+1] == 0x4b &&
             buf[i+2] == 0x05 && buf[i+3] == 0x06) {
            return pos + i;
         }
   }

   Error("FindEndHeader", "did not find end header in %s", fArchiveName.Data());

   return 0;
}

//______________________________________________________________________________
Int_t TZIPFile::ReadEndHeader(Long64_t pos)
{
   // Read the end header of the ZIP archive including the archive comment
   // at the current file position. Check that it really was a single-disk
   // archive with all the entries as expected. Most importantly, figure
   // out where the central directory begins. Returns -1 in case of error,
   // 0 otherwise.

   char buf[kEND_HEADER_SIZE];

   // read and validate first the end header magic
   fFile->Seek(pos);
   if (fFile->ReadBuffer(buf, kZIP_MAGIC_LEN) ||
       Get(buf, kZIP_MAGIC_LEN) != kEND_HEADER_MAGIC) {
      Error("ReadEndHeader", "wrong end header magic in %s", fArchiveName.Data());
      return -1;
   }

   // read rest of the header
   if (fFile->ReadBuffer(buf + kZIP_MAGIC_LEN,  kEND_HEADER_SIZE - kZIP_MAGIC_LEN)) {
      Error("ReadEndHeader", "error reading %d end header bytes from %s",
            kEND_HEADER_SIZE - kZIP_MAGIC_LEN, fArchiveName.Data());
      return -1;
   }

   UInt_t   disk    = Get(buf + kEND_DISK_OFF,       kEND_DISK_LEN);
   UInt_t   dirdisk = Get(buf + kEND_DIR_DISK_OFF,   kEND_DIR_DISK_LEN);
   UInt_t   dhdrs   = Get(buf + kEND_DISK_HDRS_OFF,  kEND_DISK_HDRS_LEN);
   UInt_t   thdrs   = Get(buf + kEND_TOTAL_HDRS_OFF, kEND_TOTAL_HDRS_LEN);
   Long64_t diroff  = Get(buf + kEND_DIR_OFFSET_OFF, kEND_DIR_OFFSET_LEN);
   Long64_t dirsz   = Get(buf + kEND_DIR_SIZE_OFF,   kEND_DIR_SIZE_LEN);
   Int_t    commlen = Get(buf + kEND_COMMENTLEN_OFF, kEND_COMMENTLEN_LEN);

   if (disk != 0 || dirdisk != 0 || dhdrs != thdrs || diroff + dirsz != pos) {
      Error("ReadEndHeader", "inconsistency in end header data in %s",
            fArchiveName.Data());
      return -1;
   }

   char *comment = new char[commlen+1];
   if (fFile->ReadBuffer(comment, commlen)) {
      Error("ReadEndHeader", "error reading %d end header comment bytes from %s",
            commlen, fArchiveName.Data());
      delete [] comment;
      return -1;
   }
   comment[commlen] = '\0';

   fComment   = comment;
   fDirOffset = fDirPos = diroff;
   fDirSize   = dirsz;

   delete [] comment;

   return 0;
}

//______________________________________________________________________________
Int_t TZIPFile::ReadDirectory()
{
   // Read the directory of the ZIP archive. Returns -1 in case of error,
   // 0 otherwise.

   char   buf[kDIR_HEADER_SIZE];
   UInt_t n, i;

   // read and validate first the header magic
   fFile->Seek(fDirPos);
   if (fFile->ReadBuffer(buf, kZIP_MAGIC_LEN) ||
       (n = Get(buf, kZIP_MAGIC_LEN)) != kDIR_HEADER_MAGIC) {
      Error("ReadDirectory", "wrong directory header magic in %s",
            fArchiveName.Data());
      return -1;
   }

   // now read the full directory
   for (i = 0; n == kDIR_HEADER_MAGIC; i++) {
      // read the rest of the header
      if (fFile->ReadBuffer(buf + kZIP_MAGIC_LEN, kDIR_HEADER_SIZE - kZIP_MAGIC_LEN)) {
         Error("ReadDirectory", "error reading %d directory bytes from %s",
               kDIR_HEADER_SIZE - kZIP_MAGIC_LEN, fArchiveName.Data());
         return -1;
      }

      UInt_t   version = Get(buf + kDIR_VREQD_OFF,      kDIR_VREQD_LEN);
      UInt_t   flags   = Get(buf + kDIR_FLAG_OFF,       kDIR_FLAG_LEN);
      UInt_t   method  = Get(buf + kDIR_METHOD_OFF,     kDIR_METHOD_LEN);
      UInt_t   time    = Get(buf + kDIR_DATE_OFF,       kDIR_DATE_LEN);
      UInt_t   crc32   = Get(buf + kDIR_CRC32_OFF,      kDIR_CRC32_LEN);
      Long64_t csize   = Get(buf + kDIR_CSIZE_OFF,      kDIR_CSIZE_LEN);
      Long64_t usize   = Get(buf + kDIR_USIZE_OFF,      kDIR_USIZE_LEN);
      Int_t    namelen = Get(buf + kDIR_NAMELEN_OFF,    kDIR_NAMELEN_LEN);
      Int_t    extlen  = Get(buf + kDIR_EXTRALEN_OFF,   kDIR_EXTRALEN_LEN);
      Int_t    commlen = Get(buf + kDIR_COMMENTLEN_OFF, kDIR_COMMENTLEN_LEN);
      UInt_t   disk    = Get(buf + kDIR_DISK_START_OFF, kDIR_DISK_START_LEN);
      UInt_t   iattr   = Get(buf + kDIR_INT_ATTR_OFF,   kDIR_INT_ATTR_LEN);
      UInt_t   xattr   = Get(buf + kDIR_EXT_ATTR_OFF,   kDIR_EXT_ATTR_LEN);
      Long64_t offset  = Get(buf + kDIR_ENTRY_POS_OFF,  kDIR_ENTRY_POS_LEN);

      // check value sanity and the variable-length fields
      if (Get(buf + kDIR_MAGIC_OFF, kZIP_MAGIC_LEN) != kDIR_HEADER_MAGIC ||
          version > kARCHIVE_VERSION ||
          flags & 8 ||
          (method != kSTORED && method != kDEFLATED) ||
          disk != 0 ||
          csize < 0 ||
          usize < 0 ||
          csize > kMaxInt ||
          usize > kMaxInt) {
         Error("ReadDirectory", "inconsistency in directory data in %s",
               fArchiveName.Data());
         return -1;
      }

      char *name    = new char[namelen+1];
      char *extra   = new char[extlen+1];
      char *comment = new char[commlen+1];
      if (fFile->ReadBuffer(name, namelen) ||
          fFile->ReadBuffer(extra, extlen) ||
          fFile->ReadBuffer(comment, commlen)) {
         Error("ReadDirectory", "error reading additional directory data from %s",
               fArchiveName.Data());
         delete [] name;
         delete [] extra;
         delete [] comment;
         return -1;
      }
      name[namelen]    = '\0';
      extra[extlen]    = '\0';
      comment[commlen] = '\0';

      // create a new archive member and store the fields
      TZIPMember *m = new TZIPMember(name);
      fMembers->Add(m);

      m->fMethod = method;
      m->fLevel  = method == kSTORED ? 0
                                     : (flags & 6)/2 == 0 ? 3  // default (:N)
                                     : (flags & 6)/2 == 1 ? 9  // best (:X)
                                     : (flags & 6)/2 == 2 ? 2  // fast (:F)
                                     : (flags & 6)/2 == 3 ? 1  // fastest (:F)
                                     : 3;                      // unreached
      m->fCsize     = csize;
      m->fDsize     = usize;
      m->fCRC32     = crc32;
      m->fModTime.Set(time, kTRUE);   // DOS date/time format
      m->fGlobalLen = extlen;
      m->fGlobal    = extra;
      m->fComment   = comment;
      m->fAttrInt   = iattr;
      m->fAttrExt   = xattr;
      m->fPosition  = offset;

      delete [] name;
      delete [] comment;
      // extra is adopted be the TZIPMember

      if (gDebug)
         Info("ReadDirectory", "%lld  %lld  %s  %s",
              m->GetDecompressedSize(), m->GetCompressedSize(),
              m->GetModTime().AsSQLString(), m->GetName());

      // done, read the next magic
      if (fFile->ReadBuffer(buf, kZIP_MAGIC_LEN)) {
         Error("ReadDirectory", "error reading %d directory bytes from %s",
               kZIP_MAGIC_LEN, fArchiveName.Data());
         return -1;
      }
      n = Get(buf, kZIP_MAGIC_LEN);
   }

   // should now see end of archive
   if (n != kEND_HEADER_MAGIC) {
      Error("ReadDirectory", "wrong end header magic in %s", fArchiveName.Data());
      return -1;
   }

   return 0;
}

//______________________________________________________________________________
Int_t TZIPFile::ReadMemberHeader(TZIPMember *member)
{
   // Read the member header of the ZIP archive. Sets the position where
   // the data starts in the member object. Returns -1 in case of error,
   // 0 otherwise.

   // read file header to find start of data, since extra len might be
   // different we cannot take it from the directory data
   char buf[kENTRY_HEADER_SIZE];

   // read and validate first the entry header magic
   fFile->Seek(member->fPosition);
   if (fFile->ReadBuffer(buf, kZIP_MAGIC_LEN) ||
       Get(buf, kZIP_MAGIC_LEN) != kENTRY_HEADER_MAGIC) {
      Error("ReadMemberHeader", "wrong entry header magic in %s",
            fArchiveName.Data());
      return -1;
   }

   // read rest of the header
   if (fFile->ReadBuffer(buf + kZIP_MAGIC_LEN,  kENTRY_HEADER_SIZE - kZIP_MAGIC_LEN)) {
      Error("ReadMemberHeader", "error reading %d member header bytes from %s",
            kENTRY_HEADER_SIZE - kZIP_MAGIC_LEN, fArchiveName.Data());
      return -1;
   }
   Int_t namelen = Get(buf + kENTRY_NAMELEN_OFF,  kENTRY_NAMELEN_LEN);
   Int_t extlen  = Get(buf + kENTRY_EXTRALEN_OFF, kENTRY_EXTRALEN_LEN);

   member->fFilePosition = member->fPosition + kENTRY_HEADER_SIZE +
                           namelen + extlen;

   return 0;
}

//______________________________________________________________________________
Int_t TZIPFile::SetCurrentMember()
{
   // Find the desired member in the member array and make it the
   // current member. Returns -1 in case member is not found, 0 otherwise.

   fCurMember = 0;

   if (fMemberIndex > -1) {
      fCurMember = (TZIPMember *) fMembers->At(fMemberIndex);
      if (!fCurMember)
         return -1;
      fMemberName = fCurMember->GetName();
   } else {
      for (int i = 0; i < fMembers->GetEntriesFast(); i++) {
         TZIPMember *m = (TZIPMember *) fMembers->At(i);
         if (fMemberName == m->fName) {
            fCurMember   = m;
            fMemberIndex = i;
            break;
         }
      }
      if (!fCurMember)
         return -1;
   }

   return ReadMemberHeader((TZIPMember *)fCurMember);
}

//______________________________________________________________________________
UInt_t TZIPFile::Get(const void *buffer, Int_t bytes)
{
   // Read a "bytes" long little-endian integer value from "buffer".

   UInt_t value = 0;
#ifdef R__BYTESWAP
   memcpy(&value, buffer, bytes);
#else
   const UChar_t *buf = static_cast<const unsigned char *>(buffer);
   for (UInt_t shift = 0; bytes; shift += 8, --bytes, ++buf)
      value += *buf << shift;
#endif
   return value;
}

//______________________________________________________________________________
void TZIPFile::Print(Option_t *) const
{
   // Pretty print ZIP archive members.

   if (fMembers)
      fMembers->Print();
}


ClassImp(TZIPMember)

//______________________________________________________________________________
TZIPMember::TZIPMember()
{
   // Default ctor.

   fLocal     = 0;
   fLocalLen  = 0;
   fGlobal    = 0;
   fGlobalLen = 0;
   fCRC32     = 0;
   fAttrInt   = 0;
   fAttrExt   = 0;
   fMethod    = 0;
   fLevel     = 0;
}

//______________________________________________________________________________
TZIPMember::TZIPMember(const char *name)
   : TArchiveMember(name)
{
   // Create ZIP member file.

   fLocal     = 0;
   fLocalLen  = 0;
   fGlobal    = 0;
   fGlobalLen = 0;
   fCRC32     = 0;
   fAttrInt   = 0;
   fAttrExt   = 0;
   fMethod    = 0;
   fLevel     = 0;
}

//______________________________________________________________________________
TZIPMember::TZIPMember(const TZIPMember &member)
   : TArchiveMember(member)
{
   // Copy ctor.

   fLocal     = 0;
   fLocalLen  = member.fLocalLen;
   fGlobal    = 0;
   fGlobalLen = member.fGlobalLen;
   fCRC32     = member.fCRC32;
   fAttrInt   = member.fAttrInt;
   fAttrExt   = member.fAttrExt;
   fMethod    = member.fMethod;
   fLevel     = member.fLevel;

   if (member.fLocal) {
      fLocal = new char [fLocalLen];
      memcpy(fLocal, member.fLocal, fLocalLen);
   }
   if (member.fGlobal) {
      fGlobal = new char [fGlobalLen];
      memcpy(fGlobal, member.fGlobal, fGlobalLen);
   }
}

//______________________________________________________________________________
TZIPMember &TZIPMember::operator=(const TZIPMember &rhs)
{
   // Assignment operator.

   if (this != &rhs) {
      TArchiveMember::operator=(rhs);

      delete [] (char*) fLocal;
      delete [] (char*) fGlobal;

      fLocal     = 0;
      fLocalLen  = rhs.fLocalLen;
      fGlobal    = 0;
      fGlobalLen = rhs.fGlobalLen;
      fCRC32     = rhs.fCRC32;
      fAttrInt   = rhs.fAttrInt;
      fAttrExt   = rhs.fAttrExt;
      fMethod    = rhs.fMethod;
      fLevel     = rhs.fLevel;

      if (rhs.fLocal) {
         fLocal = new char [fLocalLen];
         memcpy(fLocal, rhs.fLocal, fLocalLen);
      }
      if (rhs.fGlobal) {
         fGlobal = new char [fGlobalLen];
         memcpy(fGlobal, rhs.fGlobal, fGlobalLen);
      }
   }
   return *this;
}

//______________________________________________________________________________
TZIPMember::~TZIPMember()
{
   // Cleanup.

   delete [] (char*) fLocal;
   delete [] (char*) fGlobal;
}

//______________________________________________________________________________
void TZIPMember::Print(Option_t *) const
{
   // Pretty print basic ZIP member info.

   //printf("%-20lld %s   %s\n", fDsize, fModTime.AsSQLString(), fName.Data());
   // above statement does not work with VC++7.1, spurious (null)
   printf("%-20lld", fDsize);
   printf(" %s   %s\n", fModTime.AsSQLString(), fName.Data());
}
