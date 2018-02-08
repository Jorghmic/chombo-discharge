/*!
  @file rod_slab_geometry.cpp
  @brief Implementation of rod_slab_geometry.H
  @author Robert Marskar
  @date Nov. 2017
*/

#include "rod_slab.H"

#include <string>
#include <iostream>
#include <fstream>

#include <ParmParse.H>
#include <BaseIF.H>
#include <SphereIF.H>

#include "rounded_box_if.H"
#include "rod_if.H"
#include "new_sphere_if.H"

rod_slab::rod_slab(){
  this->set_eps0(1.0);
  m_dielectrics.resize(1);
  m_electrodes.resize(1);

  // Electrode
  bool live            = true;
  Real rod_radius      = 200.E-6;
  RealVect rod_center1 = RealVect::Zero;
#if CH_SPACEDIM == 2
  RealVect rod_center2 = RealVect(0.0, 1.0);
#else
  RealVect rod_center2 = RealVect(0..0, 0.0, 1.0);
#endif


  //Slab
  Real eps_mat     = 5.0;
  Real curv        = 200.E-6;
#if CH_SPACEDIM == 2
  RealVect slab_lo = RealVect(-2.0123E-2, -2.0123E-2);
  RealVect slab_hi = RealVect(2.0123E-2,  -1.0123E-2);
#else
  RealVect slab_lo = RealVect(-1.0123E-2, -2.0123E-2,  -2.0123E-2);
  RealVect slab_hi = RealVect( 1E0,        2.0123E-2,  -1.5123E-2);
#endif

  RefCountedPtr<BaseIF> rod  = RefCountedPtr<BaseIF> (new rod_if(rod_center1, rod_center2, rod_radius, false));
  RefCountedPtr<BaseIF> sph  = RefCountedPtr<BaseIF> (new new_sphere_if(RealVect(0., 0.04), 5*rod_radius, false));
  RefCountedPtr<BaseIF> slab = RefCountedPtr<BaseIF> (new rounded_box_if(slab_lo, slab_hi, curv, false));

  m_electrodes[0].define(rod,   live);  
  m_dielectrics[0].define(slab, eps_mat);

#if 1 // Debug
  m_dielectrics.resize(0);
#endif
}

rod_slab::~rod_slab(){
}

