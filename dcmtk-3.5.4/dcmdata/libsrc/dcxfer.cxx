/*
 *
 *  Copyright (C) 1994-2005, OFFIS
 *
 *  This software and supporting documentation were developed by
 *
 *    Kuratorium OFFIS e.V.
 *    Healthcare Information and Communication Systems
 *    Escherweg 2
 *    D-26121 Oldenburg, Germany
 *
 *  THIS SOFTWARE IS MADE AVAILABLE,  AS IS,  AND OFFIS MAKES NO  WARRANTY
 *  REGARDING  THE  SOFTWARE,  ITS  PERFORMANCE,  ITS  MERCHANTABILITY  OR
 *  FITNESS FOR ANY PARTICULAR USE, FREEDOM FROM ANY COMPUTER DISEASES  OR
 *  ITS CONFORMITY TO ANY SPECIFICATION. THE ENTIRE RISK AS TO QUALITY AND
 *  PERFORMANCE OF THE SOFTWARE IS WITH THE USER.
 *
 *  Module:  dcmdata
 *
 *  Author:  Gerd Ehlers
 *
 *  Purpose: handling of transfer syntaxes
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:42:11 $
 *  CVS/RCS Revision: $Revision: 1.25 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */
#include <algorithm>
#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/dcmdata/dcxfer.h"
#include "dcmtk/dcmdata/dcuid.h"
#include "dcmtk/dcmdata/dcdebug.h"

#define INCLUDE_CSTRING
#include "dcmtk/ofstd/ofstdinc.h"


typedef struct
{
    const char         *xferID;
    const char         *xferName;
    const char         *xferShortName; // max length is 13: JpegLess14SV1
    E_TransferSyntax    xfer;
    E_ByteOrder         byteOrder;
    E_VRType            vrType;
    E_JPEGEncapsulated  encapsulated;
    Uint32              JPEGProcess8;
    Uint32              JPEGProcess12;
    E_StreamCompression streamCompression;
} S_XferNames;


#define ERROR_XferName "UnknownTransferSyntax"
#define ERROR_XferShortName "Unknown"

