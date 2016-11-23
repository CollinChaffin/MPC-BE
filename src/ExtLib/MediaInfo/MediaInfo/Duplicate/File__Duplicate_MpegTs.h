/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef File__Duplicate_MpegTsH
#define File__Duplicate_MpegTsH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Duplicate/File__Duplicate__Base.h"
#include "MediaInfo/Duplicate/File__Duplicate__Writer.h"
#include <set>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File__Duplicate_MpegTs
//***************************************************************************

class File__Duplicate_MpegTs : public File__Duplicate__Base
{
public :
    //Constructor/Destructor
    File__Duplicate_MpegTs(const Ztring &Target);

    //Set
    bool   Configure (const Ztring &Value, bool ToRemove);

    //Write
    bool   Write (int16u PID, const int8u* ToAdd=NULL, size_t ToAdd_Size=0);

    //Output buffer
    size_t Output_Buffer_Get (unsigned char** Output_Buffer=NULL);
    bool Is_Wanted(int16u ProgNum, int16u PID) const;
private :
    File__Duplicate__Writer Writer;
    void Internal_Remove_Wanted_Program(int16u Program_number, bool ToRemove);
    //Configuration
    std::set<int16u> Wanted_program_numbers;
    std::set<int16u> Wanted_program_map_PIDs;
    std::set<int16u> Wanted_elementary_PIDs;
    std::set<int16u> Remove_program_numbers;
    std::set<int16u> Remove_program_map_PIDs;
    std::set<int16u> Remove_elementary_PIDs;

    //Current
public:
    std::vector<int8u> program_map_PIDs;
    std::vector<int8u> elementary_PIDs;
private:
    std::vector<int16u> elementary_PIDs_program_map_PIDs;

    struct buffer
    {
        int8u*          Buffer;
        size_t          Offset;
        size_t          Begin; //After pointer_field
        size_t          End;   //Before CRC
        size_t          Size;
        int8u           continuity_counter;
        int8u           version_number;
        int8u           FromTS_version_number_Last;
        bool            ConfigurationHasChanged;

        buffer()
        {
            Buffer=NULL;
            Offset=0;
            Begin=0;
            End=0;
            Size=0;
            continuity_counter=0xFF;
            version_number=0xFF;
            FromTS_version_number_Last=0xFF;
            ConfigurationHasChanged=true;
        }
        ~buffer()
        {
            delete[] Buffer; //Buffer=NULL;
        }
    };

    struct buffer_const
    {
        const int8u*    Buffer;
        size_t          Offset;
        size_t          Begin; //After pointer_field
        size_t          End;   //Before CRC
        size_t          Size;

        buffer_const()
        {
            Buffer=NULL;
            Offset=0;
            Begin=0;
            End=0;
            Size=0;
        }
    };

    struct buffer_big
    {
        int8u*          Buffer;
        size_t          Buffer_Size;
        size_t          Buffer_Size_Max;

        buffer_big()
        {
            Buffer=NULL;
            Buffer_Size=0;
            Buffer_Size_Max=0;
        }
        ~buffer_big()
        {
            delete[] Buffer; //Buffer=NULL;
        }
    };

    //Data
    bool Manage_PAT(const int8u* ToAdd, size_t ToAdd_Size);
    bool Manage_PMT(const int8u* ToAdd, size_t ToAdd_Size);

    //Buffers
    buffer_const                FromTS;
    std::map<int16u, buffer>    PAT;
    std::map<int16u, buffer>    PMT;
    std::map<int16u, buffer_big> BigBuffers; //key is pid

    //Helpers
    bool Parsing_Begin(const int8u* ToAdd, size_t ToAdd_Size, std::map<int16u, buffer> &ToModify);
    void Parsing_End(std::map<int16u, buffer> &ToModify);

    //Temp
    int16u StreamID;
};


} //NameSpace

#endif
