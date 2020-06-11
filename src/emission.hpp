//emission.h -- structure defining an atmospheric emission

#ifndef __emission_h_
#define __emission_h_

#include "Real.hpp"
#include <string>
#include <Eigen/Dense>
#include "atmo_vec.hpp"
#include "cuda_compatibility.hpp"
#include "voxel_vector.hpp"

using std::string;

template <int N_VOXELS>
struct emission {
  static const unsigned int n_voxels = N_VOXELS;
  string name;
  bool init;
  bool solved;
  
  Real branching_ratio;
  
  //these store physical atmospheric parameters on the grid (dimension n_voxels)
  //dynamic arrays are required for dim>~32 for Eigen
  //the vectors point to the Eigen objects so that these can be used interchangably
  typedef voxel_vector<N_VOXELS> vv;
  typedef voxel_matrix<N_VOXELS> vm;
  vv species_density; //average and point densities of scatterers and absorbers on the tabulated grid
  vv species_density_pt;
  vv absorber_density; 
  vv absorber_density_pt; 
  vv species_sigma;//average and point scatterer and absorber cross section on the tabulated grid
  vv species_sigma_pt;
  vv absorber_sigma;
  vv absorber_sigma_pt;

  vv dtau_species;
  vv dtau_species_pt;
  vv log_dtau_species; //quantities that need to be interpolated are also stored as log
  vv log_dtau_species_pt; 
  vv dtau_absorber;
  vv dtau_absorber_pt;
  vv log_dtau_absorber;
  vv log_dtau_absorber_pt; 
  vv abs; //ratio of dtau_abs to dtau_species
  vv abs_pt; //ratio of dtau_abs to dtau_species
  vv log_abs; 
  vv log_abs_pt; 

  //Radiative transfer parameters
  vm influence_matrix; //influence matrix has dimensions n_voxels, n_voxels)

  //vectors to compute the single scattering have dimensions (n_voxels)  
  vv tau_species_single_scattering;
  vv tau_absorber_single_scattering;
  vv singlescat; 

  vv sourcefn; //have dimensions (n_voxels)  
  vv log_sourcefn; 

  CUDA_CALLABLE_MEMBER
  emission() :
    init(false)
  { }
  CUDA_CALLABLE_MEMBER
  ~emission() { };

  void resize() {
    species_density.resize();
    species_density_pt.resize();
    absorber_density.resize();
    absorber_density_pt.resize();
    species_sigma.resize();
    species_sigma_pt.resize();
    absorber_sigma.resize();
    absorber_sigma_pt.resize();

    dtau_species.resize();
    dtau_species_pt.resize();
    log_dtau_species.resize();
    log_dtau_species_pt.resize();
    dtau_absorber.resize();
    dtau_absorber_pt.resize();
    log_dtau_absorber.resize();
    log_dtau_absorber_pt.resize();
    abs.resize();
    abs_pt.resize();
    log_abs.resize();
    log_abs_pt.resize();

    influence_matrix.resize();

    singlescat.resize();
    sourcefn.resize();
    log_sourcefn.resize();

    tau_species_single_scattering.resize();
    tau_absorber_single_scattering.resize();
  }
  
  // template<typename C, typename V>
  // void define(Real emission_branching_ratio,
  // 	      C &atmosphere,
  // 	      Real (C::*species_density_function)(const atmo_point),
  // 	      Real (C::*species_sigma_function)(const atmo_point),
  // 	      Real (C::*absorber_density_function)(const atmo_point),
  // 	      Real (C::*absorber_sigma_function)(const atmo_point),
  // 	      const V &pts) {
    
  //   branching_ratio = emission_branching_ratio;
    
  //   for (unsigned int i_voxel=0;i_voxel<n_voxels;i_voxel++) {
  //     species_density[i_voxel]=(atmosphere.*species_density_function)(pts[i_voxel]);
  //     species_sigma[i_voxel]=(atmosphere.*species_sigma_function)(pts[i_voxel]);
  //     absorber_density[i_voxel]=(atmosphere.*absorber_density_function)(pts[i_voxel]);
  //     absorber_sigma[i_voxel]=(atmosphere.*absorber_sigma_function)(pts[i_voxel]);
  //   }
    
