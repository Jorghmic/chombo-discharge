#!/usr/bin/env python3
import math
import subprocess
import sys
import re
from pathlib import Path
import tempfile
import os

# ==============================
# User configuration
# ==============================
INPUT_FILE = Path("example.inputs")

JOB_NAME = "TriplePo1"
USER = "jorgehm"

PARAM_POTENTIAL = "ItoKMC.potential"
PARAM_RESTART = "Driver.restart"
PARAM_OUTPUT = "Driver.output_names"
PARAM_MAXSTEPS = "Driver.max_steps"

CHECKPOINT_DIR = Path(f"/cluster/home/{USER}/work/{JOB_NAME}/chk")
SLURM_TEMPLATE = """#!/bin/bash
#SBATCH --account=nn9636k
#SBATCH --job-name={job_name}
#SBATCH --time=0-0:40:00
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

input_file={input_file}
chemistry_file=chemistry.json
particle_file=initial_particles.dat
bolsig_air=bolsig_air.dat
electron_transport=electron_transport.dat
ion_transport=ion_transport.dat

workdir=${{USERWORK}}/{job_name}
mkdir -p "$workdir"

export CH_OUTPUT_INTERVAL=9999999

cp -f $executable $workdir/
cp -f $input_file $workdir/
cp -f $chemistry_file $workdir/
cp -f -L $particle_file $workdir/
cp -f $bolsig_air $workdir/
cp -f $electron_transport $workdir/
cp -f $ion_transport $workdir/




lfs setstripe --stripe-count 8 --stripe-size 64M $workdir

cd $workdir
mpirun ./$executable $input_file Driver.initial_regrids=1
"""

# ==============================
# Inputs-file helpers
# ==============================
def read_inputs():
    return INPUT_FILE.read_text()


def set_param(text, name, value):
    pattern = rf"^{name}\s*=.*$"
    replacement = f"{name} = {value}"

    if re.search(pattern, text, re.MULTILINE):
        return re.sub(pattern, replacement, text, flags=re.MULTILINE)
    else:
        return text + f"\n{name} = {value}\n"


def get_param(text, name):
    match = re.search(
        rf"^{name}\s*=\s*([^\n#]+)",
        text,
        re.MULTILINE,
    )
    if not match:
        raise RuntimeError(f"{name} not found in inputs file")
    return float(match.group(1))


# ==============================
# SLURM submission helpers
# ==============================
def write_slurm_file(input_file):
    tmp = tempfile.NamedTemporaryFile(
        mode="w", delete=False, suffix=".slurm"
    )
    tmp.write(SLURM_TEMPLATE.format(job_name=JOB_NAME, input_file=input_file))
    tmp.close()
    return tmp.name


def submit_job(slurm_file, dependency=None):
    cmd = ["sbatch"]
    if dependency:
        cmd.append(f"--dependency=afterok:{dependency}")
    cmd.append(slurm_file)

    out = subprocess.check_output(cmd, text=True)
    return out.strip().split()[-1]

# ==============================
# Main logic
# ==============================
def main(phases):
    prev_job = None

    for idx, phase in enumerate(phases):
        phase = float(phase)

        text = read_inputs()
        potential = get_param(text, PARAM_POTENTIAL)
        max_steps = get_param(text, PARAM_MAXSTEPS)

        new_potential = potential * math.sin(math.radians(phase))

        restart = max_steps * idx

        print(
            f"Phase {phase:6.1f}: "
            f"{potential:.3e} × sin({phase}°) → {new_potential:.3e}"
        )
        print(f"  Driver.restart      = {restart}")

        text = set_param(text, PARAM_POTENTIAL, f"{new_potential:.8e}")
        text = set_param(text, PARAM_RESTART, int(restart))

        # Generate a new input file for this job
        new_input_file = f"{JOB_NAME}_inputs_{int(phase)}.inputs"
        Path(new_input_file).write_text(text)
        slurm_file = write_slurm_file(new_input_file)
        job_id = submit_job(slurm_file, prev_job)
        print(f"  submitted job {job_id} using {new_input_file}\n")

        prev_job = job_id

    print("All dependent jobs submitted successfully.")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(1)

    main(sys.argv[1:])
