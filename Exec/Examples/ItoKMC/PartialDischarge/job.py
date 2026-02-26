#!/usr/bin/env python3
import argparse
import os
import subprocess
from pathlib import Path
from textwrap import dedent


def sbatch_submit(script_text: str, *, cwd: Path, extra_args=None) -> str:
    cmd = ["sbatch", "--parsable"]
    if extra_args:
        cmd.extend(extra_args)
    p = subprocess.run(
        cmd,
        input=script_text,
        cwd=str(cwd),
        capture_output=True,
        text=True,
        check=True,
        env=os.environ.copy(),
    )
    return p.stdout.strip().split(";")[0]


def make_prep_script(args, workdir: Path) -> str:
    return dedent(f"""\
    #!/bin/bash
    #SBATCH --account={args.account}
    #SBATCH --job-name={args.job_name}_prep
    #SBATCH --time={args.prep_time}
    #SBATCH --nodes=1
    #SBATCH --ntasks=1
    #SBATCH --qos={args.qos}
    #SBATCH --mem=1G
    #SBATCH --output={workdir}/slurm-%x-%j.out

    set -euo pipefail

    module restore system
    module load foss/2023a
    module load HDF5/1.14.0-gompi-2023a
    module load OpenBLAS/0.3.23-GCC-12.3.0

    mkdir -p {workdir}

    cp -f {args.executable} {workdir}/{args.executable}
    cp -f {args.input_file} {workdir}/{args.input_file}
    cp -f {args.chemistry_file} {workdir}/{args.chemistry_file}
    cp -f -L {args.particle_file} {workdir}/{args.particle_file}
    cp -f {args.bolsig_air} {workdir}/{args.bolsig_air}

    if command -v lfs >/dev/null 2>&1; then
      lfs setstripe --stripe-count {args.stripe_count} --stripe-size {args.stripe_size} {workdir} || true
    fi

    echo "Prep done in {workdir}"
    """)


def make_array_script(args, workdir: Path, *, stage: str) -> str:
    """
    stage = "pos"  : potential = sin(phase)
    stage = "shift": potential = sin(phase + PHASE_SHIFT_DEG)
    """
    array_spec = f"0-{args.runs-1}%{args.max_parallel}" if args.runs > 1 else "0-0"

    return dedent(f"""\
    #!/bin/bash
    #SBATCH --account={args.account}
    #SBATCH --job-name={args.job_name}_{stage}
    #SBATCH --time={args.run_time}
    #SBATCH --nodes={args.nodes}
    #SBATCH --ntasks-per-node={args.ntasks_per_node}
    #SBATCH --qos={args.qos}
    #SBATCH --mem-per-cpu={args.mem_per_cpu}
    #SBATCH --array={array_spec}
    #SBATCH --output={workdir}/slurm-%x-%A_%a.out

    set -euo pipefail

    module restore system
    module load foss/2023a
    module load HDF5/1.14.0-gompi-2023a
    module load OpenBLAS/0.3.23-GCC-12.3.0

    export CH_OUTPUT_INTERVAL=9999999

    WORKDIR={workdir}
    EXE="$WORKDIR"/{args.executable}
    BASE_INPUT="$WORKDIR"/{args.input_file}

    RUNID=$(printf "%03d" "${{SLURM_ARRAY_TASK_ID}}")
    RUNDIR="$WORKDIR/run_$RUNID"
    POSDIR="$RUNDIR/pos"
    SHIFTDIR="$RUNDIR/shift"
    mkdir -p "$POSDIR" "$SHIFTDIR"

    if [ "{stage}" = "pos" ]; then
      STAGEDIR="$POSDIR"
    else
      STAGEDIR="$SHIFTDIR"
    fi

    # Copy executable + shared inputs into t