const S_XferNames XferNames[] =
{
    { UID_LittleEndianImplicitTransferSyntax,
      "LittleEndianImplicit", "LEImpLess",
      EXS_LittleEndianImplicit,
      EBO_LittleEndian,
      EVT_Implicit,
      EJE_NotEncapsulated,
      0L, 0L,
      ESC_none },
    { "",  // illegal type
      "VirtualBigEndianImplicit", "BEImpLess",
      EXS_BigEndianImplicit,
      EBO_BigEndian,
      EVT_Implicit,
      EJE_NotEncapsulated,
      0L, 0L,
      ESC_none },
    { UID_LittleEndianExplicitTransferSyntax,
      "LittleEndianExplicit", "LEExpLess",
      EXS_LittleEndianExplicit,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_NotEncapsulated,
      0L, 0L,
      ESC_none },
    { UID_BigEndianExplicitTransferSyntax,  // defined in dctypes.h
      "BigEndianExplicit", "BEExpLess",
      EXS_BigEndianExplicit,
      EBO_BigEndian,
      EVT_Explicit,
      EJE_NotEncapsulated,
      0L, 0L,
      ESC_none },
    { UID_JPEGProcess1TransferSyntax,
      "JPEG Baseline", "JpegBase",
      EXS_JPEGProcess1TransferSyntax,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      1L, 1L,
      ESC_none },
    { UID_JPEGProcess2_4TransferSyntax,
      "JPEG Extended, Process 2+4", "Jpeg24",
      EXS_JPEGProcess2_4TransferSyntax,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      2L ,4L,
      ESC_none },
    { UID_JPEGProcess3_5TransferSyntax,
      "JPEG Extended, Process 3+5", "Jpeg35",
      EXS_JPEGProcess3_5TransferSyntax,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      3L ,5L,
      ESC_none },
    { UID_JPEGProcess6_8TransferSyntax,
      "JPEG Spectral Selection, Non-hierarchical, Process 6+8", "Jpeg68",
      EXS_JPEGProcess6_8TransferSyntax,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      6L ,8L,
      ESC_none },
    { UID_JPEGProcess7_9TransferSyntax,
      "JPEG Spectral Selection, Non-hierarchical, Process 7+9", "Jpeg79",
      EXS_JPEGProcess7_9TransferSyntax,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      7L ,9L,
      ESC_none },
    { UID_JPEGProcess10_12TransferSyntax,
      "JPEG Full Progression, Non-hierarchical, Process 10+12", "Jpeg1012",
      EXS_JPEGProcess10_12TransferSyntax,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      10L ,12L,
      ESC_none },
    { UID_JPEGProcess11_13TransferSyntax,
      "JPEG Full Progression, Non-hierarchical, Process 11+13", "Jpeg1113",
      EXS_JPEGProcess11_13TransferSyntax,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      11L ,13L,
      ESC_none },
    { UID_JPEGProcess14TransferSyntax,
      "JPEG Lossless, Non-hierarchical, Process 14", "JpegLess14",
      EXS_JPEGProcess14TransferSyntax,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      14L ,14L,
      ESC_none },
    { UID_JPEGProcess15TransferSyntax,
      "JPEG Lossless, Non-hierarchical, Process 15", "JpegLess15",
      EXS_JPEGProcess15TransferSyntax,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      15L ,15L,
      ESC_none },
    { UID_JPEGProcess16_18TransferSyntax,
      "JPEG Extended, Hierarchical, Process 16+18", "Jpeg1618",
      EXS_JPEGProcess16_18TransferSyntax,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      16L ,18L,
      ESC_none },
    { UID_JPEGProcess17_19TransferSyntax,
      "JPEG Extended, Hierarchical, Process 17+19", "Jpeg1719",
      EXS_JPEGProcess17_19TransferSyntax,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      17L ,19L,
      ESC_none },
    { UID_JPEGProcess20_22TransferSyntax,
      "JPEG Spectral Selection, Hierarchical, Process 20+22", "Jpeg2022",
      EXS_JPEGProcess20_22TransferSyntax,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      20L ,22L,
      ESC_none },
    { UID_JPEGProcess21_23TransferSyntax,
      "JPEG Spectral Selection, Hierarchical, Process 21+23", "Jpeg2123",
      EXS_JPEGProcess21_23TransferSyntax,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      21L ,23L,
      ESC_none },
    { UID_JPEGProcess24_26TransferSyntax,
      "JPEG Full Progression, Hierarchical, Process 24+26", "Jpeg2426",
      EXS_JPEGProcess24_26TransferSyntax,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      24L ,26L,
      ESC_none },
    { UID_JPEGProcess25_27TransferSyntax,
      "JPEG Full Progression, Hierarchical, Process 25+27", "Jpeg2527",
      EXS_JPEGProcess25_27TransferSyntax,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      25L ,27L,
      ESC_none },
    { UID_JPEGProcess28TransferSyntax,
      "JPEG Lossless, Hierarchical, Process 28", "JpegLess28",
      EXS_JPEGProcess28TransferSyntax,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      28L ,28L,
      ESC_none },
    { UID_JPEGProcess29TransferSyntax,
      "JPEG Lossless, Hierarchical, Process 29", "JpegLess29",
      EXS_JPEGProcess29TransferSyntax,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      29L ,29L,
      ESC_none },
    { UID_JPEGProcess14SV1TransferSyntax,
      "JPEG Lossless, Non-hierarchical, 1st Order Prediction", "JpegLess14SV1",
      EXS_JPEGProcess14SV1TransferSyntax,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      14L ,14L,
      ESC_none },
    { UID_RLELosslessTransferSyntax,
      "RLE Lossless", "RLELess",
      EXS_RLELossless,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      0L, 0L,
      ESC_none },
    { UID_JPEGLSLosslessTransferSyntax,
      "JPEG-LS Lossless", "LSLess",
      EXS_JPEGLSLossless,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      0L, 0L,
      ESC_none },
    { UID_JPEGLSLossyTransferSyntax,
      "JPEG-LS Lossy (Near-lossless)", "LSLossy",
      EXS_JPEGLSLossy,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      0L, 0L,
      ESC_none },
    { UID_DeflatedExplicitVRLittleEndianTransferSyntax,
      "Deflated Explicit VR Little Endian", "DefLEExpLess",
      EXS_DeflatedLittleEndianExplicit,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_NotEncapsulated,
      0L, 0L,
#ifdef WITH_ZLIB
      ESC_zlib
#else
      ESC_unsupported
#endif
    },
    { UID_JPEG2000LosslessOnlyTransferSyntax,
      "JPEG 2000 (Lossless only)", "Jp2kLossLess",
      EXS_JPEG2000LosslessOnly,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      0L, 0L,
      ESC_none },
    { UID_JPEG2000TransferSyntax,
      "JPEG 2000 (Lossless or Lossy)", "Jp2kLossy",
      EXS_JPEG2000,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      0L, 0L,
      ESC_none },
    { UID_MPEG2MainProfileAtMainLevelTransferSyntax,
      "MPEG2 Main Profile @ Main Level", "Mpeg2",
      EXS_MPEG2MainProfileAtMainLevel,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      0L, 0L,
      ESC_none },
   { UID_JPEG2000Part2MulticomponentImageCompressionLosslessOnlyTransferSyntax,
      "JPEG 2000 Part 2 Multicomponent Image Compression (Lossless only)", "Jp2kP2Lossy",
      EXS_JPEG2000MulticomponentLosslessOnly,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      0L, 0L,
      ESC_none },
   { UID_JPEG2000Part2MulticomponentImageCompressionTransferSyntax,
      "JPEG 2000 Part 2 Multicomponent Image Compression (Lossless or Lossy)", "Jp2kP2Less",
      EXS_JPEG2000Multicomponent,
      EBO_LittleEndian,
      EVT_Explicit,
      EJE_Encapsulated,
      0L, 0L,
      ESC_none }

    // enter further transfer syntaxes here ...
};