  //   //define differential optical depths by coefficientwise multiplication
  //   dtau_species = species_density.array() * species_sigma.array();
  //   dtau_absorber = absorber_density.array() * absorber_sigma.array();
  //   abs = dtau_absorber.array() / dtau_species.array();
    
  //   for (unsigned int i=0;i<log_dtau_species.size();i++) {
  //     log_dtau_species(i) = dtau_species(i) == 0 ? -1e5 : log(dtau_species(i));
  //     log_dtau_absorber(i) = dtau_absorber(i) == 0 ? -1e5 : log(dtau_species(i));
  //     log_abs(i) = abs(i) == 0 ? -1e5 : log(abs(i));
  //   }
    
  //   init=true;
  // }

  template<typename C>
  void define(Real emission_branching_ratio,
	      C &atmosphere,
	      void (C::*species_density_function)(const atmo_voxel, Real &ret_avg, Real &ret_pt),
	      void (C::*species_sigma_function)(const atmo_voxel, Real &ret_avg, Real &ret_pt),
	      void (C::*absorber_density_function)(const atmo_voxel, Real &ret_avg, Real &ret_pt),
	      void (C::*absorber_sigma_function)(const atmo_voxel, Real &ret_avg, Real &ret_pt),
	      const atmo_voxel (&voxels)[n_voxels]) {
    
    branching_ratio = emission_branching_ratio;
    
    for (unsigned int i_voxel=0;i_voxel<n_voxels;i_voxel++) {
      (atmosphere.*species_density_function)(voxels[i_voxel],
					     species_density[i_voxel],
					     species_density_pt[i_voxel]);

      (atmosphere.*species_sigma_function)(voxels[i_voxel],
					   species_sigma[i_voxel],
					   species_sigma_pt[i_voxel]);
      
      (atmosphere.*absorber_density_function)(voxels[i_voxel],
					      absorber_density[i_voxel],
					      absorber_density_pt[i_voxel]);
      
      (atmosphere.*absorber_sigma_function)(voxels[i_voxel],
					    absorber_sigma[i_voxel],
					    absorber_sigma_pt[i_voxel]);
    }
    
    //define differential optical depths by coefficientwise multiplication
    dtau_species = species_density.array() * species_sigma.array();
    dtau_species_pt = species_density_pt.array() * species_sigma_pt.array();
    dtau_absorber = absorber_density.array() * absorber_sigma.array();
    dtau_absorber_pt = absorber_density_pt.array() * absorber_sigma_pt.array();
    abs = dtau_absorber.array() / dtau_species.array();
    abs_pt = dtau_absorber_pt.array() / dtau_species_pt.array();
    
    for (unsigned int i=0;i<log_dtau_species.size();i++) {
      log_dtau_species(i) = dtau_species(i) == 0 ? -1e5 : log(dtau_species(i));
      log_dtau_species_pt(i) = dtau_species_pt(i) == 0 ? -1e5 : log(dtau_species_pt(i));
      log_dtau_absorber(i) = dtau_absorber(i) == 0 ? -1e5 : log(dtau_species(i));
      log_dtau_absorber_pt(i) = dtau_absorber_pt(i) == 0 ? -1e5 : log(dtau_species_pt(i));
      log_abs(i) = abs(i) == 0 ? -1e5 : log(abs(i));
      log_abs_pt(i) = abs_pt(i) == 0 ? -1e5 : log(abs_pt(i));
    }
    
    init=true;
  }


  //methods to transfer objects to device
  void copy_to_device_influence(emission<N_VOXELS> *device_emission);
  void copy_to_device_brightness(emission<N_VOXELS> *device_emission);
  void vector_to_device(vv & device_vec, vv & host_vec, bool transfer = true);
  void matrix_to_device(vm & device_vec, vm & host_vec, bool transfer = true);

  void copy_influence_to_host();
  void copy_solved_to_host();
  void vector_to_host(vv & host_vec);
  void matrix_to_host(vm & host_vec);
};

#ifdef __CUDACC__
#include "emission_gpu.cu"
#endif

#endif
