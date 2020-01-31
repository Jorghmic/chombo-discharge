      subroutine setprob
      implicit double precision (a-h,o-z)
      common /cparam/ rho,bulk,cc,zz
      common /cqinit/ beta
      common /comsrc/ ndim
      common /initq/ width
c
c     # Set the material parameters for the acoustic equations
c     # Passed to the Riemann solver rp1.f in a common block
c
      open(unit=7,file='setprob.data',status='old',form='formatted')

c     # density:
      read(7,*) rho

c     # bulk modulus:
      read(7,*) bulk

      read(7,*) width

c     # sound speed:
      cc = dsqrt(bulk/rho)

c     # impedance:
      zz = cc*rho

c     # ndim = space dimensions (2 = cylindrical symmetry, 3 = spherical)
      read(7,*) ndim



      return
      end
