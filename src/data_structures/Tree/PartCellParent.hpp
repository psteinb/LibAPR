///////////////////
//
//  Bevan Cheeseman 2016
//
//  Parent Child Cell Access Structure for Parent (Ghost) nodes (Can be used for doing operations that use the APR as an octtree)
//
///////////////

#ifndef PARTPLAY_PARENT_HPP
#define PARTPLAY_PARENT_HPP

#include "PartCellNeigh.hpp"
#include "PartCellData.hpp"

// Parent / Child Relation Nodes (parent_info)
#define PARENT_MASK ((((uint64_t)1) << 12) - 1) << 0
#define PARENT_SHIFT 0

// Next child can be accessed by a simple yshift ++ on the node
#define CHILD1_MASK ((((uint64_t)1) << 13) - 1) << 12 // y0,x0,z0
#define CHILD1_SHIFT 12

#define CHILD2_MASK ((((uint64_t)1) << 13) - 1) << 25 // y0,x1,z0
#define CHILD2_SHIFT 25

#define CHILD3_MASK ((((uint64_t)1) << 13) - 1) << 38 // y0,x0,z1
#define CHILD3_SHIFT 38

#define CHILD4_MASK ((((uint64_t)1) << 13) - 1) << 51 // y0,x1,z1
#define CHILD4_SHIFT 51

// CHILD STATUS (To be used with the regular part data access strategy) (neigh_info)

#define REAL_CHILDREN 2
#define GHOST_CHILDREN 1

// type T data structure base type
template <typename T>
class PartCellParent {
    
public:
    
    T get_parent_j(const T& node_val_parent_info){
        //returns the parent j
        return ((node_val_parent_info & PARENT_MASK) >> PARENT_SHIFT);
    }
    
    T get_parent_key(const T& node_val_parent_info,const T& curr_key){
        //returns the parent access key
        
        T parent_key = 0;
        if ((parent_info.pc_key_get_depth(curr_key)-1) > parent_info.depth_min){
            parent_info.pc_key_set_x(parent_key,parent_info.pc_key_get_x(curr_key)/2);
            parent_info.pc_key_set_z(parent_key,parent_info.pc_key_get_z(curr_key)/2);
            parent_info.pc_key_set_depth(parent_key,parent_info.pc_key_get_depth(curr_key)-1);
            parent_info.pc_key_set_j(parent_key,get_parent_j(node_val_parent_info));
        }
        return parent_key;
    }
    
