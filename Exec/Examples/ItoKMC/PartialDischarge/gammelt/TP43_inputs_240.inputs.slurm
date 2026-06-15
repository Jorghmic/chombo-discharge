#!/bin/bash
#SBATCH --account=nn9636k
#SBATCH --job-name=TP43_240
#SBATCH --time=0-0:30:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=128
#SBATCH --qos=preproc
#SBATCH --mem-per-cpu=1000M

set -o errexit
set -o nounset
set -o pipefail

module restore system
module load foss/2023a
module load HDF5/1.14.0-gompi-2023a
module load OpenBLAS/0.3.23-GCC-12.3.0

executable=main2d.Linux.64.mpic++.gfortran.OPTHIGH.MPI.ex

input_file=TP43_inputs_240.inputs
chemistry_file=chemistry.json
particle_file=initial_particles.dat
bolsig_air=bolsig_air.dat
electron_transport=electron_transport.dat
ion_transport=ion_transport.dat

workdir=${USERWORK}/TP43_240
mkdir -p "$workdir"

export CH_OUTPUT_INTERVAL=9999999

cp -f $executable $workdir/
cp -f $input_file $workdir/
cp -f $chemistry_file $workdir/
cp -f -L $particle_file $workdir/
cp -f $bolsig_air $workdir/
cp -f $electron_transport $workdir/
cp -f $ion_transport $workdir/

base_job_name="TP43_90"

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

# ---------------------------------------------------------------------
# AUTO-DETECT RESTART (DEPENDENT JOBS ONLY)
# ---------------------------------------------------------------------

restart_override_args=""

if [ -n "$base_job_name" ]; then
  chkdir="$workdir/chk"

  if [ ! -d "$chkdir" ]; then
    echo "ERROR: chk directory missing after copy: $chkdir" >&2
    exit 1
  fi

  # Find highest sim.checkXXXX.2d.hdf5
  maxchk=$(ls "$chkdir"/sim.check*.2d.hdf5 2>/dev/null            | sed -E 's/.*sim\.check0*([0-9]+)\.2d\.hdf5//'            | sort -n            | tail -1)

  if [ -z "$maxchk" ]; then
    echo "ERROR: No checkpoint files found in $chkdir" >&2
    exit 1
  fi

  block_steps=$(awk -F= '/^[[:space:]]*Driver\.max_steps[[:space:]]*=/{gsub(/[[:space:]]/,"",$2); print $2; exit}' "$input_file")

  if [ -z "$block_steps" ]; then
    echo "ERROR: Could not read Driver.max_steps from $input_file" >&2
    exit 1
  fi

  new_max_steps=$((maxchk + block_steps))

  echo "Auto-detected restart from chk/: restart=$maxchk"
  echo "Setting max_steps = restart + block_steps = $new_max_steps"

  restart_override_args="Driver.restart=$maxchk Driver.max_steps=$new_max_steps"
fi

mpirun ./$executable $input_file Driver.initial_regrids=1 $restart_override_args
