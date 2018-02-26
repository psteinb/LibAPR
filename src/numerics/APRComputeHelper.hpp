//
// Created by cheesema on 23.02.18.
//

#ifndef APR_TIME_APRCOMPUTEHELPER_HPP
#define APR_TIME_APRCOMPUTEHELPER_HPP

#include <src/data_structures/APR/APRTree.hpp>
#include <src/numerics/APRTreeNumerics.hpp>
#include "APRNumerics.hpp"
#include <src/algorithm/APRConverter.hpp>

template<typename ImageType>
class APRComputeHelper {
public:


    APRComputeHelper(APR<ImageType>& apr){
        apr_tree.init(apr);
    }

    APRTree<ImageType> apr_tree;
    ExtraParticleData<ImageType> adaptive_max;
    ExtraParticleData<ImageType> adaptive_min;

    template<typename T>
    void compute_local_scale(APR<ImageType>& apr,ExtraParticleData<T>& local_intensity_scale,unsigned int smooth_iterations = 3){

        APRTimer timer;
        timer.verbose_flag = true;

        timer.start_timer("smooth");

        APRNumerics aprNumerics;
        ExtraParticleData<uint16_t> smooth(apr);
        std::vector<float> filter = {0.1f, 0.8f, 0.1f}; // << Feel free to play with these
        aprNumerics.seperable_smooth_filter(apr, apr.particles_intensities, smooth, filter, smooth_iterations);

        //weight_neighbours(apr,apr.particles_intensities,smooth,0.8);

        timer.stop_timer();

        unsigned int smoothing_steps_local = 3;

        timer.start_timer("adaptive min");

        APRTreeNumerics::calculate_adaptive_min(apr,apr_tree,smooth,adaptive_min,smoothing_steps_local);

        timer.stop_timer();

        timer.start_timer("adaptive max");
        APRTreeNumerics::calculate_adaptive_max(apr,apr_tree,smooth,adaptive_max,smoothing_steps_local);

        timer.stop_timer();

        local_intensity_scale.init(apr);
        adaptive_max.zip(apr,adaptive_min,local_intensity_scale, [](const uint16_t &a, const uint16_t &b) { return abs(a-b); });

    }


    void compute_apr_edge_energy(){

    }


    void compute_apr_interior_energy(){

    }

