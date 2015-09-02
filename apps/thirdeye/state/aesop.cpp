/*
 * aesop.cpp
 *
 *  Created on: Aug 19, 2015
 *      Author: bcurtis
 */

#include <iostream>
#include <iomanip>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "aesop.hpp"

#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

AESOP::Aesop::Aesop(RESOURCES::Resource &resource):mRes(resource) {
    // load and initialize 'start' sop
    uint16_t start_index = mRes.getIndex("kernel");
    mSOP[start_index] = std::make_unique<SOP>(mRes, start_index);

    std::cout << "DEBUG: " << mSOP[start_index]->getPC() << std::endl;
    std::cout << "DEBUG: " << mSOP[start_index]->getSOPHeader().import_resource << std::endl;
}

AESOP::Aesop::~Aesop() {
    //cleanup
}

void AESOP::Aesop::show() {

    const std::vector<uint8_t> &sop = mRes.getAsset("start");
    sop_data = sop;

    SOPScriptHeader sopHeader = reinterpret_cast<SOPScriptHeader&>(sop_data[0]);
    position = sizeof(SOPScriptHeader);


    const std::vector<uint8_t> &sop_import = mRes.getAsset(sopHeader.import_resource);
    SOPImExHeader sopImportHeader = reinterpret_cast<const SOPImExHeader&>(sop_import[0]);
    uint16_t imPosition = sizeof(SOPImExHeader);

    while (imPosition < sop_import.size()){
        const uint32_t end_marker = reinterpret_cast<const uint32_t&>(sop_import[imPosition]);
        if (end_marker == 0)
            break;
        uint16_t string_size = reinterpret_cast<const uint16_t&>(sop_import[imPosition]);
        imPosition += 2;
        //std::cout << "First IMPORT string size: " << string_size;
        std::string function(reinterpret_cast<const char*>(sop_import.data()) + imPosition, string_size);
        imPosition += string_size;

        std::vector<std::string> fields;
        boost::split(fields, function, boost::is_any_of(":"));
        fields[1].pop_back();   // trim the last 0 byte.
        function = fields[1];

        //std::cout << " value: " << function;

        string_size = reinterpret_cast<const uint16_t&>(sop_import[imPosition]);
        imPosition += 2;
        //std::cout << " Second IMPORT string size: " << string_size;
        std::string number(reinterpret_cast<const char*>(sop_import.data()) + imPosition, string_size);
        imPosition += string_size;
        //std::cout << " value: " << number << std::endl;
        number.pop_back(); // trim off null terminator
        mImport[boost::lexical_cast<uint16_t>(number)] = function;
    }

    const std::vector<uint8_t> &sop_export = mRes.getAsset(sopHeader.export_resource);
    SOPImExHeader sopExportHeader = reinterpret_cast<const SOPImExHeader&>(sop_export[0]);
    uint16_t exPosition = sizeof(SOPImExHeader);
    while (exPosition < sop_export.size()){
        uint32_t end_marker = reinterpret_cast<const uint32_t&>(sop_export[exPosition]);
        if (end_marker == 0)
            break;
        uint16_t string_size = reinterpret_cast<const uint16_t&>(sop_export[exPosition]);
        exPosition += 2;
        //std::cout << "First EXPORT string size: " << string_size;
        std::string string(reinterpret_cast<const char*>(sop_export.data()) + exPosition, string_size);
        exPosition += string_size;
        //std::cout << " value: " << string;

        std::vector<std::string> fields;
        boost::split(fields, string, boost::is_any_of(":"));
        fields[1].pop_back();   // trim the last 0 byte.

        std::string message_name = "";
        if (fields[1] != "OBJECT"){ // ignore the header object
            uint16_t object_number = boost::lexical_cast<uint16_t>(fields[1]);
            //std::cout << " int: " << object_number;
            message_name = mRes.getTableEntry(object_number, 4);
            //std::cout << " message: " << message_name;
        }

        //std::cout << "(" << fields[0]+'\0' << " : " << fields[1]+'\0' << ") ";

        string_size = reinterpret_cast<const uint16_t&>(sop_export[exPosition]);
        exPosition += 2;
        //std::cout << " Second EXPORT string size: " << string_size;
        string = std::string(reinterpret_cast<const char*>(sop_export.data()) + exPosition, string_size);
        exPosition += string_size;
        string.pop_back();  // trim off null terminator
        //std::cout << " value: " << string << " " << string.length() << " for " << message_name << std::endl;
        if (fields[1] != "OBJECT"){ // ignore the header object
            mExport[boost::lexical_cast<uint16_t>(string)] = message_name;
        }
    }

    uint16_t local_var_size;
    std::stringbuf op_output;
    std::ostream op_output_stream(&op_output);
    while (position < sop_data.size()){
        op_output.str("");
        uint16_t start_position = position;
        std::string s_op = "";
        uint32_t value = 0;
        uint32_t end_value = 0;
        std::string s_value = "";

        if (mExport.count(position) == 1){
            local_var_size = reinterpret_cast<uint16_t&>(sop_data[position]);
            std::cout << std::endl
                      << "AESOP Method: '" << mExport[position] << "'"
                      << std::endl << ' ' << std::setfill(' ') << std::setw(4)
                      << start_position << "/" << sop_data.size()
                      << " local_var_size: " << local_var_size
                      << std::endl;
            position += sizeof(uint16_t);
        }

        uint8_t &op = getByte();
        switch (op) {
        case OP_BRT:
            s_op = "BRT";
            value = getWord();
            break;
        case OP_BRF:
            s_op = "BRF";
            value = getWord();
            break;
        case OP_BRA:
            s_op = "BRA";
            value = getWord();
            break;
        case OP_CASE:
            s_op = "CASE";
            value = getWord();  //number of entries in this CASE
            for (uint16_t i = 0; i < value; i++){
                uint32_t case_value = getLong();
                uint16_t jump_address = getWord();
                op_output_stream << std::endl << "          CASE #"
                                 << i << ": "
                                 << case_value << " -> " << jump_address;
            }
            {
                uint16_t default_jump_address = getWord();
                op_output_stream << std::endl << "          DEFAULT: -> "
                                 << default_jump_address;
            }
            break;
        case OP_PUSH:
            // TODO: no value, what do we do?
            s_op = "PUSH";
            break;
        case OP_NEG:
            s_op = "NEG";
            break;
        case OP_EXP:
            s_op = "EXP";
            break;
        case OP_SHTC:
            s_op = "SHTC";
            value = getByte();
            break;
        case OP_INTC:
            s_op = "INTC";
            value = getWord();
            break;
        case OP_LNGC:
            s_op = "LNGC";
            value = getLong();
            break;
        case OP_RCRS:
            s_op = "RCRS";
            value = getWord();
            op_output_stream << mImport[boost::lexical_cast<uint16_t>(value)];
            break;
        case OP_CALL:
            s_op = "CALL";
            value = getByte();
            break;
        case OP_SEND:
            s_op = "SEND";
            value = getByte();
            op_output_stream << " " << value;
            value = getWord();
            op_output_stream << " -> '" << mRes.getTableEntry(value, 4) << "'";
            break;
        case OP_LAB:
            s_op = "LAB";
            value = getWord();
            break;
        case OP_LAW:
            s_op = "LAW";
            value = getWord();
            break;
        case OP_LAD:
            s_op = "LAD";
            value = getWord();
            break;
        case OP_SAW:
            s_op = "SAW";
            value = getWord();
            break;
        case OP_SAD:
            s_op = "SAD";
            value = getWord();
            break;
        case OP_LXD:
            s_op = "LXD";
            value = getWord();
            break;
        case OP_SXB:
            s_op = "SXB";
            value = getWord();
            break;
        case OP_LXDA:
            s_op = "LXDA";
            value = getWord();
            break;
        case OP_LECA:
            s_op = "LECA";
            value = getWord();
            getByte();
            end_value = getWord();
            s_value = std::string(reinterpret_cast<const char*>(sop_data.data()) + value, end_value - value);
            position += end_value-value;
            break;
        case OP_END:
            s_op = "END";
            break;
        default:
            s_op = "UNKN";
            value = 0xFF;
            break;
        }

        std::cout << ' ' <<
                     std::setfill(' ') << std::setw(4) <<
                     start_position << "/" << sop_data.size() << " " <<
                     std::hex << std::setw(4) <<
                     s_op << " (0x" << std::setfill('0') << std::setw(2) <<
                     (uint16_t) op << "):  " << std::dec <<
                     (uint16_t) value << " (" << s_value << ") " << op_output.str() <<
                     std::endl;
        //break;
    }

    return;
}

