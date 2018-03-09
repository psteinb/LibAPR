#include <algorithm>
#include <vector>
#include <array>
#include <iostream>
#include <cassert>
#include <limits>
#include <chrono>
#include <iomanip>

#include "data_structures/APR/APR.hpp"
#include "data_structures/APR/APRTreeIterator.hpp"
#include "data_structures/APR/ExtraParticleData.hpp"

template <typename T>
T* onto_device(const std::vector<T>& _container){

    T* value = nullptr;
    auto bytes = _container.size()*sizeof(T);
    cudaMalloc(&value, bytes );
    auto err = cudaGetLastError();
    if(err!=cudaSuccess){
        std::cerr << "[onto_device std::vector] memory allocation failed ("<< bytes <<" Bytes)!\n";
        return nullptr;
    }

    cudaMemcpy(value, _container.data(), bytes ,cudaMemcpyHostToDevice );
    err = cudaGetLastError();
    if(err!=cudaSuccess){
        std::cerr << "[onto_device std::vector] memory transfer failed!\n";
        return nullptr;
    }

    return value;
}

template <typename T>
T* onto_device(std::size_t n_elements, T init = 0){

    T* value = nullptr;
    auto bytes = n_elements*sizeof(T);
    cudaMalloc(&value, bytes );
    auto err = cudaGetLastError();
    if(err!=cudaSuccess){
        std::cerr << "[onto_device] memory allocation failed ("<< bytes <<" Bytes)!\n";
        return nullptr;
    }
    cudaMemset(value, 0, n_elements*sizeof(T) );
    return value;
}

struct cmdLineOptions{
    std::string output = "output";
    std::string stats = "";
    std::string directory = "";
    std::string input = "";
};

bool command_option_exists(char **begin, char **end, const std::string &option) {
    return std::find(begin, end, option) != end;
}

char* get_command_option(char **begin, char **end, const std::string &option) {
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end) {
        return *itr;
    }
    return 0;
}

cmdLineOptions read_command_line_options(int argc, char **argv) {
    cmdLineOptions result;

    if(argc == 1) {
        std::cerr << "Usage: \"Example_apr_neighbour_access -i input_apr_file -d directory\"" << std::endl;
        exit(1);
    }
    if(command_option_exists(argv, argv + argc, "-i")) {
        result.input = std::string(get_command_option(argv, argv + argc, "-i"));
    } else {
        std::cout << "Input file required" << std::endl;
        exit(2);
    }
    if(command_option_exists(argv, argv + argc, "-d")) {
        result.directory = std::string(get_command_option(argv, argv + argc, "-d"));
    }
    if(command_option_exists(argv, argv + argc, "-o")) {
        result.output = std::string(get_command_option(argv, argv + argc, "-o"));
    }

    return result;
}

__global__ void one_line(const std::uint16_t* _pdata,
                         const std::uint16_t* _y_ex,
                         std::uint16_t*       _temp_vec,
                         std::size_t          _tindex,
                         std::size_t          _global_offset
    ){


    std::size_t global_index = _global_offset + (blockDim.x*blockIdx.x + threadIdx.x);
    uint16_t current_particle_value = _pdata[global_index];
    auto y = _y_ex[global_index];
    _temp_vec[_tindex+y] = current_particle_value;

}

__global__ void nothing(){

    std::size_t index = (blockDim.x*blockIdx.x + threadIdx.x);
    if(threadIdx.x == 0){
        printf("++ nothing ++");}

}


__global__ void addone(std::uint16_t*           _pdata, std::size_t len){

    std::size_t index = (blockDim.x*blockIdx.x + threadIdx.x);
    if(index < len)
        _pdata[index]++;

}

__global__ void insert(
    std::size_t _level,
    std::size_t _z_index,
    const ulong2* _line_offsets,
    const std::uint16_t*           _y_ex,
    const std::uint16_t*           _pdata,
    const std::size_t*             _offsets,
    std::size_t                    _max_y,
    std::size_t                    _max_x,
    std::size_t                    _nparticles,
    std::uint16_t*                 _temp_vec,
    std::size_t                    _stencil_size
    ){

    unsigned int x_index = blockDim.x * blockIdx.x + threadIdx.x;
    printf("[insert] hello");

    if(x_index >= _max_x){
        return; // out of bounds
    }

    auto level_zx_offset = _offsets[_level] + _max_x * _z_index + x_index;
    auto row_start = _line_offsets[level_zx_offset];

    if(row_start.y == 0)
        return;

    auto particle_index_begin = row_start.x;
    auto particle_index_end   = row_start.y;

    auto t_index = x_index*_max_y + ((_z_index % _stencil_size)*_max_y*_max_x) ;

    const int threads = 32;
    const int blocks = ((particle_index_end - particle_index_begin) + threads - 1)/threads;
//printf("[insert] threads %i blocks %i particles %i",threads,blocks,(particle_index_end - particle_index_begin));

    one_line<<<blocks,threads>>>(_pdata, _y_ex, _temp_vec, t_index,
                                 particle_index_begin);



}

