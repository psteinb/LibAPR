///////////////////////////////////////////
//
//  ImageGen 2016
//
//  Bevan Cheeseman 2016
//
//  Header file with some specific hdf5 functions
//
//////////////////////////////////////////

#ifndef PARTPLAY_HDF5FUNCTIONS_BLOSC_H
#define PARTPLAY_HDF5FUNCTIONS_BLOSC_H

extern "C" {
	#include "blosc_filter.h"
}
#include "hdf5.h"
#include <string>
#include <algorithm>
#include <iostream>
#include <vector>
#include <fstream>


void hdf5_register_blosc();
hid_t hdf5_create_file_blosc(std::string file_name);
void hdf5_load_data_blosc(hid_t obj_id, void* buff, const char* data_name);
void hdf5_load_data_blosc(hid_t obj_id, hid_t dataType, void* buff, const char* data_name);
void hdf5_write_attribute_blosc(hid_t obj_id,hid_t type_id,const char* attr_name,hsize_t rank,hsize_t* dims, const void * const data );
void hdf5_write_data_blosc(hid_t obj_id,hid_t type_id,const char* ds_name,hsize_t rank,hsize_t* dims, void* data ,unsigned int comp_type,unsigned int comp_level,unsigned int shuffle);
void write_main_paraview_xdmf_xml(std::string save_loc,std::string file_name,uint64_t num_parts);


#endif