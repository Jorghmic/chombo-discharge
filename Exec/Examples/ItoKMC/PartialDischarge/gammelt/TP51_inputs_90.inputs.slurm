#!/bin/bash
#SBATCH --account=nn9636k
#SBATCH --job-name=TP51_90
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

input_file=TP51_inputs_90.inputs
chemistry_file=chemistry.json
particle_file=initial_particles.dat
bolsig_air=bolsig_air.dat
electron_transport=electron_transport.dat
ion_transport=ion_transport.dat

workdir=${USERWORK}/TP51_90
mkdir -p "$workdir/chk"

base_job_name=""
base_workdir=${USERWORK}/$base_job_name

restart_override_args=""

cd "$workdir"

# ---------------------------------------------------------------------
# RESTART HANDLING (ROBUST)
# ---------------------------------------------------------------------

if [ -n "$base_job_name" ]; then

  echo "Dependency job detected"
  echo "Copying checkpoint directory..."

  mkdir -p "$workdir/chk"

  rsync -a "$base_workdir/chk/" "$workdir/chk/" || {
    echo "ERROR: rsync failed" >&2
    exit 1
  }

  shopt -s nullglob
  files=("$workdir/chk"/sim.check*.2d.hdf5)

  echo "DEBUG: found ${#files[@]} checkpoint files"

  if [ ${#files[@]} -eq 0 ]; then
    echo "ERROR: No checkpoint files found in $workdir/chk" >&2
    ls -l "$workdir/chk" >&2
    exit 1
  fi

  maxchk=$(printf '%s
' "${files[@]}"     | sed -E 's/.*sim\.check0*([0-9]+)\.2d\.hdf5//'     | sort -n     | tail -1)

  if ! [[ "$maxchk" =~ ^[0-9]+$ ]]; then
    echo "ERROR: invalid maxchk='$maxchk'" >&2
    exit 1
  fi

  block_steps=$(grep -E "^[[:space:]]*Driver\.max_steps" "$input_file"     | head -1     | cut -d= -f2     | tr -cd '0-9')

  if ! [[ "$block_steps" =~ ^[0-9]+$ ]]; then
    echo "ERROR: invalid block_steps='$block_steps'" >&2
    exit 1
  fi

  new_max_steps=$((maxchk + block_steps))

  echo "Restarting from $maxchk"
  echo "Block steps: $block_steps"
  echo "New max_steps: $new_max_steps"

  restart_override_args="Driver.restart=$maxchk Driver.max_steps=$new_max_steps"
fi

lfs setstripe --stripe-count 8 --stripe-size 64M $workdir

# ---------------------------------------------------------------------
# RUN
# ---------------------------------------------------------------------

mpirun ./$executable $input_file Driver.initial_regrids=1 $restart_override_args
