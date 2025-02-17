Publication
-----------

Title: Towards quantitative partial discharge modeling. 

Authors: Robert Marskar

DOI: ...


Simulation information
----------------------

Application: Physics/DischargeInception/
Application: Physics/ItoKMC/

git hash: 5a0daa861

Simulations were run locally and on Betzy. The required number of CPUs are different for each application, but all cases used <= 512 CPU cores on Betzy.

The simulation data set consists of two separate models that are run on four different cases.
We used both the ItoKMC model and the DischargeInception model.
The simulation data is organized into folders as follows:

* Protrusion/
  * Protrusion/DischargeInception
  * Protrusion/ItoKMC
* Void/
  * Void/DischargeInception
  * Void/ItoKMC
* WireWire/
  * WireWire/DischargeInception
  * WireWire/ItoKMC
* TripleJunction/
  * TripleJunction/DischargeInception
  * TripleJunction/ItoKMC


Each folder is self-contained and contains the relevant transport data, compilable files, and makefiles.
The chemistry model and transport data are common for all the examples; there are symbolic links within each folder pointing to chemistry.json and transport_data.dat (which reside in this folder).
The only difference between the chemistry model in each example is that each ItoKMC application has a separate list of initial particles (electrons). 

All programs are intended for 3D compilation, with exception of the programs in the WireWire folder which are intended as a 2D example.

Compilation instructions
------------------------

Navigate to the relevant example and compile in 2D and 3D using

```
make -s DIM=2 OPT=HIGH program
make -s DIM=3 OPT=HIGH program
```

and run the programs in 2D/3D using

```
mpirun -np<num_cores> program2d.<bunch_of_options>.ex <input_file>
mpirun -np<num_cores> program3d.<bunch_of_options>.ex <input_file>
```