    void get_children_keys(const T& node_val_parent_info,const T& curr_key,std::vector<T>& children_keys){
        //
        //  Returns the children keys from the structure
        //
        //  Ordering follows particle ordering ((0,0,0),(1,0,0),(0,1,0),(1,1,0),..) y,x then z
        //
        children_keys.resize(8,0);
        
        T child_x = parent_info.pc_key_get_x(curr_key)*2;
        T child_z = parent_info.pc_key_get_z(curr_key)*2;
        T child_depth = parent_info.pc_key_get_depth(curr_key)+1;
        
        //loop over and set variables
        for(int p = 0; p < 8;p++){
            parent_info.pc_key_set_x(children_keys[p],child_x + parent_info.seed_part_x[p]);
            parent_info.pc_key_set_z(children_keys[p],child_z + parent_info.seed_part_z[p]);
            parent_info.pc_key_set_depth(children_keys[p],child_depth);
        }
        
        parent_info.pc_key_set_j(children_keys[0],((node_val_parent_info & CHILD1_MASK) >> CHILD1_SHIFT));
        parent_info.pc_key_set_j(children_keys[1],((node_val_parent_info & CHILD1_MASK) >> CHILD1_SHIFT)+1);
        
        parent_info.pc_key_set_j(children_keys[2],((node_val_parent_info & CHILD2_MASK) >> CHILD2_SHIFT));
        parent_info.pc_key_set_j(children_keys[3],((node_val_parent_info & CHILD2_MASK) >> CHILD2_SHIFT)+1);
        
        parent_info.pc_key_set_j(children_keys[4],((node_val_parent_info & CHILD3_MASK) >> CHILD3_SHIFT));
        parent_info.pc_key_set_j(children_keys[5],((node_val_parent_info & CHILD3_MASK) >> CHILD3_SHIFT)+1);
        
        parent_info.pc_key_set_j(children_keys[6],((node_val_parent_info & CHILD4_MASK) >> CHILD4_SHIFT));
        parent_info.pc_key_set_j(children_keys[7],((node_val_parent_info & CHILD4_MASK) >> CHILD4_SHIFT)+1);
        
        
    }
    
    
    void get_neighs_parent_all(const uint64_t& curr_key,uint64_t node_val,PartCellNeigh<uint64_t>& neigh_keys){
        // Selects the neighbour in the correct direction
        
        neigh_keys.curr = curr_key;
        
        neigh_keys.neigh_face[0].resize(0);
        neigh_keys.neigh_face[1].resize(0);
        neigh_keys.neigh_face[2].resize(0);
        neigh_keys.neigh_face[3].resize(0);
        neigh_keys.neigh_face[4].resize(0);
        neigh_keys.neigh_face[5].resize(0);
        
        get_neighs_face_parent_t<0>(curr_key,node_val,neigh_keys.neigh_face[0]);
        get_neighs_face_parent_t<1>(curr_key,node_val,neigh_keys.neigh_face[1]);
        get_neighs_face_parent_t<2>(curr_key,node_val,neigh_keys.neigh_face[2]);
        get_neighs_face_parent_t<3>(curr_key,node_val,neigh_keys.neigh_face[3]);
        get_neighs_face_parent_t<4>(curr_key,node_val,neigh_keys.neigh_face[4]);
        get_neighs_face_parent_t<5>(curr_key,node_val,neigh_keys.neigh_face[5]);
        
    }
    
    
    PartCellParent(){};
    
    template<typename S>
    PartCellParent(PartCellStructure<S,T>& pc_struct){
        initialize_parents(pc_struct);
    };
    
    PartCellData<T> neigh_info;
    PartCellData<T> parent_info;
    
    
private:
    //counter for number of parent cells, should be set on intialization
    uint64_t number_parent_cells;
    
