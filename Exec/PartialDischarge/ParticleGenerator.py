import numpy as np
import random

def drawCircleDistribution(X,Y,R,N,W):
    """
    Draw particles with 2D coordinates inside the circle with center at X,Y and radius R. 

    :param X: Center of the sphere along the Cartesian x-axis.
    :param Y: Center of the sphere along the Cartesian y-axis.
    :param R: Radius of the sphere.
    :param N: Number of particles to sample. 
    :param W: Particle weight. 

    :return: Returns the particles in the form of a numpy array with N entries. Each entry corresponds to a particle represented as (x,y,w).
    """
    
    particles = np.zeros(shape=(N,3))

    ipart = 0
    while(ipart < N):

        x = X + 2.0 * (random.random()-0.5) * R
        y = Y + 2.0 * (random.random()-0.5) * R

        if(pow(x-X,2) + pow(y-Y,2) < pow(R,2)):
            particles[ipart] = [x,y,W]
            ipart += 1

    return particles

def drawSphereDistribution(X,Y,Z,R,W,N):
    """
    Draw particles with 3D coordinates inside the sphere with center at X,Y,Z and radius R. 

    :param X: Center of the sphere along the Cartesian x-axis.
    :param Y: Center of the sphere along the Cartesian y-axis.
    :param Z: Center of the sphere along the Cartesian z-axis.
    :param R: Radius of the sphere.
    :param W: Particle weight. 
    :param N: Number of particles to sample. 

    :return: Returns the particles in the form of a numpy array with N entries. Each entry corresponds to a particle represented as (x,y,z,w).
    """
    
    particles = np.zeros(shape=(N,4))

    ipart = 0
    while(ipart < N):

        x = X + 2.0 * (random.random()-0.5) * R
        y = Y + 2.0 * (random.random()-0.5) * R
        z = Z + 2.0 * (random.random()-0.5) * R

        if(pow(x-X,2) + pow(y-Y,2) + pow(z-Z,2) < pow(R,2)):
            particles[ipart] = [x,y,z,W]
            ipart += 1

    return particles


# Set the random seed for reproducibility reasons
random.seed(0)

# Here is an example that samples the initial particles and stores them to a file in the format (x,y,z,w) where w is the particle weight.
particles = drawSphereDistribution(X = 0.0175,
                                   Y = 0.01,
                                   Z = 0.0,
                                   R = 200E-6,
                                   N = 1000,
                                   W = 1.0)

np.savetxt("initial_particles.txt",
           particles,
           fmt='%18.11E',
           header="Particles are stored as x,y,z,w")

