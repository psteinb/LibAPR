%module apr

%include "std_vector.i"
%include "stdint.i"

using namespace std;
namespace std {
//%template(IntegerVec) std::vector<uint16_t>;
//%template(ShortVec) std::vector<uint8_t>;
%template(U16Vec) std::vector<uint16_t>;
%template(U8Vec) std::vector<uint8_t>;
%template(UVec) std::vector<unsigned int>;
}

%include "std_string.i"

%rename(equals) operator==;
%rename(less_than) operator<;

%{
#include "src/data_structures/particle_map.hpp"
#include "src/data_structures/structure_parts.h"
#include "src/algorithm/gradient.hpp"
#include "src/algorithm/pipeline.h"
#include "src/io/partcell_io.h"
//#include "src/data_structures/Tree/ParticleData.hpp"
#include "src/data_structures/Tree/ParticleDataNew.hpp"
//#include "src/data_structures/Tree/PartCellStructure.hpp"
#include "src/numerics/filter_help/CurrLevel.hpp"
#include "src/numerics/misc_numerics.hpp"
%}

%include "src/data_structures/particle_map.hpp"
%include "src/data_structures/structure_parts.h"
%include "src/algorithm/gradient.hpp"
%include "src/algorithm/pipeline.h"
%include "src/io/partcell_io.h"
%include "src/data_structures/Tree/ParticleData.hpp"
%include "src/data_structures/Tree/ParticleDataNew.hpp"
%include "src/data_structures/Tree/ExtraPartCellData.hpp"
%include "src/data_structures/Tree/PartCellData.hpp"
%include "src/data_structures/Tree/PartCellStructure.hpp"
%include "src/data_structures/particle_map.hpp"
%include "src/data_structures/meshclass.h"
%include "src/numerics/misc_numerics.hpp"

%template(PartDataUint16) Part_data<uint16_t>;
%template(PartDataUint8) Part_data<uint8_t>;

%template(ParticleDataStd) ParticleData<float, uint64_t>;
%template(PartCellStructureStd) PartCellStructure<float, uint64_t>;
%template(PartCellDataStd) PartCellData<uint64_t>;
%template(ParticleDataNewStd) ParticleDataNew<float, uint64_t>;
%template(ExtraPartCellDataStd) ExtraPartCellData<uint16_t>;
%template(ExtraPartCellDataFloat) ExtraPartCellData<float>;

// function templates
%template(ReadFloatAPRFromFile) read_apr_pc_struct<float>;
%template(ShiftParticlesFromCellsStd) shift_particles_from_cells<uint16_t, uint64_t, float>;