const int DIM_OF_XferNames = (sizeof(XferNames) / sizeof(S_XferNames));


// ********************************


DcmXfer::DcmXfer(E_TransferSyntax xfer)
  : xferID(""),
    xferName(ERROR_XferName),
    xferShortName(ERROR_XferShortName),
    xferSyn(EXS_Unknown),
    byteOrder(EBO_unknown),
    vrType(EVT_Implicit),
    encapsulated(EJE_NotEncapsulated),
    JPEGProcess8(0L),
    JPEGProcess12(0L),
    streamCompression(ESC_none)
{
    int i = 0;
    while ((i < DIM_OF_XferNames) && XferNames[i].xfer != xfer)
        i++;
    if ((i < DIM_OF_XferNames) && (XferNames[i].xfer == xfer))
    {
        xferSyn           = XferNames[i].xfer;
        xferID            = XferNames[i].xferID;
        xferName          = XferNames[i].xferName;
        xferShortName     = XferNames[i].xferShortName;
        byteOrder         = XferNames[i].byteOrder;
        vrType            = XferNames[i].vrType;
        encapsulated      = XferNames[i].encapsulated;
        JPEGProcess8      = XferNames[i].JPEGProcess8;
        JPEGProcess12     = XferNames[i].JPEGProcess12;
        streamCompression = XferNames[i].streamCompression;
    }
}


// ********************************


