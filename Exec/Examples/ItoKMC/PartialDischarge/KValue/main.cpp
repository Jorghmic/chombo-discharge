#include <CD_Driver.H>
#include <CD_MechanicalShaft.H>
#include <CD_DischargeInceptionStepper.H>
#include <CD_DischargeInceptionTagger.H>

using namespace ChomboDischarge;
using namespace Physics::DischargeInception;

int
main(int argc, char* argv[])
{
  ChomboDischarge::initialize(argc, argv);

  auto alphaTable = DataParser::simpleFileReadASCII("alpha.dat");
  auto etaTable   = DataParser::simpleFileReadASCII("eta.dat");

  alphaTable.prepareTable(0, 1000, LookupTable::Spacing::Exponential);
  etaTable.prepareTable(0, 1000, LookupTable::Spacing::Exponential);

  const Real N = 1E5 / (Units::kb * 293);

  auto E2EN = [N](const Real E) -> Real {
    return E / N * 1E21;
  };

  auto alpha = [&](const Real& E, const RealVect& x) -> Real {
    return alphaTable.interpolate<1>(E2EN(E)) * N;
  };
  auto eta = [&](const Real& E, const RealVect& x) -> Real {
    return etaTable.interpolate<1>(E2EN(E)) * N;
  };
  auto alphaEff = [&](const Real& E, const RealVect& x) -> Real {
    return alpha(E, x) - eta(E, x);
  };
  auto bgRate = [&](const Real& E, const RealVect& x) -> Real {
    return 0.0;
  };
  auto detachRate = [&](const Real& E, const RealVect& x) -> Real {
    return 0.0;
  };
  auto fieldEmission = [&](const Real& E, const RealVect& x) -> Real {
    return 0.0;
  };
  auto secondCoeff = [&](const Real& E, const RealVect& x) -> Real {
    return 0.0;
  };
  auto ionMobility = [&](const Real& E) -> Real {
    return 0.0;
  };
  auto ionDiffusion = [&](const Real& E) -> Real {
    return 0.0;
  };
  auto ionDensity = [&](const RealVect& x) -> Real {
    return 0.0;
  };
  auto voltageCurve = [&](const Real& time) -> Real {
    return 1.0;
  };

  auto compgeom    = RefCountedPtr<ComputationalGeometry>(new MechanicalShaft());
  auto amr         = RefCountedPtr<AmrMesh>(new AmrMesh());
  auto timestepper = RefCountedPtr<DischargeInceptionStepper<>>(new DischargeInceptionStepper<>());
  auto celltagger  = RefCountedPtr<DischargeInceptionTagger>(
    new DischargeInceptionTagger(amr, timestepper->getElectricField(), alphaEff));
  auto driver = RefCountedPtr<Driver>(new Driver(compgeom, timestepper, amr, celltagger));

  timestepper->setAlpha(alpha);
  timestepper->setEta(eta);
  timestepper->setBackgroundRate(bgRate);
  timestepper->setDetachmentRate(detachRate);
  timestepper->setFieldEmission(fieldEmission);
  timestepper->setSecondaryEmission(secondCoeff);
  timestepper->setIonMobility(ionMobility);
  timestepper->setIonDiffusion(ionDiffusion);
  timestepper->setIonDensity(ionDensity);
  timestepper->setVoltageCurve(voltageCurve);

  driver->setupAndRun();

  ChomboDischarge::finalize();
}
