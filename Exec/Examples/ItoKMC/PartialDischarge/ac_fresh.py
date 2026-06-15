#!/usr/bin/env python3

import math
import subprocess
import sys
import re
from pathlib import Path

INPUT_FILE = Path("example.inputs")
JOB_NAME = "F31_long"

PARAM_POTENTIAL = "ItoKMC.potential"
PARAM_RESTART = "Driver.restart"
PARAM_MAXSTEPS = "Driver.max_steps"
PARAM_STOPTIME = "Driver.stop_time"

# ---------------------------------------------------------------------
# SLURM TEMPLATE
# ---------------------------------------------------------------------
SLURM_TEMPLATE = r"""#!/bin/bash
#SBATCH --account=nn9636k
#SBATCH --job-name=__JOB_NAME__
#SBATCH --time=0-2:00:00
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
input_file=__INPUT_FILE__
chemistry_file=chemistry.json
particle_file=initial_particles.dat
bolsig_air=bolsig_air.dat
electron_transport=electron_transport.dat
ion_transport=ion_transport.dat

workdir=${USERWORK}/__JOB_NAME__
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

mpirun ./$executable $input_file Driver.initial_regrids=0
"""

# ---------------------------------------------------------------------
# INPUT FILE UTILITIES
# ---------------------------------------------------------------------
def read_inputs():
    return INPUT_FILE.read_text()


def set_param(text, name, value):
    pattern = rf"^{re.escape(name)}\s*=.*$"
    replacement = f"{name} = {value}"

    if re.search(pattern, text, re.MULTILINE):
        return re.sub(pattern, replacement, text, flags=re.MULTILINE)
    else:
        return text + f"\n{name} = {value}\n"


def get_param(text, name):
    match = re.search(
        rf"^{re.escape(name)}\s*=\s*([^\n#]+)",
        text,
        re.MULTILINE,
    )

    if not match:
        raise RuntimeError(f"{name} not found in inputs file")

    return float(match.group(1))


# ---------------------------------------------------------------------
# SLURM FILE GENERATION
# ---------------------------------------------------------------------
def write_slurm_file(input_file, job_name):
    filename = f"{input_file}.slurm"

    content = SLURM_TEMPLATE
    content = content.replace("__JOB_NAME__", job_name)
    content = content.replace("__INPUT_FILE__", input_file)

    with open(filename, "w") as f:
        f.write(content)

    return filename


def submit_job(slurm_file):
    out = subprocess.check_output(
        ["sbatch", slurm_file],
        text=True,
    )

    return out.strip().split()[-1]


# ---------------------------------------------------------------------
# BUILD INPUTS
# ---------------------------------------------------------------------
def build_input_for_phase(
    base_text,
    phase_deg,
    *,
    potential0,
    block_steps,
    stop_time,
):
    new_potential = potential0 * math.sin(math.radians(phase_deg))

    text = base_text

    text = set_param(
        text,
        PARAM_POTENTIAL,
        f"{new_potential:.8e}",
    )

    text = set_param(text, PARAM_RESTART, 0)

    text = set_param(
        text,
        PARAM_MAXSTEPS,
        int(block_steps),
    )

    text = set_param(
        text,
        PARAM_STOPTIME,
        f"{stop_time:.8e}",
    )

    return text, new_potential


# ---------------------------------------------------------------------
# MAIN
# ---------------------------------------------------------------------
def main(phases):

    phases = [float(p) for p in phases]

    base_text = read_inputs()

    potential0 = get_param(base_text, PARAM_POTENTIAL)
    block_steps = int(get_param(base_text, PARAM_MAXSTEPS))
    stop_time = get_param(base_text, PARAM_STOPTIME)

    for phase in phases:

        input_text, new_potential = build_input_for_phase(
            base_text,
            phase,
            potential0=potential0,
            block_steps=block_steps,
            stop_time=stop_time,
        )

        input_file = f"{JOB_NAME}_inputs_{int(phase)}.inputs"

        Path(input_file).write_text(input_text)

        job_name = f"{JOB_NAME}_{int(phase)}"

        slurm_file = write_slurm_file(
            input_file,
            job_name,
        )

        job_id = submit_job(slurm_file)

        print(
            f"Phase {phase:6.1f}: "
            f"{potential0:.3e} × sin({phase}°) "
            f"→ {new_potential:.3e}"
        )

        print(
            f" submitted independent job {job_id}\n"
        )


if __name__ == "__main__":

    if len(sys.argv) < 2:
        print("Usage: python script.py <phase1> [phase2 ...]")
        sys.exit(1)

    main(sys.argv[1:])
