#ifdef CH_LANG_CC
/*
 *      _______              __
 *     / ___/ /  ___  __ _  / /  ___
 *    / /__/ _ \/ _ \/  V \/ _ \/ _ \
 *    \___/_//_/\___/_/_/_/_.__/\___/
 *    Please refer to Copyright.txt, in Chombo's root directory.
 */
#endif

#include <cstdio>

#include "PatchMappedConsOperator.H"
#include "FourthOrderUtil.H"
#include "CellToEdge.H"
#include "SimpleDivergence.H"
#include "AdvectOpF_F.H"
#include "UnitNormalsF_F.H"

#include "NamespaceHeader.H"

//////////////////////////////////////////////////////////////////////////////
// Constructor - set up some defaults
PatchMappedConsOperator::PatchMappedConsOperator() : PatchConsOperator()
{
  m_numFluxesPerField = SpaceDim;
  m_coordSysPtr = NULL;
  m_faceMetricTerms = NULL;
  m_unitNormalFabPtr = NULL;
}

//////////////////////////////////////////////////////////////////////////////
// Destructor - free up storage
PatchMappedConsOperator::~PatchMappedConsOperator()
{
}


//////////////////////////////////////////////////////////////////////////////
// Define the object so that time stepping can begin
void PatchMappedConsOperator::define(
                                     const ProblemDomain&      a_domain,
                                     const Real&               a_dx,
                                     const MOLPhysics* const a_molPhysics,
                                     const int&                a_numFields)
{
  // Sanity checks
  CH_assert(a_dx > 0.0);

  // Cache data
  m_dx = a_dx;
  m_domain = a_domain;

  m_numFields = a_numFields;

  // MOLPhysics* m_molPhysics;
  m_molPhysics = a_molPhysics->new_molPhysics();
  m_molPhysics->define(m_domain, m_dx);

  m_numFluxes = m_molPhysics->numFluxes();
  // Actually we will use SpaceDim * m_numCons as number of fluxes.

  // MOLUtilities m_util;
  m_util.define(m_domain, m_dx);
  m_util.highOrderLimiter(m_highOrderLimiter);

  m_doDeconvolution = true;
  m_noPPM = false;
  m_useArtificialViscosity = false;

  // Everything is defined now.
  m_isDefined = true;
}