DcmXfer::DcmXfer(const char* xferName_xferID)
  : xferID(""),
    xferName(ERROR_XferName),
    xferShortName(ERROR_XferShortName),
    xferSyn(EXS_Unknown),
    byteOrder(EBO_unknown),
    vrType(EVT_Implicit),
    encapsulated(EJE_NotEncapsulated),
    JPEGProcess8(0L),
    JPEGProcess12(0L),
    streamCompression(ESC_none)
{
    if (xferName_xferID == NULL) return;
    const string xname(xferName_xferID);
    bool isName = false,
        isUID = all_of(xname.begin(), xname.end(), [](const char c){ return (c >= '0' && c <= '9') || c == '.'; });
    if(!isUID) isName = any_of(xname.begin(), xname.end(), [](const char c){ return c == ' '; }); 
    if(!isUID && !isName) isName = xname.length() > 13;
    const S_XferNames *pxn = find_if(XferNames, XferNames + DIM_OF_XferNames, 
        [isUID, isName, xferName_xferID](const S_XferNames &rXferName){
            return 0 == strcmp(xferName_xferID, isUID ? rXferName.xferID : (isName ? rXferName.xferName : rXferName.xferShortName));
        });
    if (pxn < XferNames + DIM_OF_XferNames)
    {
        xferSyn           = pxn->xfer;
        xferID            = pxn->xferID;
        xferName          = pxn->xferName;
        xferShortName     = pxn->xferShortName;
        byteOrder         = pxn->byteOrder;
        vrType            = pxn->vrType;
        encapsulated      = pxn->encapsulated;
        JPEGProcess8      = pxn->JPEGProcess8;
        JPEGProcess12     = pxn->JPEGProcess12;
        streamCompression = pxn->streamCompression;
    }
}


// ********************************


DcmXfer::DcmXfer(const DcmXfer &newXfer)
  : xferID(newXfer.xferID),
    xferName(newXfer.xferName),
    xferShortName(newXfer.xferShortName),
    xferSyn(newXfer.xferSyn),
    byteOrder(newXfer.byteOrder),
    vrType(newXfer.vrType),
    encapsulated(newXfer.encapsulated),
    JPEGProcess8(newXfer.JPEGProcess8),
    JPEGProcess12(newXfer.JPEGProcess12),
    streamCompression(newXfer.streamCompression)
{
}


// ********************************


DcmXfer::~DcmXfer()
{
}


// ********************************


DcmXfer &DcmXfer::operator=(const E_TransferSyntax xfer)
{
    int i = 0;
    while ((i < DIM_OF_XferNames) && (XferNames[i].xfer != xfer))
        i++;
    if ((i < DIM_OF_XferNames) && (XferNames[i].xfer == xfer))
    {
        xferSyn           = XferNames[i].xfer;
        xferID            = XferNames[i].xferID;
        xferName          = XferNames[i].xferName;
        xferShortName     = XferNames[i].xferShortName;
        byteOrder         = XferNames[i].byteOrder;
        vrType            = XferNames[i].vrType;
        encapsulated      = XferNames[i].encapsulated;
        JPEGProcess8      = XferNames[i].JPEGProcess8;
        JPEGProcess12     = XferNames[i].JPEGProcess12;
        streamCompression = XferNames[i].streamCompression;
    } else {
        xferSyn           = EXS_Unknown;
        xferID            = "";
        xferName          = ERROR_XferName;
        xferShortName     = ERROR_XferShortName;
        byteOrder         = EBO_unknown;
        vrType            = EVT_Implicit;
        encapsulated      = EJE_NotEncapsulated;
        JPEGProcess8      = 0L;
        JPEGProcess12     = 0L;
        streamCompression = ESC_none;
    }
    return *this;
}


// ********************************


DcmXfer &DcmXfer::operator=(const DcmXfer &newXfer)
{
    if (this != &newXfer)
    {
        xferSyn           = newXfer.xferSyn;
        xferID            = newXfer.xferID;
        xferName          = newXfer.xferName;
        xferShortName     = newXfer.xferShortName;
        byteOrder         = newXfer.byteOrder;
        vrType            = newXfer.vrType;
        encapsulated      = newXfer.encapsulated;
        JPEGProcess8      = newXfer.JPEGProcess8;
        JPEGProcess12     = newXfer.JPEGProcess12;
        streamCompression = newXfer.streamCompression;
    }
    return *this;
}


