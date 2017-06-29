/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about VC-3 video streams
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_Vc3H
#define MediaInfo_Vc3H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/Multiple/File_Mpeg4.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Avs
//***************************************************************************

class File_Vc3 : public File__Analyze
{
public :
    //In
    int64u  Frame_Count_Valid;
    float64 FrameRate;

    //constructor/Destructor
    File_Vc3();
    ~File_Vc3();

private :
    //Streams management
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - Demux
    #if MEDIAINFO_DEMUX
    bool Demux_UnpacketizeContainer_Test();
    #endif //MEDIAINFO_DEMUX

    //Buffer - Global
    void Read_Buffer_Unsynched();

    //Buffer - Per element
    bool Header_Begin ();
    void Header_Parse ();
    void Data_Parse ();

    //Elements
    void HeaderPrefix();
    void CodingControlA();
    void ImageGeometry();
    void CompressionID();
    void CodingControlB();
    void TimeCode();
    void UserData();
    void UserData_8();
    void MacroblockScanIndices();

    //Parsers
    #if defined(MEDIAINFO_CDP_YES)
        File__Analyze*  Cdp_Parser;
    #endif //defined(MEDIAINFO_CDP_YES)

    //Temp
    string  TimeCode_FirstFrame;
    int32u  HS;
    int32u  CID;
    int16u  ALPF;
    int16u  SPL;
    int16u  PARC;
    int16u  PARN;
    int8u   SBD;
    int8u   FFC_FirstFrame;
    int8u   HVN;
    int8u   CLV;
    bool    CLF;
    bool    SSC;
    bool    CRCF;
    bool    SST;
    bool    VBR;
    bool    PMA;
    bool    LLA;
    bool    ALP;
    bool    FFE;
};

} //NameSpace

#endif
