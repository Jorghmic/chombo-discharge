#!/bin/bash
#SBATCH --account=nn9636k
#SBATCH --job-name=TP05_200
#SBATCH --time=0-0:10:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=128
#SBATCH --qos=preproc
#SBATCH --mem-per-cpu=1000M

set -o errexit
set -o nounset

module restore system
module load foss/2023a
module load HDF5/1.14.0-gompi-2023a
module load OpenBLAS/0.3.23-GCC-12.3.0

executable=main2d.Linux.64.mpic++.gfortran.OPTHIGH.MPI.ex

input_file=TP05_inputs_200.inputs
chemistry_file=chemistry.json
particle_file=initial_particles.dat
bolsig_air=bolsig_air.dat
electron_transport=electron_transport.dat
ion_transport=ion_transport.dat

workdir=${USERWORK}/TP05_200
mkdir -p "$workdir"

export CH_OUTPUT_INTERVAL=9999999

cp -f $executable $workdir/
cp -f $input_file $workdir/
cp -f $chemistry_file $workdir/
cp -f -L $particle_file $workdir/
cp -f $bolsig_air $workdir/
cp -f $electron_transport $workdir/
cp -f $ion_transport $workdir/

base_job_name="TP05_90"
if [ -n "$base_job_name" ]; then
  base_workdir=${USERWORK}/$base_job_name
  echo "Dependency job: copying chk from $base_workdir/chk -> $workdir/chk"

  if [ ! -d "$base_workdir/chk" ]; then
    echo "ERROR: base checkpoint directory not found: $base_workdir/chk" >&2
    exit 1
  fi

  mkdir -p "$workdir/chk"
  cp -a "$base_workdir/chk/." "$workdir/chk/"
fi

lfs setstripe --stripe-count 8 --stripe-size 64M $workdir

cd $workdir
mpirun ./$executable $input_file Driver.initial_regrids=1
