//////////////////////////////////////////////////////
///
/// Bevan Cheeseman 2018
///
/// Examples of simple iteration an access to Particle Cell, and particle information. (See Example_neigh, for neighbor access)
///
/// Usage:
///
/// (using output of Example_get_apr)
///
/// Example_apr_iterate -i input_image_tiff -d input_directory
///
/////////////////////////////////////////////////////

#include <algorithm>
#include <iostream>

#include "benchmarks/development/Example_newstructures.h"





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

cmdLineOptions read_command_line_options(int argc, char **argv){

    cmdLineOptions result;

    if(argc == 1) {
        std::cerr << "Usage: \"Example_apr_iterate -i input_apr_file -d directory\"" << std::endl;
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

    return result;

}
template<typename T>
void compare_two_maps(APR<T>& apr,APRAccess& aa1,APRAccess& aa2){


    uint64_t z_;
    uint64_t x_;

    //first compare the size

    for(uint64_t i = (apr.level_min());i <= apr.level_max();i++) {

        const unsigned int x_num_ = aa1.x_num[i];
        const unsigned int z_num_ = aa1.z_num[i];
        const unsigned int y_num_ = aa1.y_num[i];

        for (z_ = 0; z_ < z_num_; z_++) {
            for (x_ = 0; x_ < x_num_; x_++) {
                const uint64_t offset_pc_data = x_num_ * z_ + x_;


                if(aa1.gap_map.data[i][offset_pc_data].size()!=aa2.gap_map.data[i][offset_pc_data].size()) {
                    std::cout << "number of maps size mismatch" << std::endl;
                }

                if(aa1.gap_map.data[i][offset_pc_data].size()>0){
                    if(aa1.gap_map.data[i][offset_pc_data][0].map.size()!=aa2.gap_map.data[i][offset_pc_data][0].map.size()) {
                        std::cout << "number of gaps size mismatch" << std::endl;
                    }
                }


                if(aa1.gap_map.data[i][offset_pc_data].size()>0) {

                    std::vector<uint16_t> y_begin;
                    std::vector<uint16_t> y_end;
                    std::vector<uint64_t> global_index;

                    for (auto const &element : aa1.gap_map.data[i][offset_pc_data][0].map) {
                        y_begin.push_back(element.first);
                        y_end.push_back(element.second.y_end);
                        global_index.push_back(element.second.global_index_begin);
                    }

                    std::vector<uint16_t> y_begin2;
                    std::vector<uint16_t> y_end2;
                    std::vector<uint64_t> global_index2;

                    for (auto const &element : aa2.gap_map.data[i][offset_pc_data][0].map) {
                        y_begin2.push_back(element.first);
                        y_end2.push_back(element.second.y_end);
                        global_index2.push_back(element.second.global_index_begin);
                    }

                    for (int j = 0; j < y_begin.size(); ++j) {

                        if(y_begin[j]!=y_begin2[j]){
                            std::cout << "ybegin broke" << std::endl;
                        }

                        if(y_end[j]!=y_end2[j]){
                            std::cout << "ybegin broke" << std::endl;
                        }

                        if(global_index[j]!=global_index2[j]){
                            std::cout << "index broke" << std::endl;
                        }

                    }


                }

            }
        }
    }






}



int main(int argc, char **argv) {

    // INPUT PARSING

    cmdLineOptions options = read_command_line_options(argc, argv);

    // Filename
    std::string file_name = options.directory + options.input;

    // Read the apr file into the part cell structure
    APRTimer timer;

    timer.verbose_flag = true;

    // APR datastructure
    APR<uint16_t> apr;

    //read file
    apr.read_apr(file_name);

    apr.parameters.input_dir = options.directory;

    std::string name = options.input;
    //remove the file extension
    name.erase(name.end()-3,name.end());

   // APRAccess apr_access;


    //just run old code and initialize it there
//    apr_access.test_method(apr);

    /////
    //
    //  Now new data-structures
    //
    /////

    APRAccess apr_access2;
    std::vector<std::vector<uint8_t>> p_map;

    timer.start_timer("generate pmap");
    apr_access2.generate_pmap(apr,p_map);
    timer.stop_timer();

    timer.start_timer("generate map structure");
    apr_access2.initialize_structure_from_particle_cell_tree(apr,p_map);
    timer.stop_timer();

    APRIteratorNew<uint16_t> apr_it(apr_access2);

    ExtraParticleData<uint16_t> particles_int;
    ExtraParticleData<uint16_t> x;
    ExtraParticleData<uint16_t> y;
    ExtraParticleData<uint16_t> z;
    ExtraParticleData<uint16_t> level;

    for (apr.begin(); apr.end()!=0;apr.it_forward()) {
        particles_int.data.push_back(apr(apr.particles_int));
        x.data.push_back(apr.x());
        y.data.push_back(apr.y());
        z.data.push_back(apr.z());
        level.data.push_back(apr.level());
    }


    uint64_t counter = 0;

    for (apr_it.it_begin();apr_it.it_end(); apr_it.it_forward()) {
        counter++;

        if(apr_it.y() != apr_it(y)){
            std::cout << "broken y" << std::endl;
        }

    }

    std::cout << counter << std::endl;


//    MapStorageData map_data;
//
//    compare_two_maps(apr,apr_access,apr_access2);
//
//    timer.start_timer("flatten");
//    apr_access2.flatten_structure(apr,map_data);
//    timer.stop_timer();
//
//    std::cout << apr_access2.total_number_parts << std::endl;
//    std::cout << apr_access2.total_number_gaps << std::endl;
//    std::cout << apr_access2.total_number_non_empty_rows << std::endl;
//
//    APRAccess apr_access3;
//
//    apr_access3.total_number_non_empty_rows = apr_access2.total_number_non_empty_rows;
//    apr_access3.total_number_gaps = apr_access2.total_number_gaps;
//    apr_access3.total_number_parts = apr_access2.total_number_parts;
//
//    apr_access3.x_num = apr_access2.x_num;
//    apr_access3.y_num = apr_access2.y_num;
//    apr_access3.z_num = apr_access2.z_num;
//
//    apr_access3.rebuild_map(apr,map_data);
//
//    compare_two_maps(apr,apr_access,apr_access3);

}

