#include <algorithm>
#include <iostream>

#include "resample_img.h"
#include "../../src/data_structures/meshclass.h"
#include "../../src/io/readimage.h"

#include "../../src/algorithm/gradient.hpp"
#include "../../src/data_structures/particle_map.hpp"
#include "../../src/data_structures/Tree/PartCellBase.hpp"
#include "../../src/data_structures/Tree/PartCellStructure.hpp"
#include "../../src/algorithm/level.hpp"
#include "../../src/io/writeimage.h"
#include "../../src/io/write_parts.h"
#include "../../src/io/partcell_io.h"
#include "../../src/data_structures/Tree/PartCellParent.hpp"
#include "../utils.h"

bool command_option_exists(char **begin, char **end, const std::string &option)
{
    return std::find(begin, end, option) != end;
}

char* get_command_option(char **begin, char **end, const std::string &option)
{
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

cmdLineOptions read_command_line_options(int argc, char **argv, Part_rep& part_rep){
    
    cmdLineOptions result;
    
    if(argc == 1) {
        std::cerr << "Usage: \"pipeline -i inputfile -d directory [-t] [-o outputfile]\"" << std::endl;
        exit(1);
    }
    
    if(command_option_exists(argv, argv + argc, "-i"))
    {
        result.input = std::string(get_command_option(argv, argv + argc, "-i"));
    } else {
        std::cout << "Input file required" << std::endl;
        exit(2);
    }
    
    if(command_option_exists(argv, argv + argc, "-d"))
    {
        result.directory = std::string(get_command_option(argv, argv + argc, "-d"));
    }
    
    if(command_option_exists(argv, argv + argc, "-o"))
    {
        result.output = std::string(get_command_option(argv, argv + argc, "-o"));
    }
    
    if(command_option_exists(argv, argv + argc, "-t"))
    {
        part_rep.timer.verbose_flag = true;
    }
    
    return result;
    
}


int main(int argc, char **argv) {
    
    Part_rep part_rep;
    
    // INPUT PARSING
    
    cmdLineOptions options = read_command_line_options(argc, argv, part_rep);
    
    // COMPUTATIONS
    PartCellStructure<float,uint64_t> pc_struct;
    
    //output
    std::string file_name = options.directory + options.input;
    
    read_apr_pc_struct(pc_struct,file_name);
    
    int num_cells = pc_struct.get_number_cells();
    int num_parts = pc_struct.get_number_parts();
    
    std::cout << "Number cells: " << num_cells << std::endl;
    std::cout << "Number parts: " << num_parts << std::endl;
    
    Mesh_data<uint64_t> interp;
    
    Part_timer timer;
    
    timer.verbose_flag = true;
    
    timer.start_timer("interp to pc");
    
    pc_struct.interp_parts_to_pc(interp,pc_struct.part_data.particle_data);
   
    timer.stop_timer();
    
    debug_write(interp,"interp_out");
    
}