__global__ void push_back(
    std::size_t _level,
    std::size_t _z_index,
    const ulong2* _line_offsets,
    const std::uint16_t*           _y_ex,
    const std::uint16_t*           _temp_vec,
    const std::size_t*             _offsets,
    std::size_t                    _max_y,
    std::size_t                    _max_x,
    std::size_t                    _nparticles,
    std::uint16_t*                 _pdata,
    std::size_t                    _stencil_size
    ){

    unsigned int x_index = blockDim.x * blockIdx.x + threadIdx.x;

    if(x_index >= _max_x){
        return; // out of bounds
    }

    auto level_zx_offset = _offsets[_level] + _max_x * _z_index + x_index;
    auto row_start = _line_offsets[level_zx_offset];

    if(row_start.y == 0)
        return;

    auto particle_index_begin = row_start.x;
    auto particle_index_end   = row_start.y;

    auto t_index = x_index*_max_y + ((_z_index % _stencil_size)*_max_y*_max_x) ;

    for (std::size_t global_index = particle_index_begin;
         global_index <= particle_index_end; ++global_index) {

        auto y = _y_ex[global_index];
        _pdata[global_index] = _temp_vec[t_index+y];


    }


}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
    // Read provided APR file
    cmdLineOptions options = read_command_line_options(argc, argv);
    const int reps = 1;

    std::string fileName = options.directory + options.input;
    APR<uint16_t> apr;
    apr.read_apr(fileName);

    std::cout << "loaded " << fileName << "\n";
    // Get dense representation of APR
    APRIterator<uint16_t> aprIt(apr);

    ///////////////////////////
    ///
    /// Sparse Data for GPU
    ///
    ///////////////////////////

    std::vector<std::tuple<std::size_t,std::size_t>> level_zx_index_start;//size = number of rows on all levels
    std::vector<std::uint16_t> y_explicit;y_explicit.reserve(aprIt.total_number_particles());//size = number of particles
    std::vector<std::uint16_t> particle_values;particle_values.reserve(aprIt.total_number_particles());//size = number of particles
    std::vector<std::size_t> level_offset(aprIt.level_max()+1,UINT64_MAX);//size = number of levels

    std::size_t x = 0;
    std::size_t z = 0;

    std::size_t zx_counter = 0;


    for (int level = aprIt.level_min(); level <= aprIt.level_max(); ++level) {
        level_offset[level] = zx_counter;

        for (z = 0; z < aprIt.spatial_index_z_max(level); ++z) {
            for (x = 0; x < aprIt.spatial_index_x_max(level); ++x) {

                zx_counter++;
                if (aprIt.set_new_lzx(level, z, x) < UINT64_MAX) {
                    level_zx_index_start.emplace_back(std::make_tuple<std::size_t,std::size_t>(aprIt.global_index(),
                                                                                               aprIt.particles_zx_end(level,z,x)-1)); //This stores the begining and end global index for each level_xz_row
                } else {
                    level_zx_index_start.emplace_back(std::make_tuple<std::size_t,std::size_t>(UINT64_MAX, 0)); //This stores the begining and end global index for each level_
                }

                for (aprIt.set_new_lzx(level, z, x);
                     aprIt.global_index() < aprIt.particles_zx_end(level, z,
                                                                   x); aprIt.set_iterator_to_particle_next_particle()) {
                    y_explicit.emplace_back(aprIt.y());
                    particle_values.emplace_back(apr.particles_intensities[aprIt]);

                }
            }

        }
    }
    std::cout << "data setup on CPU\n";


    ////////////////////
    ///
    /// Example of doing our level,z,x access using the GPU data structure
    ///
    /////////////////////
    auto start = std::chrono::high_resolution_clock::now();

    std::cout << "[cuda] transforming tuples on host\n";

    std::vector<ulong2> h_level_zx_index_start(level_zx_index_start.size());
    std::transform(level_zx_index_start.begin(), level_zx_index_start.end(),
                      h_level_zx_index_start.begin(),
                      [] ( const auto& _el ){
                          return make_ulong2(std::get<0>(_el), std::get<1>(_el));
                      } );

    std::cout << "[cuda] setting up device vectors\n";

    auto d_level_zx_index_start = onto_device(h_level_zx_index_start);
    auto d_y_explicit = onto_device(y_explicit);
    auto d_level_offset = onto_device(level_offset);
    std::uint16_t* d_temp_vec = nullptr;
    auto d_particle_values = onto_device(particle_values);
    auto d_test_access_data = onto_device<std::uint16_t>(particle_values.size(),0);

    std::size_t max_elements = 0;
    const int stencil_size =5;

    //this for-loop needs to leave at some point
    for (int level = aprIt.level_min(); level <= aprIt.level_max(); ++level) {
        auto xtimesy = aprIt.spatial_index_y_max(level) + (stencil_size - 1);
        xtimesy *= aprIt.spatial_index_x_max(level) + (stencil_size - 1);
        if(max_elements < xtimesy)
            max_elements = xtimesy;
    }


    d_temp_vec = onto_device<std::uint16_t>(max_elements*stencil_size,0);
    std::cout << "[cuda] finished setting up temp_vec\n";

    if(cudaGetLastError()!=cudaSuccess){
        std::cerr << "memory transfers failed!\n";

    }
    auto end_gpu_tx = std::chrono::high_resolution_clock::now();
    cudaDeviceSynchronize();

    nothing<<<2,32>>>();

    std::cout << "[cuda] running kernels\n";

    for ( int r = 0;r<reps;++r){

        auto start_gpu_kernel = std::chrono::high_resolution_clock::now();

        for (int lvl = aprIt.level_min(); lvl <= aprIt.level_max(); ++lvl) {

            const int y_num = aprIt.spatial_index_y_max(lvl);
            const int x_num = aprIt.spatial_index_x_max(lvl);
            const int z_num = aprIt.spatial_index_z_max(lvl);

            dim3 threads(32);
            dim3 blocks((x_num + threads.x- 1)/threads.x
                );

            nothing<<<2,32>>>();

            for(int z = 0;z<z_num;++z){

                std::cout << "["<< lvl <<"] " << y_num << ", " << x_num << ", " << z <<"/" << z_num << ": "
                          << "bxt = " << blocks.x << " x " << threads.x << "\n";

                //addone<<<blocks,threads>>>(d_temp_vec,max_elements*stencil_size);
                insert<<<blocks,threads>>>(lvl,
                                           z,
                                           d_level_zx_index_start,
                                           d_y_explicit,
                                           d_particle_values,
                                           d_level_offset,
                                           y_num,x_num,
                                           particle_values.size(),
                                           d_temp_vec,
                                           stencil_size);

                auto err = cudaGetLastError();
                if(err!=cudaSuccess){
                    std::cerr << "on " << lvl << " [z="<< z << "] the insert cuda kernel does not run ("
                              << cudaGetErrorString(err)
                              << ")!\n";
                    break;
                }

                cudaDeviceSynchronize();

                // push_back<<<blocks,threads>>>(lvl,
                //                               z,
                //                               d_level_zx_index_start,
                //                               d_y_explicit,
                //                               d_temp_vec,
                //                               d_level_offset,
                //                               y_num,x_num,
                //                               particle_values.size(),
                //                               d_test_access_data,
                //                               stencil_size);

                // if(cudaGetLastError()!=cudaSuccess){
                //     std::cerr << "on " << lvl << " the insert cuda kernel does not run!\n";
                //     break;
                // }

            }
        }
        cudaDeviceSynchronize();
        auto end_gpu_kernel = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double,std::milli> rep_diff = end_gpu_kernel - start_gpu_kernel;
        std::cout << std::setw(3) << r << " GPU:      " << rep_diff  .count() << " ms\n";

    }

    auto end_gpu_kernels = std::chrono::high_resolution_clock::now();

    std::vector<std::uint16_t> test_access_data(particle_values.size(),std::numeric_limits<std::uint16_t>::max());
    cudaMemcpy(test_access_data.data(),d_test_access_data,particle_values.size()*sizeof(std::uint16_t),cudaMemcpyDeviceToHost);


    auto end_gpu = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double,std::milli> gpu_tx_up = end_gpu_tx - start;
    std::chrono::duration<double,std::milli> gpu_tx_down = end_gpu - end_gpu_kernels;

    std::cout << "   GPU: up   " << gpu_tx_up  .count() << " ms\n";
    std::cout << "   GPU: down " << gpu_tx_down.count() << " ms\n";

    assert(test_access_data.back() != std::numeric_limits<std::uint16_t>::max());

    cudaFree(d_level_zx_index_start );
    cudaFree(d_y_explicit );
    cudaFree(d_level_offset );
    cudaFree(d_temp_vec );
    cudaFree(d_particle_values );
    cudaFree(d_test_access_data );

    //////////////////////////
    ///
    /// Now check the data
    ///
    ////////////////////////////

    bool success = true;

    for (std::size_t i = 0; i < test_access_data.size(); ++i) {
        if(apr.particles_intensities.data[i]!=test_access_data[i]){
            success = false;
            std::cout << i << " expected: " << apr.particles_intensities.data[i] << ", received: " << test_access_data[i] << "\n";
            break;
        }
    }

    if(success){
        std::cout << "PASS" << std::endl;
    } else {
        std::cout << "FAIL" << std::endl;
    }


}