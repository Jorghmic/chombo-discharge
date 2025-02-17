#!/bin/bash
#SBATCH --account=nn9887k
#SBATCH --job-name=TriplePointInception
#SBATCH --time=0-02:00:00
#SBATCH --nodes=40
#SBATCH --ntasks-per-node=128

## Safety settings, I don't know why this is recommended in the documentation but fine
set -o errexit
set -o nounset

## Software modules
module restore system
module load foss/2021a
module load HDF5/1.10.7-gompi-2021a

# Program to run and input script
executable=program3d.Linux.64.mpic++.gfortran.OPTHIGH.MPI.ex

# Input files. Must be copied to where we run the simulation. 
input_file=simulation.inputs
transport_file=transport_data.dat

# Create directory, copy executable and input script
workdir=${USERWORK}/${SLURM_JOB_NAME}
if [ ! -d $workdir ]; then mkdir -p $workdir; fi

# Only pout.0
export CH_OUTPUT_INTERVAL=9999999

# Copy executables to work directory
cp -f -L $executable $workdir/$executable
cp -f -L $input_file $workdir/$input_file
cp -f -L $transport_file $workdir/$transport_file

# Lustre striping
lfs setstripe --stripe-count 8 --stripe-size 64M $workdir

# Navigate to directory and run
cd $workdir

# The base cases uses a 40um gap between the electrode and the insulator. Here are the settings required for changing
# to the values in the paper
#  5	um: MechanicalShaft.electrode.inner.radius=1.75050001E-2
# 10	um: MechanicalShaft.electrode.inner.radius=1.7510001E-2
# 20	um: MechanicalShaft.electrode.inner.radius=1.7520001E-2
# 40	um: MechanicalShaft.electrode.inner.radius=1.7540001E-2
# 80	um: MechanicalShaft.electrode.inner.radius=1.7580001E-2
# 160	um: MechanicalShaft.electrode.inner.radius=1.7660001E-2
# 320	um: MechanicalShaft.electrode.inner.radius=1.7820001E-2
# 640	um: MechanicalShaft.electrode.inner.radius=1.8140001E-2
# 1280	um: MechanicalShaft.electrode.inner.radius=1.8780001E-2
#
# mpirun ./$executable $input_file DischargeInceptionStepper.output_file = 5um.txt    MechanicalShaft.electrode.inner.radius=1.7505001E-2
# mpirun ./$executable $input_file DischargeInceptionStepper.output_file = 10um.txt   MechanicalShaft.electrode.inner.radius=1.7510001E-2
# mpirun ./$executable $input_file DischargeInceptionStepper.output_file = 20um.txt   MechanicalShaft.electrode.inner.radius=1.7520001E-2
mpirun ./$executable $input_file DischargeInceptionStepper.output_file = 40um.txt   MechanicalShaft.electrode.inner.radius=1.7540001E-2
# mpirun ./$executable $input_file DischargeInceptionStepper.output_file = 80um.txt   MechanicalShaft.electrode.inner.radius=1.7580001E-2
# mpirun ./$executable $input_file DischargeInceptionStepper.output_file = 160um.txt  MechanicalShaft.electrode.inner.radius=1.7660001E-2
# mpirun ./$executable $input_file DischargeInceptionStepper.output_file = 320um.txt  MechanicalShaft.electrode.inner.radius=1.7820001E-2
# mpirun ./$executable $input_file DischargeInceptionStepper.output_file = 640um.txt  MechanicalShaft.electrode.inner.radius=1.8140001E-2
# mpirun ./$executable $input_file DischargeInceptionStepper.output_file = 1280um.txt MechanicalShaft.electrode.inner.radius=1.8780001E-2
