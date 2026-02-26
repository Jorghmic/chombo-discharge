/* chombo-discharge
 * Copyright © 2025 SINTEF Energy Research.
 * Please refer to Copyright.txt and LICENSE in the chombo-discharge root directory.
 */

/*!
  @file   CD_ItoKMCSubclass.H
  @brief  Declaration of a subclass which can add particles on restarts.
  @author Robert Marskar
*/

#ifndef CD_ItoKMCSubclass_H
#define CD_ItoKMCSubclass_H

// Our includes
#include <CD_ItoKMCGodunovStepper.H>
#include <CD_NamespaceHeader.H>

namespace Physics {
  namespace ItoKMC {

    /*!
      @brief Subclass of ItoKMCSubclass that manipulates space charges on restarts.
    */
    template <typename I = ItoSolver, typename C = CdrCTU, typename R = McPhoto, typename F = FieldSolverGMG>
    class ItoKMCSubclass : public ItoKMCGodunovStepper<I, C, R, F>
    {
    public:
      /*!
	@brief Disallowed default constructor. Use the full constructor.
      */
      ItoKMCSubclass() = delete;

      /*!
	@brief Full constructor. 
	@param[in] a_phyics Physics implementation. 
      */
      ItoKMCSubclass(RefCountedPtr<ItoKMCPhysics>& a_physics);

      /*!
	@brief Destructor. Does nothing
      */
      virtual ~ItoKMCSubclass() = default;

      /*!
	@brief Regrid methods -- puts all data on the new mesh. 
	@param[in] a_lmin           The coarsest level that changed. 
	@param[in] a_oldFinestLevel The finest level before the regrid. 
	@param[in] a_newFinestLevel The finest level after the regrid. 
      */
      virtual void
      regrid(const int a_lmin, const int a_oldFinestLevel, const int a_newFinestLevel) noexcept override;

      /*!
	@brief Perform post-checkpoint operations
	@details After running the regular post checkpoint setup, this class looks through every particle solver
	and adds the initial data back in.
      */
      virtual void
      postCheckpointSetup() noexcept override;

      /*!
	@brief Override, only to indicate that checkpoint stage is over.
	@param[in] a_dt Time step
      */
      virtual Real
      advance(const Real a_dt) override;

    protected:
      /*!
	@brief Set to true if the postCheckpointMethod was called
      */
      bool m_restart;

      /*!
	@brief Fill all solvers except the surface charge solver (which is left untouched)
      */
      virtual void
      refillSolversExceptSigma() noexcept;
    };
  } // namespace ItoKMC
} // namespace Physics

#include <CD_NamespaceFooter.H>

#include <CD_ItoKMCSubclassImplem.H>

#endif
