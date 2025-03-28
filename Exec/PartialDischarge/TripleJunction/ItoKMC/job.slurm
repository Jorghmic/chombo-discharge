#!/bin/bash
#SBATCH --account=nn9636k
#SBATCH --job-name=TriplePointItoKMC5
#SBATCH --time=0-12:00:00
#SBATCH --nodes=20
#SBATCH --ntasks-per-node=128

## Safety settings, I don't know why this is recommended in the documentation but fine
set -o errexit
set -o nounset

## Software modules
module restore system
module load foss/2022a
module load HDF5/1.12.2-gompi-2022a

# Program to run and input script
executable=program2d.Linux.64.mpic++.gfortran.OPTHIGH.MPI.ex

# Input files. Must be copied to where we run the simulation. 
input_file=simulation.inputs
transport_file=transport_data.dat
chemistry_file=chemistry.json
particle_file=initial_particles.txt

# Create directory, copy executable and input script
workdir=${USERWORK}/${SLURM_JOB_NAME}
if [ ! -d $workdir ]; then mkdir -p $workdir; fi

# Only pout.0
export CH_OUTPUT_INTERVAL=9999999

# Copy executables to work directory
cp -f $executable $workdir/$executable
cp -f $input_file $workdir/$input_file
cp -f $transport_file $workdir/$transport_file
cp -f $chemistry_file $workdir/$chemistry_file
cp -f -L $particle_file $workdir/$particle_file


# Lustre striping
lfs setstripe --stripe-count 8 --stripe-size 64M $workdir

# Navigate to directory and run
cd $workdir

mpirun ./$executable $input_file Driver.initial_regrids=1
