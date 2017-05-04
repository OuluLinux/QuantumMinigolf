#pragma once

// QuantumSimulator - the class implementing the numerical solution of the
// time-dependent Schrödinger equation.
// The solution is based on a time-splitting scheme.
// The propagation is performed successively in momentum and position space
// To propagate in momentum space, the wave function is fourier-transformed.

#include <fftw3.h>

#include <CtrlLib/CtrlLib.h>
using namespace Upp;


class QuantumSimulator {

public:
	QuantumSimulator(int width, int height, double dt);
	
	void Clear();
	
	void BuildPositionPropagator(const Image& V);
	void BuildMomentumPropagator();
	
	double PropagatePosition(double quench);
	void PropagateMomentum();
	
	// return the result of a position measurement on psi
	void PositionMeasurement(int *x, int *y);
	
	// GenGauss - initialize psi with a gaussian wavepacket of width w,
	// centered around cx and cy in position and around kx and ky in momentum space
	void GenGauss(int cx, int cy, double kx, double ky, double w);
	void ClearWave(void);
	
	fftwf_complex *psi; // the complex wavefunction
	fftwf_complex *xprop; // the propagator in position space
	
public:
	~QuantumSimulator(void);
	
private:
	fftwf_complex *prop; // the propagator in momentum space
	
	fftwf_plan fft, ifft; // plans for the Fourier transformations
	// into momentum and position space
	double dt;			// the timestep
	int width, height;
	double GaussNorm;		// Norm of the wave packet after initialization
	
};
