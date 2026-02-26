/* chombo-discharge
 * Copyright © 2025 SINTEF Energy Research.
 * Please refer to Copyright.txt and LICENSE in the chombo-discharge root directory.
 */

/*!
  @file   CD_ItoKMCSubclassImplem.H
  @brief  Implementation of CD_ItoKMCSubclass.H
  @author Robert Marskar
*/

#ifndef CD_ItoKMCSubclassImplem_H
#define CD_ItoKMCSubclassImplem_H

// Our includes
#include <CD_ItoKMCSubclass.H>
#include <CD_NamespaceHeader.H>

using namespace Physics::ItoKMC;

template <typename I, typename C, typename R, typename F>
ItoKMCSubclass<I, C, R, F>::ItoKMCSubclass(RefCountedPtr<ItoKMCPhysics>& a_physics)
  : ItoKMCGodunovStepper<I, C, R, F>(a_physics)
{
  CH_TIME("ItoKMCSubclass::ItoKMCSubclass");
  if (this->m_verbosity > 5) {
    pout() << "ItoKMCSubclass::ItoKMCSubclass" << endl;
  }

  m_restart = false;
}

template <typename I, typename C, typename R, typename F>
void
ItoKMCSubclass<I, C, R, F>::refillSolversExceptSigma() noexcept
{
  CH_TIME("ItoKMCSubclass::refillSolversExceptSigma");
  if (this->m_verbosity > 5) {
    pout() << "ItoKMCSubclass::refillSolversExceptSigma" << endl;
  }

  this->m_ito->initialData();
  this->m_cdr->initialData();
  this->m_rte->initialData();

  this->m_ito->organizeParticlesByCell(ItoSolver::WhichContainer::Bulk);
  this->m_ito->makeSuperparticles(ItoSolver::WhichContainer::Bulk, this->m_particlesPerCell);
  this->m_ito->organizeParticlesByPatch(ItoSolver::WhichContainer::Bulk);
}

template <typename I, typename C, typename R, typename F>
void
ItoKMCSubclass<I, C, R, F>::postCheckpointSetup() noexcept
{
  CH_TIME("ItoKMCSubclass::postCheckpointSetup");
  if (this->m_verbosity > 5) {
    pout() << "ItoKMCSubclass::postCheckpointSetup" << endl;
  }

  // Fill solvers with initial data (except the surface charge solver)
  this->refillSolversExceptSigma();

  // Solve the Poisson equation for the electric field
  this->m_fieldSolver->setPermittivities();
  this->computeSpaceChargeDensity();
  this->solvePoisson();

  // Compute drift velocities and diffusion coefficients and prepare for a time step.
  this->computeDriftVelocities();
  this->computeDiffusionCoefficients();
  this->fillNeutralDensity();

  m_restart = true;
}

template <typename I, typename C, typename R, typename F>
void
ItoKMCSubclass<I, C, R, F>::regrid(const int a_lmin, const int a_oldFinestLevel, const int a_newFinestLevel) noexcept
{
  CH_TIME("ItoKMCSubclass::regrid");
  if (this->m_verbosity > 5) {
    pout() << "ItoKMCSubclass::regrid" << endl;
  }

  // Doing this because ItoKMCGodunovStepper runs a different regrid routine that also takes into account
  // the semi-implicit update. But that's not relevant on restarts becaue all the space charge is reset. We can
  // call the usual regrid method (or alternatively, reset all the semi-implicit stuff but that's 5 extra lines of
  // code.
  if (m_restart) {
    this->refillSolversExceptSigma();

    ItoKMCStepper<I, C, R, F>::regrid(a_lmin, a_oldFinestLevel, a_newFinestLevel);
  }
  else {
    ItoKMCGodunovStepper<I, C, R, F>::regrid(a_lmin, a_oldFinestLevel, a_newFinestLevel);
  }
}

template <typename I, typename C, typename R, typename F>
Real
ItoKMCSubclass<I, C, R, F>::advance(const Real a_dt)
{
  CH_TIME("ItoKMCSubclass::advance");
  if (this->m_verbosity > 5) {
    pout() << "ItoKMCSubclass::advance" << endl;
  }

  m_restart = false;

  return ItoKMCGodunovStepper<I, C, R, F>::advance(a_dt);
}

#include <CD_NamespaceFooter.H>

#endif