    void set_parent_relationships(){
        //
        //  Sets the parent relationships
        //
        //
        //
        
        Part_timer timer;
        timer.verbose_flag = true;
        uint64_t z_;
        uint64_t x_;
        uint64_t j_;
        
        timer.start_timer("Get parent child");
        
        unsigned int y_parent;
        uint64_t j_parent;
       
        
        uint64_t node_val;
        uint64_t y_coord;
        
        
        for(int i = (parent_info.depth_min+1);i <= parent_info.depth_max;i++){
            
            const unsigned int x_num_ = parent_info.x_num[i];
            const unsigned int z_num_ = parent_info.z_num[i];
            
            const unsigned int x_num_parent = parent_info.x_num[i-1];
            
            
#pragma omp parallel for default(shared) private(z_,x_,j_,node_val,y_parent,j_parent,y_coord) if(z_num_*x_num_ > 100)
            for(z_ = 0;z_ < (z_num_);z_++){
                
                for(x_ = 0;x_ < (x_num_);x_++){
                    
                    const size_t z_parent = (z_)/2;
                    const size_t x_parent = (x_)/2;
                    
                    const size_t offset_pc_data = x_num_*z_ + x_;
                    const size_t offset_pc_data_parent = x_num_parent*z_parent + x_parent;
                    
                    
                    //initialization
                    y_coord = (neigh_info.data[i][offset_pc_data][0] & NEXT_COORD_MASK) >> NEXT_COORD_SHIFT ;
                    y_parent = (neigh_info.data[i-1][offset_pc_data_parent][0] & NEXT_COORD_MASK) >> NEXT_COORD_SHIFT;
                    
                    j_parent = 1;
    
                    if (neigh_info.data[i-1][offset_pc_data_parent].size() == 1){
                        //set to max so its not checked
                        y_parent = 64000;
                    }
                    
                    y_coord--;
                    
                    const size_t j_num = neigh_info.data[i][offset_pc_data].size();
                    const size_t j_num_parent = neigh_info.data[i-1][offset_pc_data_parent].size();
                    
                    for(j_ = 1;j_ < j_num;j_++){
                        
                        // Parent relation
                        
                        node_val = neigh_info.data[i][offset_pc_data][j_];
                        
                        if (node_val&1){
                            //get the index gap node
                            y_coord = (node_val & NEXT_COORD_MASK) >> NEXT_COORD_SHIFT;
                            y_coord--;
                            
                        } else {
                            //normal node
                            y_coord++;
                            
                            while ((y_parent < y_coord/2) & (j_parent < (j_num_parent-1))){
                                
                                j_parent++;
                                node_val = neigh_info.data[i-1][offset_pc_data_parent][j_parent];
                                
                                if (node_val&1){
                                    //get the index gap node
                                    y_parent = (node_val & NEXT_COORD_MASK) >> NEXT_COORD_SHIFT;
                                    j_parent++;
                                    
                                } else {
                                    //normal node
                                    y_parent++;
                                }
                            }
                            
                            
                            if (y_coord/2 == y_parent){
                                //set the link to the parent
                                
                                parent_info.data[i][offset_pc_data][j_] |= (j_parent << PARENT_SHIFT);
                                
                                //symmetric
                                if(y_coord == y_parent*2){
                                    //only add parent once
                                    //need to choose the correct one... formulae
                                    if(z_ == z_parent*2){
                                        
                                        if(x_ == x_parent*2){
                                           parent_info.data[i-1][offset_pc_data_parent][j_parent] |= (j_ << CHILD1_SHIFT);
                                        } else {
                                           parent_info.data[i-1][offset_pc_data_parent][j_parent] |= (j_ << CHILD2_SHIFT);
                                        }
                                        
                                    } else {
                                        if(x_ == x_parent*2){
                                            parent_info.data[i-1][offset_pc_data_parent][j_parent] |= (j_ << CHILD3_SHIFT);
                                        } else {
                                            parent_info.data[i-1][offset_pc_data_parent][j_parent] |= (j_ << CHILD4_SHIFT);
                                        }
                                        
                                    }
                                    
                                }
                            } else {
                                //std::cout << "BUG" << std::endl;
                            }
                        }
                    }
                }
            }
        }
    }