uint8_t &AESOP::Aesop::getByte(){
    position += 1;
    return sop_data[position-1];
}

uint16_t &AESOP::Aesop::getWord(){
    position += 2;
    return reinterpret_cast<uint16_t&>(sop_data[position-2]);
}

uint32_t &AESOP::Aesop::getLong(){
    position += 4;
    return reinterpret_cast<uint32_t&>(sop_data[position-4]);
}



AESOP::SOP::SOP(RESOURCES::Resource &resource, uint16_t index):
mRes(resource), mIndex(index){
    mPC = 0;    // set our position counter
    mSOP = mRes.getAsset(mIndex);
    mSOPHeader = reinterpret_cast<SOPScriptHeader&>(mSOP[0]);
    mPC += sizeof(SOPScriptHeader);

    std::cout << "SOP HEADER " <<
                 mSOPHeader.static_size << " " <<
                 mSOPHeader.import_resource << " " <<
                 mSOPHeader.export_resource << " " <<
                 std::setw(2) << std::setfill('0') << std::hex <<
                 mSOPHeader.parent << " " <<
                 std::dec <<
                 mPC << std::endl;

    mSOPImport = mRes.getAsset(mSOPHeader.import_resource);
    mSOPImportHeader = reinterpret_cast<SOPImExHeader&>(mSOPImport[0]);
    uint16_t IP = sizeof(SOPImExHeader);

    mSOPExport = mRes.getAsset(mSOPHeader.export_resource);
    mSOPExportHeader = reinterpret_cast<SOPImExHeader&>(mSOPExport[0]);
    uint16_t EP = sizeof(SOPImExHeader);

    std::cout << "IMPORT:" << std::endl;
    //std::map<uint16_t, std::string> mImport;
    // parse import resource
    while (IP < mSOPImport.size()){
        if (reinterpret_cast<const uint32_t&>(mSOPImport[IP]) == 0)
            break;  // stop processing when end is reached

        uint16_t string_size = reinterpret_cast<const uint16_t&>(mSOPImport[IP]);
        IP += 2;
        std::string function(reinterpret_cast<const char*>(mSOPImport.data()) + IP, string_size);
        IP += string_size;

        // split on :, not currently needed
        //std::vector<std::string> fields;
        //boost::split(fields, function, boost::is_any_of(":"));
        //fields[1].pop_back();   // trim the last 0 byte.
        //function = fields[1];

        std::cout << function;

        string_size = reinterpret_cast<const uint16_t&>(mSOPImport[IP]);
        IP += 2;
        std::string number(reinterpret_cast<const char*>(mSOPImport.data()) + IP, string_size);
        IP += string_size;
        std::cout << ", " << number << std::endl;
        number.pop_back(); // trim off null terminator
        //mImport[boost::lexical_cast<uint16_t>(number)] = function;
    }

    std::cout << std::endl;
    std::cout << "EXPORT:" << std::endl;
    while (EP < mSOPExport.size()){
        uint32_t end_marker = reinterpret_cast<const uint32_t&>(mSOPExport[EP]);
        if (end_marker == 0)
            break;

        // parse first string
        uint16_t string_size = reinterpret_cast<const uint16_t&>(mSOPExport[EP]);
        EP += 2;
        std::string first_string(reinterpret_cast<const char*>(mSOPExport.data()) + EP, string_size);
        EP += string_size;
        std::cout << first_string;
        // split on : in first_string
        std::vector<std::string> first_fields;
        boost::split(first_fields, first_string, boost::is_any_of(":"));
        first_fields[first_fields.size()-1].pop_back();   // trim the last 0 byte.

        // parse second string
        string_size = reinterpret_cast<const uint16_t&>(mSOPExport[EP]);
        EP += 2;
        std::string second_string(reinterpret_cast<const char*>(mSOPExport.data()) + EP, string_size);
        EP += string_size;
        second_string.pop_back();  // trim off null terminator
        std::cout << ", " << second_string << std::endl; //<< " for " << message_name << std::endl;
        // split on , in second_string
        std::vector<std::string> second_fields;
        boost::split(second_fields, second_string, boost::is_any_of(","));
        int8_t number_of_elements = -1;
        if (second_fields.size() == 2)
            number_of_elements = boost::lexical_cast<int8_t>(second_fields[second_fields.size()-1]);

        uint16_t index;
        // massage data by index
        if (first_string[0] == IMEX_METHOD){
            index = boost::lexical_cast<uint16_t>(first_fields[1]);
            mSOPImportData[index].first = first_string;
            mSOPImportData[index].second = second_string;
            mSOPImportData[index].position = boost::lexical_cast<uint16_t>(second_string);
            mSOPImportData[index].type = IMEX_METHOD;
            mSOPImportData[index].table_entry = mRes.getTableEntry(index, 4);
            mSOPImportData[index].elements = number_of_elements;
        } else if (first_string[0] == IMEX_BYTE){
            index = boost::lexical_cast<uint16_t>(second_string);
            mSOPImportData[index].first = first_string;
            mSOPImportData[index].second = second_string;
            mSOPImportData[index].position = -1;
            mSOPImportData[index].type = IMEX_BYTE;
            mSOPImportData[index].table_entry = mRes.getTableEntry(index, 4);
            mSOPImportData[index].elements = number_of_elements;
        } else if (first_string[0] == IMEX_LONG){
            index = boost::lexical_cast<uint16_t>(second_string);
            mSOPImportData[index].first = first_string;
            mSOPImportData[index].second = second_string;
            mSOPImportData[index].position = -1;
            mSOPImportData[index].type = IMEX_LONG;
            mSOPImportData[index].table_entry = mRes.getTableEntry(index, 4);
            mSOPImportData[index].elements = number_of_elements;
        } else if (first_string[0] == IMEX_WORD){
            index = boost::lexical_cast<uint16_t>(second_fields[0]);
            mSOPImportData[index].first = first_string;
            mSOPImportData[index].second = second_string;
            mSOPImportData[index].position = -1;
            mSOPImportData[index].type = IMEX_WORD;
            mSOPImportData[index].table_entry = mRes.getTableEntry(index, 4);
            mSOPImportData[index].elements = number_of_elements;
        }
        std::cout << " DEBUG: " << mSOPImportData[index].table_entry << std::endl;
    }

}

AESOP::SOP::~SOP() {
    //cleanup
}

uint32_t AESOP::SOP::getPC() {
    return mPC;
}

AESOP::SOPScriptHeader &AESOP::SOP::getSOPHeader(){
    return mSOPHeader;
}