// ********************************

Uint32 DcmXfer::sizeofTagHeader(DcmEVR evr)
{
    Uint32 len = 0;
    if (isExplicitVR())
    {
        // some VRs have an extended format
        DcmVR vr(evr);
        if (vr.usesExtendedLengthEncoding()) {
            len = 12;  // for Tag, Length, VR und reserved
        } else {
            len = 8;   // for Tag, Length und VR
        }
    } else {
        // all implicit VRs have the same format
        len = 8;       // for Tag und Length
    }
    return len;
}

// ********************************

static E_ByteOrder FindMachineTransferSyntax()
{
    E_ByteOrder localByteOrderFlag;
    union
    {
        Uint32 ul;
        char uc[4];
    } tl;

    union
    {
        Uint16 us;
        char uc[2];
    } ts;

    tl.ul = 1;
    ts.us = 1;

    if (tl.uc[0] == 1 && !(tl.uc[1] | tl.uc[2] | tl.uc[3]) && ts.uc[0] == 1 && !(ts.uc[1]))
        localByteOrderFlag = EBO_LittleEndian;
    else if (tl.uc[3] == 1 && !(tl.uc[0] | tl.uc[1] | tl.uc[2]) && ts.uc[1] == 1 && !(ts.uc[0]))
        localByteOrderFlag = EBO_BigEndian;
    else
        localByteOrderFlag = EBO_unknown;

    return localByteOrderFlag;
}

// global constant: local byte order (little or big endian)
const E_ByteOrder gLocalByteOrder = FindMachineTransferSyntax();


/*
 * CVS/RCS Log:
 * $Log: dcxfer.cc,v $
 * Revision 1.25  2005/12/08 15:42:11  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.24  2005/10/25 08:55:34  meichel
 * Updated list of UIDs and added support for new transfer syntaxes
 *   and storage SOP classes.
 *
 * Revision 1.23  2004/04/06 18:09:14  joergr
 * Updated data dictionary, UIDs and transfer syntaxes for the latest Final Text
 * Supplements (42 and 47) and Correction Proposals (CP 25).
 * Added missing suffix "TransferSyntax" to some transfer syntax constants.
 *
 * Revision 1.22  2004/01/16 13:44:46  joergr
 * Adapted type casts to new-style typecast operators defined in ofcast.h.
 *
 * Revision 1.21  2002/11/29 17:06:50  joergr
 * Fixed doc++ warning about different number of opening and closing brackets.
 * Replaced German comments by English translations.
 *
 * Revision 1.20  2002/11/27 12:07:01  meichel
 * Adapted module dcmdata to use of new header file ofstdinc.h
 *
 * Revision 1.19  2002/08/27 16:56:01  meichel
 * Initial release of new DICOM I/O stream classes that add support for stream
 *   compression (deflated little endian explicit VR transfer syntax)
 *
 * Revision 1.18  2002/06/19 15:35:01  meichel
 * Fixed typo in transfer syntax name
 *
 * Revision 1.17  2001/11/08 16:17:34  meichel
 * Updated data dictionary, UIDs and transfer syntaxes for DICOM 2001 edition.
 *
 * Revision 1.16  2001/06/01 15:49:23  meichel
 * Updated copyright header
 *
 * Revision 1.15  2001/01/17 10:20:38  meichel
 * Added toolkit support for JPEG-LS transfer syntaxes
 *
 * Revision 1.14  2000/04/14 16:10:35  meichel
 * Minor changes for thread safety.
 *
 * Revision 1.13  2000/03/08 16:26:53  meichel
 * Updated copyright header.
 *
 * Revision 1.12  1999/03/31 09:26:05  meichel
 * Updated copyright header in module dcmdata
 *
 *
 */
