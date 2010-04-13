#! /bin/sh

# Generate a resource file holding the version info for a FILENAME / EXE.
# Called as
#    makeresource.sh <FILENAME>
# it will generate <FILENAME>.rc to be compiled with
#    %.res: %.rc
#		     rc -fo"$@" $< $CXXFLAGS
# This .res file can then be passed to the linker like an object file.
#
# Author: Axel, 2009.

FILENAME=$1

FILEBASE=`basename $FILENAME`
RC=$FILENAME.rc
FILESTEM=${FILEBASE%.*}

if [ "x`echo $FILENAME| grep -i '\.DLL$'`" = "x${FILENAME}" ]; then
	RCFILETYPE=VFT_DLL
	RCFILETITLE=library
else
	RCFILETYPE=VFT_APP
	RCFILETITLE=application
fi
if [ "$FILEBASE" = "root.exe" ]; then
	RCFILEICON="101 ICON \"icons/RootIcon.ico\""
else
	RCFILEICON=
fi

# Use svninfo.txt: more precise than the info in Rversion.h
SVNBRANCH=`cat etc/svninfo.txt|head -n1`
SVNREV=`cat etc/svninfo.txt|head -n2|tail -n1`
SVNDATE="`cat etc/svninfo.txt|tail -n1`"
SVNYEARCOMMA=`echo $SVNDATE|cut -d' ' -f3`

# Can't do that inside the .rc file: FILEVERSION doesn't evaluate a>>b
VERSION=`grep "ROOT_RELEASE " include/RVersion.h | sed 's,^.*"\([^"]*\)".*$,\1,' | sed 's,[^[:digit:]], ,g'`
VER1=`echo $VERSION| cut -d ' ' -f 1| sed 's,^0\+,,'`
VER2=`echo $VERSION| cut -d ' ' -f 2| sed 's,^0\+,,'`
VER3=`echo $VERSION| cut -d ' ' -f 3| sed 's,^0\+,,'`

# 0: tag, 1: trunk, 2: branch
VERBRANCHFLAG=1
if [ "x${SVNBRANCH/tag/}" != "x${SVNBRANCH}" ]; then
   VERBRANCHFLAG=0
elif [ "x${SVNBRANCH/branch/}" != "x${SVNBRANCH}" ]; then
   VERBRANCHFLAG=2
fi

DATE="`date +'%F %T'`"
HOST=`hostname`

cat > $RC <<EOF
// ROOT version resource file for $FILENAME
// Generated by $0 on $DATE

#include "RConfig.h"
#include <windows.h>
#include <winver.h>

#if $RCFILETYPE == VFT_APP
LANGUAGE LANG_NEUTRAL, SUBLANG_NEUTRAL
#pragma code_page(1250)
${RCFILEICON}
#endif

#define ROOT_VERSION_STR ROOT_RELEASE " (r${SVNREV}@${SVNBRANCH}, ${SVNDATE})\0"

#if (${VERBRANCHFLAG} != 0)
# define ROOT_IS_PRERELEASE VS_FF_PRERELEASE
#else 
# define ROOT_IS_PRERELEASE 0
#endif

#ifndef DEBUG
#define ROOT_IS_DEBUG 0
#else
#define ROOT_IS_DEBUG VS_FF_DEBUG
#endif

VS_VERSION_INFO VERSIONINFO
FILEVERSION     $VER1, $VER2, $VER3, $VERBRANCHFLAG
PRODUCTVERSION  $VER1, $VER2, $VER3, $VERBRANCHFLAG
FILEFLAGSMASK   VS_FFI_FILEFLAGSMASK
FILEFLAGS       (ROOT_IS_DEBUG | ROOT_IS_PRERELEASE)
FILEOS          VOS__WINDOWS32
FILETYPE        $RCFILETYPE
BEGIN
   BLOCK "VarFileInfo"
   BEGIN
      VALUE "Translation", 0x409, 1252
   END

   BLOCK "StringFileInfo"
   BEGIN
      BLOCK "040904E4"
      BEGIN
         VALUE "Built By",        "$USER\0"
         VALUE "Build Host",      "$HOST\0"
         VALUE "Build Time",      "$DATE\0"
         VALUE "Comments",        "ROOT: An Object-Oriented Data Analysis Framework\0"
         VALUE "CompanyName",     "The ROOT Team\0"
         VALUE "FileDescription", "ROOT ${RCFILETITLE} ${FILESTEM}\0"
         VALUE "FileVersion",      ROOT_VERSION_STR
         VALUE "InternalName",     "${FILESTEM}\0"
         VALUE "LegalCopyright",  "Copyright (C) 1995-${SVNYEARCOMMA} Rene Brun and Fons Rademakers.\0"
         VALUE "OriginalFilename","${FILENAME}\0"
         VALUE "ProductName",     "ROOT\0"
         VALUE "ProductVersion",  ROOT_VERSION_STR
      END
   END
END
EOF