//////////////////////////////////////////////////////////////////////////////
void
PatchMappedConsOperator::setCoordSys(NewFourthOrderCoordSys* a_coordSysPtr)
{
  m_coordSysPtr = a_coordSysPtr;
  // Set Tuple<IntVect, SpaceDim> m_metricTermComponents.
  for (int idir = 0; idir < SpaceDim; idir++)
    {
      IntVect& metricTermComponentsDir = m_metricTermComponents[idir];
      for (int comp = 0; comp < SpaceDim; comp++)
        {
          metricTermComponentsDir[comp] = // transposed 22 Oct 2009
            m_coordSysPtr->getNcomponent(comp, idir); // WAS (idir, comp);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
// Evaluate the operator (-div(F) ) at time m_currentTime.
// on conserved variables a_U:  at coarser level a_UcoarseOld, a_UcoarseNew.
// "a_finerFluxRegister" is the flux register with the next finer level.
// "a_coarseFluxRegister" is flux register with the next coarser level.
// The flux registers are incremented with the normal derivatives of a_U
// at the boundary, times a fraction of the time step corresponding to
// which stage of an explicit time-stepping scheme (e.g. Runge-Kutta)
// is being invoked.
void PatchMappedConsOperator::evalRHS(
                                      FArrayBox&          a_LofU,
                                      const FArrayBox&    a_JUavgFab,
                                      FluxBox&            a_FfaceAvg,
                                      Real                a_weight,
                                      bool                a_setFlattening,
                                      FArrayBox&          a_flatteningFab)
{
  CH_assert(isDefined());
  CH_assert(m_isCurrentBoxSet);

  /*
    Get face-centered N.
  */
  Box bx1 = grow(m_currentBox, 1);
  int nFaceMetricTerms = SpaceDim*SpaceDim;
  m_faceMetricTerms = new FluxBox(bx1, nFaceMetricTerms);
  m_coordSysPtr->getN(*m_faceMetricTerms, bx1);

  /*
    Get UavgFab:  cell-averaged conserved variables
    (from cell-averaged J * U in a_JUavgFab)
  */
  Box grownBox = grow(m_currentBox, m_numGhost);
  int numU = a_JUavgFab.nComp();
  FArrayBox UavgFab(grownBox, numU);
  cellUJToCellU(UavgFab, a_JUavgFab);
  PatchConsOperator::evalRHS(a_LofU, UavgFab, a_FfaceAvg, a_weight,
                             a_setFlattening, a_flatteningFab);
  delete m_faceMetricTerms;
}

//////////////////////////////////////////////////////////////////////////////

void PatchMappedConsOperator::getFluxDivergence(FArrayBox&   a_LofU,
                                                FluxBox&     a_FfaceAvg,
                                                FluxBox&     a_FfaceForGrad)
{
  /*
    Fill in one layer of ghost faces of FfaceForGrad that are outside m_domain
    by 2nd-order extrapolation.
    Starting with FfaceForGrad on all faces of grow(m_currentBox, 1) & m_domain
    we end with FfaceForGrad on all faces of grow(m_currentBox, 1).
    Added by petermc, 19 Oct 2010.
  */
  Box bx1 = grow(m_currentBox, 1);
  Box bx1inDomain(bx1);
  bx1inDomain &= m_domain;
  for (int idir = 0; idir < SpaceDim; idir++)
    { // idir specifies the flux direction
      FArrayBox& FfaceForGradFab = a_FfaceForGrad[idir];
      secondOrderTransExtrapFacesAtDomainBdry(FfaceForGradFab,
                                              idir,
                                              bx1inDomain,
                                              m_domain);
    }

  // Copy components of FfaceAvg into FfaceAvgComp.
  // These are the components we need for
  // computeMetricTermProductAverage and mappedGridDivergence.

  // Components of FfaceAvg are grouped by dimension, m_numFluxes at a time.
  // FfaceAvgComp has SpaceDim components for a single flux variable.
  RealVect dxVect = m_dx * RealVect::Unit;
  FluxBox FfaceAvgComp(m_currentBox, SpaceDim);
  FluxBox FfaceForGradComp(bx1, SpaceDim);
  FluxBox normalFfaceAvgComp(bx1, 1);
  // Do I need this to be bigger?  bx1inDomain?
  FluxBox normalFfaceAvgAll(m_currentBox, m_numFluxes);
  for (int comp = 0; comp < m_numFields; comp++)
    {
      // Find one component of a_LofU at a time.
      //      LevelData<FArrayBox> LofUComp;
      //      aliasLevelData(LofUComp, &a_LofU, compInt);
      for (int idir = 0; idir < SpaceDim; idir++)
        { // idir specifies the flux direction
          int srcComp = idir*m_numFluxes + comp;
          int dstComp = idir;

          FfaceAvgComp.copy(a_FfaceAvg, srcComp, dstComp, 1);
          FfaceForGradComp.copy(a_FfaceForGrad, srcComp, dstComp, 1);
        }

      // LevelData<FluxBox> normalFfaceAvgComp(m_grids, 1, ivGhost);
      // You can't alias LevelData<FluxBox>.
      //      aliasLevelData(normalFfaceAvgComp, &normalFfaceAvg, compInt);
      // computes 4th-order average of product = N^T*F
      // m_coordSysPtr->computeMetricTermProductAverage(normalFfaceAvgComp,
      // FfaceAvgComp);

      /*
        Find normalFfaceAvgComp on all faces of m_currentBox
        using FfaceAvgComp on all faces of m_currentBox
        and faceMetricTerms on all faces of bx1 = grow(m_currentBox, 1)
        and FfaceForGradComp on all faces of bx1 = grow(m_currentBox, 1).

        (Use FfaceForGradComp instead of FfaceAvgComp in order to reduce
        the size of the stencil required.)
      */
      m_coordSysPtr->computeMetricTermProductAverage(normalFfaceAvgComp,
                                                     FfaceAvgComp,
                                                     *m_faceMetricTerms,
                                                     FfaceForGradComp,
                                                     m_currentBox);

      // added by petermc, 6 Oct 2009
      normalFfaceAvgComp *= (1./m_dx);

      // mappedGridDivergence calls computeMetricTermProductAverage,
      // to find N^T*F.  But the components are in the wrong order.

      // Need to call this one component at a time.
      // LofUCompFab aliases a_LofU[comp].
      /*
        Set LofUCompFab = div(normalFfaceAvgComp)
        on all cells of m_currentBox,
        using normalFfaceAvgComp on all faces of m_currentBox.
       */
      FArrayBox LofUCompFab;
      Interval compInt(comp, comp);
      LofUCompFab.define(compInt, a_LofU);
      // LofUCompFab[i] := sum_d (1/dx[d]) *
      //    (normalFfaceAvgComp[i + e_d/2] - normalFfaceAvgComp[i - e_d/2])
      simpleDivergence(LofUCompFab, normalFfaceAvgComp, m_currentBox, dxVect);

      // Set a_normalFfaceAvg[comp] = normalFfaceAvgComp[0]
      normalFfaceAvgAll.copy(normalFfaceAvgComp, 0, comp, 1);
    }
  a_FfaceAvg.resize(m_currentBox, m_numFluxes);
  a_FfaceAvg.copy(normalFfaceAvgAll);

  // This doesn't work, but if you did not have to shuffle components:
  //  m_coordSysPtr->computeMetricTermProductAverage(a_normalFfaceAvg,
  //                                                 FfaceAvg,
  //                                                 faceMetricTerms,
  //                                                 grownBox);
  //  simpleDivergence(a_LofU, a_normalFfaceAvg, m_currentBox, dxVect);

  // Should this be BEFORE divergence computation?
  // kludge that seems to help
  // removed by petermc, 5 Oct 2009
  // a_normalFfaceAvg *= 1. / m_dx;
}


//////////////////////////////////////////////////////////////////////////////
void
PatchMappedConsOperator::getAllFluxes(FluxBox&        a_FfaceAvg,
                                      FluxBox&        a_FfaceCen,
                                      const FluxBox&  a_WfaceAvg,
                                      const FluxBox&  a_WfaceCen)
{
  // Need to cast non-const in order to alias
  FluxBox& WfaceAvg = (FluxBox&) a_WfaceAvg;
  FluxBox& WfaceCen = (FluxBox&) a_WfaceCen;
  CH_assert(m_isCurrentBoxSet);
  Box bx1inDomain = grow(m_currentBox, 1);
  bx1inDomain &= m_domain;
  for (int idir = 0; idir < SpaceDim; idir++)
    { // idir specifies which faces:  either x or y or z
      // box of valid edges for this grid
      Box faceBox1(bx1inDomain);
      faceBox1.surroundingNodes(idir);
      getDirFluxes(a_FfaceAvg[idir], WfaceAvg[idir], faceBox1);

      Box faceBox0(m_currentBox);
      faceBox0.surroundingNodes(idir);
      getDirFluxes(a_FfaceCen[idir], WfaceCen[idir], faceBox0);
    }
}

//////////////////////////////////////////////////////////////////////////////
void
PatchMappedConsOperator::getDirFluxes(FArrayBox&  a_Fface,
                                      const FArrayBox&  a_Wface,
                                      const Box&  a_box)
{
  // Fill in a_Fface.
  // It has SpaceDim*m_numFluxes components.
  // We fill in m_numFluxes components at a time.
  // So they will contain:
  // 0:m_numFluxes-1 for idir=0
  // ...
  // (SpaceDim-1)*m_numFluxes:SpaceDim*m_numFluxes-1 for idir=SpaceDim-1
  for (int idir = 0; idir < SpaceDim; idir++)
    { // idir specifies which flux component
      int fluxIntLo = idir * m_numFluxes;
      int fluxIntHi = fluxIntLo + m_numFluxes-1;
      Interval fluxInt(fluxIntLo, fluxIntHi);
      FArrayBox FfaceDirD(fluxInt, a_Fface);
      m_molPhysics->getFlux(FfaceDirD, a_Wface, idir, a_box);
    }
}


//////////////////////////////////////////////////////////////////////////////
void
PatchMappedConsOperator::preRiemann(FArrayBox&  a_WLeft,
                                    FArrayBox&  a_WRight,
                                    int         a_dir,
                                    const Box&  a_box)
{
  setBasisVectors(a_box, a_dir);
  Interval velInt = m_molPhysics->velocityInterval();

  FArrayBox& shiftWLeft  = (FArrayBox&)a_WLeft;
  FArrayBox& shiftWRight = (FArrayBox&)a_WRight;
  shiftWLeft .shiftHalf(a_dir, 1);
  shiftWRight.shiftHalf(a_dir,-1);
  CH_assert(shiftWLeft .box().contains(a_box));
  CH_assert(shiftWRight.box().contains(a_box));

  FArrayBox velLeftFab(velInt, shiftWLeft);
  FArrayBox velRightFab(velInt, shiftWRight);
  forwardBasisTransform(velLeftFab);
  forwardBasisTransform(velRightFab);

  shiftWLeft .shiftHalf(a_dir,-1);
  shiftWRight.shiftHalf(a_dir, 1);
}


//////////////////////////////////////////////////////////////////////////////
void
PatchMappedConsOperator::postRiemann(FArrayBox&  a_Wface,
                                     int         a_dir,
                                     const Box&  a_box)
{
 // Transform back velocity components in a_Wface.
  Interval velInt = m_molPhysics->velocityInterval();
  FArrayBox velNewFab(velInt, a_Wface);
  reverseBasisTransform(velNewFab);
  unsetBasisVectors();
}


//////////////////////////////////////////////////////////////////////////////
void
PatchMappedConsOperator::setBasisVectors(const Box& a_box,
                                         int a_dir)
{
  const IntVect& metricTermComponentsDir = m_metricTermComponents[a_dir];
  FArrayBox& faceMetricTermsDirFab = (*m_faceMetricTerms)[a_dir];
  m_unitNormalFabPtr = new FArrayBox(a_box, SpaceDim*SpaceDim);
  FArrayBox& unitNormalFab = *m_unitNormalFabPtr;
  FORT_GETUNITNORMALS(CHF_FRA(unitNormalFab),
                      CHF_CONST_FRA(faceMetricTermsDirFab),
                      CHF_CONST_INTVECT(metricTermComponentsDir),
                      CHF_CONST_INT(a_dir),
                      CHF_BOX(a_box));
}


//////////////////////////////////////////////////////////////////////////////
void
PatchMappedConsOperator::unsetBasisVectors()
{
  if (m_unitNormalFabPtr != NULL)
    {
      delete m_unitNormalFabPtr;
    }
}


//////////////////////////////////////////////////////////////////////////////
void
PatchMappedConsOperator::forwardBasisTransform(FArrayBox& a_W)
{
  CH_assert(m_unitNormalFabPtr != NULL);
  FArrayBox& unitNormalFab = *m_unitNormalFabPtr;
  Box bx = a_W.box() & unitNormalFab.box();
  FORT_FORWARDTRANSFORMF(CHF_FRA(a_W),
                         CHF_CONST_FRA(unitNormalFab),
                         CHF_BOX(bx));
}


//////////////////////////////////////////////////////////////////////////////
void
PatchMappedConsOperator::reverseBasisTransform(FArrayBox& a_W)
{
  CH_assert(m_unitNormalFabPtr != NULL);
  FArrayBox& unitNormalFab = *m_unitNormalFabPtr;
  Box bx = a_W.box() & unitNormalFab.box();
  FORT_REVERSETRANSFORMF(CHF_FRA(a_W),
                         CHF_CONST_FRA(unitNormalFab),
                         CHF_BOX(bx));
}


//////////////////////////////////////////////////////////////////////////////
void PatchMappedConsOperator::cellUJToCellU(FArrayBox& a_UavgFab,
                                            const FArrayBox& a_UJavgFab) const
{
  CH_assert(m_isCurrentBoxSet);
  /*
    Get <J>.
    a_UavgFab has m_numGhost ghosts
    a_UJavgFab has m_numGhost ghosts
    Therefore, a_UavgFab can only be computed on m_numGhost-1 ghosts and J needs
    at most size m_numGhosts.
  */
  Box grownBox2 = grow(m_currentBox, m_numGhost + 2);
  FArrayBox JavgFab(grownBox2, 1);
  m_coordSysPtr->getAvgJ(JavgFab, grownBox2);
  //  Box solveBox = grow(m_currentBox, m_numGhost - 1);

  // return <U> = <UJ>/<J> - h^2/12 * (grad (<UJ>/<J>)) dot (grad <J>) / <J>
  // as described in ESL paper:  Colella, Dorr, Hittinger, and Martin,
  // "High-order, finite-volume methods in mapped coordinates"

  //  cellFGToCellF(a_UavgFab, a_UJavgFab, JavgFab, solveBox);
  int ncomp = a_UavgFab.nComp();
  // Set JinvFab = 1.0 / Javgfab .
  FArrayBox JinvFab(JavgFab.box(), 1);
  JinvFab.copy(JavgFab);
  JinvFab.invert(1.0);

  Box intersectBox(a_UJavgFab.box());
  intersectBox &= JinvFab.box();
  intersectBox &= a_UavgFab.box();

  // Initially set <U> = <UJ>/<J> on intersectBox.
  a_UavgFab.copy(a_UJavgFab, intersectBox);
  for (int comp = 0; comp < ncomp; comp++)
    {
      // Multiply a_UavgFab[comp] by JinvFab[0].
      a_UavgFab.mult(JinvFab, intersectBox, 0, comp);
    }

  // compute box over which we can do this
  Box gradIntersectBox(a_UavgFab.box());
  gradIntersectBox.grow(-1);
  gradIntersectBox &= JavgFab.box();
  // added by petermc, 13 Sep 2010
  Box interiorBox(gradIntersectBox);
  interiorBox &= m_domain;
  secondOrderCellExtrapAtDomainBdry(a_UavgFab, interiorBox, m_domain);
  // Set gradProduct = factor * grad(a_UavgFAb) dot grad(JavgFab).
  FArrayBox gradProduct(gradIntersectBox, ncomp);
  gradProduct.setVal(0.);
  // since dx's cancel, use 1 here
  Real fakeDx = 1.0;
  Real factor = -1.0/12.0; // (dfm - 3/22/09)--WAS 12 instead of 24 -- No, change back!
  for (int dir=0; dir<SpaceDim; dir++)
    {
      // This function is in fourthOrderMappedGrids/util/FourthOrderUtil.cpp
      // sets gradProduct += factor * d(a_UavgFab)/dx[dir] * d(JavgFab)/dx[dir]
      incrementGradProduct(gradProduct,
                           a_UavgFab,
                           JavgFab,
                           gradIntersectBox,
                           fakeDx,
                           factor,
                           dir);
    }

  for (int comp = 0; comp < ncomp; comp++)
    {
      // Multiply gradProduct[comp] by JinvFab[0].
      gradProduct.mult(JinvFab, gradIntersectBox, 0, comp);
    }
  // Now gradProduct = factor * grad(a_UavgFAb) dot grad(JavgFab) / JFab.

  a_UavgFab.plus(gradProduct, gradIntersectBox, 0, 0, ncomp);
}


//////////////////////////////////////////////////////////////////////////////

void
PatchMappedConsOperator::computeCompFaceFluxes( FluxBox& a_uTimesV,
                                                const FluxBox& a_u,
                                                const FluxBox& a_v) const
{
   // Compute the SpaceDim-by-nComp face-averaged fluxes in computational
   // space, where a_v is the SpaceDim-dimensional velocity vector and
   // a_u is the nComp-dim state vector
   int ncomp = a_u.nComp();
   CH_assert(a_v.nComp() == SpaceDim);
   CH_assert(a_uTimesV.nComp() == SpaceDim * ncomp);

   // loop over faces (index "d" in notes)
   for (int faceDir=0; faceDir<SpaceDim; faceDir++)
     {
       FArrayBox& thisUVdir = a_uTimesV[faceDir];
       const FArrayBox& thisUdir = a_u[faceDir];
       const FArrayBox& thisVdir = a_v[faceDir];

       // compute <u_p><v_d> tensor
       Box intersectBox(thisUdir.box());
       intersectBox &= thisVdir.box();
       intersectBox &= thisUVdir.box();

       thisUVdir.setVal(0.0);
       FORT_INCREMENTFACEPROD(CHF_FRA(thisUVdir),
                              CHF_CONST_FRA(thisUdir),
                              CHF_CONST_FRA(thisVdir),
                              CHF_BOX(intersectBox));

       if (m_spaceOrder == 4)
         {
           // now increment with product of tangential gradients
           // (sum over index "d'" in notes)
           for (int tanDir=0; tanDir<SpaceDim; tanDir++)
             {
               if (tanDir!=faceDir)
                 {
                   // since dx's cancel, use 1 here
                   Real fakeDx = 1.0;
                   Real factor = 1.0/12.0;

                   // compute box over which we can do this
                   Box gradIntersectBox(thisUdir.box());
                   gradIntersectBox &= thisVdir.box();
                   gradIntersectBox.grow(tanDir,-1);
                   gradIntersectBox &= thisUVdir.box();

                   FORT_INCREMENTFACEPRODGRAD(CHF_FRA(thisUVdir),
                                              CHF_CONST_FRA(thisUdir),
                                              CHF_CONST_FRA(thisVdir),
                                              CHF_BOX(gradIntersectBox),
                                              CHF_REAL(fakeDx),
                                              CHF_REAL(factor),
                                              CHF_INT(tanDir));

                 } // end if tangential direction
             } // end loop over tanDir
         } // end if fourth order
     } // end loop over faceDir
}

#include "NamespaceFooter.H"