    void compute_local_scale_smooth_propogate(APR<ImageType>& apr,MeshData<ImageType>& input_image,ExtraParticleData<ImageType>& local_intensity_scale){

        APRConverter<ImageType> aprConverter;
        unsigned int smooth_factor = 15;

        MeshData<float> local_scale_temp;

        MeshData<float> local_scale_temp2;

        downsample(input_image, local_scale_temp,
                   [](const float &x, const float &y) -> float { return x + y; },
                   [](const float &x) -> float { return x / 8.0; },true);

        local_scale_temp2.init(local_scale_temp);

        aprConverter.get_local_intensity_scale(local_scale_temp,local_scale_temp2);

        APRTreeIterator<uint16_t> aprTreeIterator(apr_tree);
        uint64_t parent_number;

        local_intensity_scale.init(apr);

        ExtraParticleData<uint16_t> local_intensity_scale_tree(apr_tree);

        for (parent_number = aprTreeIterator.particles_level_begin(aprTreeIterator.level_max());
             parent_number < aprTreeIterator.particles_level_end(aprTreeIterator.level_max()); ++parent_number) {

            aprTreeIterator.set_iterator_to_particle_by_number(parent_number);

            local_intensity_scale_tree[aprTreeIterator] = (ImageType)local_scale_temp.at(aprTreeIterator.y_nearest_pixel(),aprTreeIterator.x_nearest_pixel(),aprTreeIterator.z_nearest_pixel());

        }

        for (int i = 0; i < smooth_factor; ++i) {
            //smooth step
            for (unsigned int level = (aprTreeIterator.level_max());
                 level >= aprTreeIterator.level_min(); --level) {
                for (parent_number = aprTreeIterator.particles_level_begin(level);
                     parent_number < aprTreeIterator.particles_level_end(level); ++parent_number) {

                    aprTreeIterator.set_iterator_to_particle_by_number(parent_number);

                    float temp = local_intensity_scale_tree[aprTreeIterator];
                    float counter = 1;

                    for (int direction = 0; direction < 6; ++direction) {
                        // Neighbour Particle Cell Face definitions [+y,-y,+x,-x,+z,-z] =  [0,1,2,3,4,5]
                        if (aprTreeIterator.find_neighbours_same_level(direction)) {

                            if (aprTreeIterator.set_neighbour_iterator(aprTreeIterator, direction, 0)) {

                                if (local_intensity_scale_tree[aprTreeIterator] > 0) {
                                    temp += local_intensity_scale_tree[aprTreeIterator];
                                    counter++;
                                }
                            }
                        }
                    }

                    local_intensity_scale_tree[aprTreeIterator] = temp / counter;

                }
            }
        }

        uint64_t particle_number;
        APRIterator<uint16_t> apr_iterator(apr);

        APRTreeIterator<uint16_t> parent_iterator(apr_tree);

        //Now set the highest level particle cells.
        for (particle_number = apr_iterator.particles_level_begin(apr_iterator.level_max());
             particle_number <
             apr_iterator.particles_level_end(apr_iterator.level_max()); ++particle_number) {
            //This step is required for all loops to set the iterator by the particle number
            apr_iterator.set_iterator_to_particle_by_number(particle_number);

            parent_iterator.set_iterator_to_parent(apr_iterator);
            local_intensity_scale[apr_iterator] = local_intensity_scale_tree[parent_iterator];

        }


        APRTreeIterator<uint16_t> neighbour_tree_iterator(apr_tree);


        ExtraParticleData<uint16_t> boundary_type(apr);

        //spread solution

        for (particle_number = apr_iterator.particles_level_begin(apr_iterator.level_max() - 1);
             particle_number <
             apr_iterator.particles_level_end(apr_iterator.level_max() - 1); ++particle_number) {
            //This step is required for all loops to set the iterator by the particle number
            apr_iterator.set_iterator_to_particle_by_number(particle_number);

            //now we only update the neighbours, and directly access them through a neighbour iterator

            if (apr_iterator.type() == 2) {

                float temp = 0;
                float counter = 0;

                float counter_neigh = 0;

                aprTreeIterator.set_particle_cell_no_search(apr_iterator);

                //loop over all the neighbours and set the neighbour iterator to it
                for (int direction = 0; direction < 6; ++direction) {
                    // Neighbour aprTreeIterator Cell Face definitions [+y,-y,+x,-x,+z,-z] =  [0,1,2,3,4,5]
                    if (aprTreeIterator.find_neighbours_same_level(direction)) {

                        if (neighbour_tree_iterator.set_neighbour_iterator(aprTreeIterator, direction, 0)) {
                            temp += local_intensity_scale_tree[neighbour_tree_iterator];
                            counter++;

                        }
                    }
                }
                if(counter>0) {
                    local_intensity_scale[apr_iterator] = temp/counter;
                    boundary_type[apr_iterator] = apr_iterator.level_max();
                }

            }
        }

        APRIterator<uint16_t> neigh_iterator(apr);


        for (int level = (apr_iterator.level_max()-1); level >= apr_iterator.level_min() ; --level) {

            bool still_empty = true;
            while(still_empty) {
                still_empty = false;
                for (particle_number = apr_iterator.particles_level_begin(level);
                     particle_number <
                     apr_iterator.particles_level_end(level); ++particle_number) {
                    //This step is required for all loops to set the iterator by the particle number
                    apr_iterator.set_iterator_to_particle_by_number(particle_number);

                    if (local_intensity_scale[apr_iterator] == 0) {

                        float counter = 0;
                        float temp = 0;

                        //loop over all the neighbours and set the neighbour iterator to it
                        for (int direction = 0; direction < 6; ++direction) {
                            // Neighbour Particle Cell Face definitions [+y,-y,+x,-x,+z,-z] =  [0,1,2,3,4,5]
                            if (apr_iterator.find_neighbours_in_direction(direction)) {

                                if (neigh_iterator.set_neighbour_iterator(apr_iterator, direction, 0)) {

                                    if(boundary_type[neigh_iterator]>=(level+1)) {
                                        counter++;
                                        temp += local_intensity_scale[neigh_iterator];
                                    }
                                }
                            }
                        }

                        if (counter > 0) {
                            local_intensity_scale[apr_iterator] = temp / counter;
                            boundary_type[apr_iterator] = level;
                        } else {
                            still_empty = true;
                        }
                    } else {
                        boundary_type[apr_iterator]=level+1;
                    }
                }
            }
        }


    }





};


#endif //APR_TIME_APRCOMPUTEHELPER_HPP