    void set_neighbor_relationships(uint8_t face){
        //
        //  Neighbour function for different face for parent nodes (does not go up and down levels)
        //
        //
        //
        const uint64_t depth_mask_dir[6] = {YP_DEPTH_MASK,YM_DEPTH_MASK,XP_DEPTH_MASK,XM_DEPTH_MASK,ZP_DEPTH_MASK,ZM_DEPTH_MASK};
        const uint64_t depth_shift_dir[6] =  {YP_DEPTH_SHIFT,YM_DEPTH_SHIFT,XP_DEPTH_SHIFT,XM_DEPTH_SHIFT,ZP_DEPTH_SHIFT,ZM_DEPTH_SHIFT};
        
        const uint64_t index_shift_dir[6] = {YP_INDEX_SHIFT,YM_INDEX_SHIFT,XP_INDEX_SHIFT,XM_INDEX_SHIFT,ZP_INDEX_SHIFT,ZM_INDEX_SHIFT};
        
        
        //variables for neighbour search loops
        const uint8_t x_start_vec[6] = {0,0,0,1,0,0};
        const uint8_t x_stop_vec[6] = {0,0,1,0,0,0};
        
        const uint8_t z_start_vec[6] = {0,0,0,0,0,1};
        const uint8_t z_stop_vec[6] = {0,0,0,0,1,0};
        
        //replication of above
        const int8_t x_offset_vec[6] = {0,0,1,-1,0,0};
        const int8_t z_offset_vec[6] = {0,0,0,0,1,-1};
        
        
        //x/z variables
        const uint64_t x_start = x_start_vec[face];
        const uint64_t x_stop = x_stop_vec[face];
        const uint64_t z_start = z_start_vec[face];
        const uint64_t z_stop = z_stop_vec[face];
        const int8_t x_offset = x_offset_vec[face];
        const int8_t z_offset = z_offset_vec[face];
        const uint64_t index_shift_0 = index_shift_dir[face];
        const uint64_t depth_shift_0 = depth_shift_dir[face];
        
        const uint64_t depth_mask_0 = depth_mask_dir[face];
        
        
        //y variables
        Part_timer timer;
        timer.verbose_flag = true;
        uint64_t z_;
        uint64_t x_;
        uint64_t j_;
        
        timer.start_timer("Get neighbours dir: " + std::to_string(face));
        
        unsigned int y_neigh;
        unsigned int y_parent;
        uint64_t j_parent;
        uint64_t j_neigh;
        
        uint64_t node_val;
        uint64_t y_coord;
        
        if (face > 1){
            
            for(int i = (neigh_info.depth_min);i <= neigh_info.depth_max;i++){
                
                const unsigned int x_num_ = neigh_info.x_num[i];
                const unsigned int z_num_ = neigh_info.z_num[i];
                
#pragma omp parallel for default(shared) private(z_,x_,j_,node_val,y_parent,j_parent,j_neigh,y_neigh,y_coord) if(z_num_*x_num_ > 100)
                for(z_ = z_start;z_ < (z_num_-z_stop);z_++){
                    
                    for(x_ = x_start;x_ < (x_num_-x_stop);x_++){
                        
                        const size_t z_neigh = (z_+z_offset);
                        const size_t x_neigh = (x_+x_offset);
                        
                        const size_t offset_pc_data = x_num_*z_ + x_;
                        const size_t offset_pc_data_neigh = x_num_*z_neigh + x_neigh;
                        
                        //initialization
                        y_coord = (neigh_info.data[i][offset_pc_data][0] & NEXT_COORD_MASK) >> NEXT_COORD_SHIFT;
                        y_neigh = (neigh_info.data[i][offset_pc_data_neigh][0] & NEXT_COORD_MASK) >> NEXT_COORD_SHIFT;
                        
                        j_neigh = 1;
                        
                        if (neigh_info.data[i][offset_pc_data_neigh].size() == 1){
                            //set to max so its not checked
                            y_neigh = 64000;
                        }
                        
                        y_coord--;
                        
                        const size_t j_num = neigh_info.data[i][offset_pc_data].size();
                        const size_t j_num_neigh = neigh_info.data[i][offset_pc_data_neigh].size();
                        
                        for(j_ = 1;j_ < j_num;j_++){
                            
                            // Parent relation
                            
                            node_val = neigh_info.data[i][offset_pc_data][j_];
                            
                            if (node_val&1){
                                //get the index gap node
                                y_coord = (node_val & NEXT_COORD_MASK) >> NEXT_COORD_SHIFT;
                                y_coord--;
                                
                            } else {
                                //normal node
                                y_coord++;
                                
                                while ((y_neigh < y_coord) & (j_neigh < (j_num_neigh-1))){
                                    
                                    j_neigh++;
                                    node_val = neigh_info.data[i][offset_pc_data_neigh][j_neigh];
                                    
                                    if (node_val&1){
                                        //get the index gap node
                                        y_neigh = (node_val & NEXT_COORD_MASK) >> NEXT_COORD_SHIFT;
                                        j_neigh++;
                                        
                                    } else {
                                        //normal node
                                        y_neigh++;
                                    }
                                }
                                
                                
                                if(y_coord == y_neigh){
                                    neigh_info.data[i][offset_pc_data][j_] |= (j_neigh << index_shift_0);
                                    neigh_info.data[i][offset_pc_data][j_]&= -((depth_mask_0)+1);
                                    neigh_info.data[i][offset_pc_data][j_] |= (LEVEL_SAME << depth_shift_0);
                                    
                                } else {
                                    //std::cout << "BUG" << std::endl;
                                }
                            }
                        }
                    }
                    
                }
                
            }
            
        }
    }
    
    template<typename S>
    void initialize_parents(PartCellStructure<S,T>& pc_struct){
        //
        //
        //  From a given particle structure of active cells create
        //
        //
        
        //Initialize the structures
        
        parent_info.depth_max = pc_struct.pc_data.depth_max-1;
        parent_info.depth_min = pc_struct.pc_data.depth_min;
        
        neigh_info.depth_max = pc_struct.pc_data.depth_max-1;
        neigh_info.depth_min = pc_struct.pc_data.depth_min;
        
        parent_info.z_num.resize(parent_info.depth_max+1);
        parent_info.x_num.resize(parent_info.depth_max+1);
        
        parent_info.data.resize(parent_info.depth_max+1);
        
        neigh_info.z_num.resize(neigh_info.depth_max+1);
        neigh_info.x_num.resize(neigh_info.depth_max+1);
        
        neigh_info.data.resize(neigh_info.depth_max+1);
        
        for(int i = neigh_info.depth_min;i <= neigh_info.depth_max;i++){
            neigh_info.z_num[i] = pc_struct.pc_data.z_num[i];
            neigh_info.x_num[i] = pc_struct.pc_data.x_num[i];
            neigh_info.data[i].resize(neigh_info.z_num[i]*neigh_info.x_num[i]);
        }
        
        for(int i = parent_info.depth_min;i <= parent_info.depth_max;i++){
            parent_info.z_num[i] = pc_struct.pc_data.z_num[i];
            parent_info.x_num[i] = pc_struct.pc_data.x_num[i];
            parent_info.data[i].resize(parent_info.z_num[i]*parent_info.x_num[i]);
        }
        
        //temporary array needs to be created for efficient creation of the parent structure
        std::vector<std::vector<uint8_t>> parent_map;
        
        parent_map.resize(neigh_info.depth_max+1);
        
        for(int i = parent_info.depth_min;i <= parent_info.depth_max;i++){
           
            parent_map[i].resize(pc_struct.x_num[i]*pc_struct.y_num[i]*pc_struct.z_num[i]);
        }
        
        
        //now loop over the structure
        uint64_t y_coord; //keeps track of y coordinate
        //initialize
        uint64_t node_val;
        
        uint64_t x_;
        uint64_t z_;
        uint64_t j_;
        uint64_t curr_key = 0;

        
        for(int i = (pc_struct.pc_data.depth_min+1);i <= pc_struct.pc_data.depth_max;i++){
            
            const unsigned int x_num_ = pc_struct.pc_data.x_num[i];
            const unsigned int z_num_ = pc_struct.pc_data.z_num[i];
            
            
#pragma omp parallel for default(shared) private(z_,x_,j_,node_val,curr_key,y_coord) if(z_num_*x_num_ > 100)
            for(z_ = 0;z_ < z_num_;z_++){
                
                curr_key = 0;
                
                //set the key values
                pc_struct.pc_data.pc_key_set_z(curr_key,z_);
                pc_struct.pc_data.pc_key_set_depth(curr_key,i);
                
                for(x_ = 0;x_ < x_num_;x_++){
                    
                    pc_struct.pc_data.pc_key_set_x(curr_key,x_);
                    
                    const size_t offset_pc_data = x_num_*z_ + x_;
                    
                    //number of nodes on the level
                    const size_t j_num = pc_struct.pc_data.data[i][offset_pc_data].size();
                    
                    uint64_t parent_x;
                    uint64_t parent_y;
                    uint64_t parent_z;
                    
                    uint64_t depth;
                    
                    for(j_ = 0;j_ < j_num;j_++){
                        
                        //this value encodes the state and neighbour locations of the particle cell
                        node_val = pc_struct.pc_data.data[i][offset_pc_data][j_];
                        
                        if (!(node_val&1)){
                            y_coord++; //iterate y
                            
                            parent_x = x_/2;
                            parent_z = z_/2;
                            parent_y = y_coord/2;
                            depth = i - 1;
                            
                            if(parent_map[depth][parent_z*pc_struct.y_num[depth]*pc_struct.x_num[depth] + parent_x*pc_struct.y_num[depth] + parent_y] ==0){
                                
                                parent_map[depth][parent_z*pc_struct.y_num[depth]*pc_struct.x_num[depth] + parent_x*pc_struct.y_num[depth] + parent_y] = 2;
                                
                                parent_x = parent_x/2;
                                parent_z = parent_z/2;
                                parent_y = parent_y/2;
                                depth = depth - 1;
                                
                                if(depth > pc_struct.pc_data.depth_min){
                                    
                                    while(parent_map[depth][parent_z*pc_struct.y_num[depth]*pc_struct.x_num[depth] + parent_x*pc_struct.y_num[depth] + parent_y] ==0 ){
                                        
                                        parent_map[depth][parent_z*pc_struct.y_num[depth]*pc_struct.x_num[depth] + parent_x*pc_struct.y_num[depth] + parent_y] = 1;
                                        
                                        parent_x = parent_x/2;
                                        parent_z = parent_z/2;
                                        parent_y = parent_y/2;
                                        depth = depth - 1;
                                        
                                        if  (depth < pc_struct.pc_data.depth_min){
                                            break;
                                        }
                                        
                                    }
                                }
                                
                                
                            }
                            
                            
                            
                        } else {
                            //This is a gap node
                            
                            //Gap nodes store the next and previous coodinate
                            y_coord = (node_val & NEXT_COORD_MASK) >> NEXT_COORD_SHIFT;
                            y_coord--; //set the y_coordinate to the value before the next coming up in the structure
                        }
                        
                    }
                }
            }
        }
        
        
        uint16_t curr_index;
        uint8_t status;
        uint8_t prev_ind = 0;
        uint64_t y_;
        
        // Construct the parent info
        
        for(int i = pc_struct.pc_data.depth_min;i <= (pc_struct.pc_data.depth_max-1);i++){
            
            const unsigned int x_num_ = pc_struct.x_num[i];
            const unsigned int z_num_ = pc_struct.z_num[i];
            const unsigned int y_num_ = pc_struct.y_num[i];
            
            
#pragma omp parallel for default(shared) private(z_,x_,y_,curr_index,status,prev_ind) if(z_num_*x_num_ > 100)
            for(z_ = 0;z_ < z_num_;z_++){
                
                for(x_ = 0;x_ < x_num_;x_++){
                    
                    size_t first_empty = 0;
                    const size_t offset_part_map = x_*y_num_ + z_*y_num_*x_num_;
                    const size_t offset_pc_data = x_num_*z_ + x_;
                    curr_index = 0;
                    prev_ind = 0;
                    
                    //first value handle the duplication of the gap node
                    
                    status = parent_map[i][offset_part_map];
                    if((status> 0)){
                        first_empty = 0;
                    } else {
                        first_empty = 1;
                    }
                    
                    for(y_ = 0;y_ < y_num_;y_++){
                        
                        status = parent_map[i][offset_part_map + y_];
                        
                        if(status> 0){
                            curr_index+= 1 + prev_ind;
                            prev_ind = 0;
                        } else {
                            prev_ind = 1;
                        }
                    }
                    
                    if(curr_index == 0){
                        neigh_info.data[i][offset_pc_data].resize(1); //always first adds an extra entry for intialization and extra info
                        parent_info.data[i][offset_pc_data].resize(1); //always first adds an extra entry for intialization and extra info
                    } else {
                        
                        neigh_info.data[i][offset_pc_data].resize(curr_index + 2 - first_empty,0); //gap node to begin, already finishes with a gap node
                        parent_info.data[i][offset_pc_data].resize(curr_index + 2 - first_empty,0); //gap node to begin, already finishes with a gap node
                        
                    }
                }
            }
            
        }
        
        uint64_t prev_coord = 0;
        
        
        for(int i = neigh_info.depth_min;i <= neigh_info.depth_max;i++){
            
            const unsigned int x_num_ = pc_struct.x_num[i];
            const unsigned int z_num_ = pc_struct.z_num[i];
            const unsigned int y_num_ = pc_struct.  y_num[i];
            
#pragma omp parallel for default(shared) private(z_,x_,y_,curr_index,status,prev_ind,prev_coord) if(z_num_*x_num_ > 100)
            for(z_ = 0;z_ < z_num_;z_++){
                
                for(x_ = 0;x_ < x_num_;x_++){
                    
                    
                    const size_t offset_part_map = x_*y_num_ + z_*y_num_*x_num_;
                    const size_t offset_pc_data = x_num_*z_ + x_;
                    curr_index = 0;
                    prev_ind = 1;
                    prev_coord = 0;
                    
                    //initialize the first values type
                    neigh_info.data[i][offset_pc_data][0] = TYPE_GAP_END;
                    
                    for(y_ = 0;y_ < y_num_;y_++){
                        
                        status = parent_map[i][offset_part_map + y_];
                        
                        if(status> 0){
                            
                            curr_index++;
                            
                            //set starting type
                            if(prev_ind == 1){
                                //gap node
                                //set type
                                neigh_info.data[i][offset_pc_data][curr_index-1] = TYPE_GAP;
                                neigh_info.data[i][offset_pc_data][curr_index-1] |= (y_ << NEXT_COORD_SHIFT);
                                neigh_info.data[i][offset_pc_data][curr_index-1] |= ( prev_coord << PREV_COORD_SHIFT);
                                neigh_info.data[i][offset_pc_data][curr_index-1] |= (NO_NEIGHBOUR << YP_DEPTH_SHIFT);
                                neigh_info.data[i][offset_pc_data][curr_index-1] |= (NO_NEIGHBOUR << YM_DEPTH_SHIFT);
                                
                                curr_index++;
                            }
                            prev_coord = y_;
                            //set type
                            neigh_info.data[i][offset_pc_data][curr_index-1] = TYPE_PC;
                            
                            //initialize the neighbours to empty (to be over-written later if not the case) (Boundary Conditions)
                            neigh_info.data[i][offset_pc_data][curr_index-1] |= (NO_NEIGHBOUR << XP_DEPTH_SHIFT);
                            neigh_info.data[i][offset_pc_data][curr_index-1] |= (NO_NEIGHBOUR << XM_DEPTH_SHIFT);
                            neigh_info.data[i][offset_pc_data][curr_index-1] |= (NO_NEIGHBOUR << ZP_DEPTH_SHIFT);
                            neigh_info.data[i][offset_pc_data][curr_index-1] |= (NO_NEIGHBOUR << ZM_DEPTH_SHIFT);
                            
                            //set the status
                            switch(status){
                                case GHOST_CHILDREN:
                                {
                                    neigh_info.data[i][offset_pc_data][curr_index-1] |= (GHOST_CHILDREN <<STATUS_SHIFT);
                                    break;
                                }
                                case REAL_CHILDREN:
                                {
                                    neigh_info.data[i][offset_pc_data][curr_index-1] |= (REAL_CHILDREN<<STATUS_SHIFT);
                                    break;
                                }
                            }
                            
                            prev_ind = 0;
                        } else {
                            //store for setting above
                            if(prev_ind == 0){
                                //prev_coord = y_;
                            }
                            
                            prev_ind = 1;
                            
                        }
                    }
                    
                    //Initialize the last value GAP END indicators to no neighbour
                    neigh_info.data[i][offset_pc_data][neigh_info.data[i][offset_pc_data].size()-1] = TYPE_GAP_END;
                    neigh_info.data[i][offset_pc_data][neigh_info.data[i][offset_pc_data].size()-1] |= (NO_NEIGHBOUR << YP_DEPTH_SHIFT);
                    neigh_info.data[i][offset_pc_data][neigh_info.data[i][offset_pc_data].size()-1] |= (NO_NEIGHBOUR << YM_DEPTH_SHIFT);
                }
            }
            
        }

        //
        //  Now set the neighbours for the neigh_info
        //
        
        set_neighbor_relationships(0);
        set_neighbor_relationships(1);
        set_neighbor_relationships(2);
        set_neighbor_relationships(3);
        set_neighbor_relationships(4);
        set_neighbor_relationships(5);
        
        //now set the parent child relationships
        set_parent_relationships();
        
    }
    
    
    template<uint64_t face>
    void get_neighs_face_parent_t(const uint64_t& curr_key,uint64_t node_val,std::vector<uint64_t>& neigh_keys){
        //
        //  Bevan Cheeseman (2016)
        //
        //  Get all the nieghbours in direction face for parent structure
        //
        /** Get neighbours of a cell in one of the direction
         *
         *  @param curr_key    input: current key, output: neighbour key
         *  @param face        direction to follow. Possible values are [0,5]
         *                     They stand for [+y,-y,+x,-x,+z,-z] //change this ordering.. (y+ y-) are different,
         */
        //
        
        
        constexpr uint64_t index_mask_dir[6] = {YP_INDEX_MASK,YM_INDEX_MASK,XP_INDEX_MASK,XM_INDEX_MASK,ZP_INDEX_MASK,ZM_INDEX_MASK};
        constexpr uint64_t index_shift_dir[6] = {YP_INDEX_SHIFT,YM_INDEX_SHIFT,XP_INDEX_SHIFT,XM_INDEX_SHIFT,ZP_INDEX_SHIFT,ZM_INDEX_SHIFT};
        
        constexpr int8_t von_neumann_y_cells[6] = { 1,-1, 0, 0, 0, 0};
        constexpr int8_t von_neumann_x_cells[6] = { 0, 0, 1,-1, 0, 0};
        constexpr int8_t von_neumann_z_cells[6] = { 0, 0, 0, 0, 1,-1};
        
        uint64_t neigh_j;
        
        uint64_t neigh_key;
        
        // +-y direction is different
        if(face < 2){
            
            neigh_key = curr_key;
            
            neigh_info.pc_key_offset_j(neigh_key,von_neumann_y_cells[face]);
            
            node_val = neigh_info.get_val(neigh_key);
            
            if(!(node_val&1)){
                //same level
                neigh_keys.push_back(neigh_key);
                
                return;
            } else {
                return;
            }
        }
        
        //dir
        neigh_j =  neigh_info.node_get_val(node_val,index_mask_dir[face],index_shift_dir[face]);
        
        if (neigh_j > 0){
            //same level return single neighbour
            neigh_key = curr_key;
        
            neigh_info.pc_key_set_j(neigh_key, neigh_j);
            neigh_info.pc_key_offset_x(neigh_key,von_neumann_x_cells[face]);
            neigh_info.pc_key_offset_z(neigh_key,von_neumann_z_cells[face]);
        
            neigh_keys.push_back(neigh_key);
        }
        
    }
    
    
};



#endif //PARTPLAY_PARTKEY_HPP