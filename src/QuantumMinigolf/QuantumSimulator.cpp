#include "QuantumSimulator.h"
#include "QuantumMinigolf.h"

#define INTENS 120 // color intensity at maximal probability density

// constructor: setup the FFT engine and compute the Momentum Propagator
QuantumSimulator::QuantumSimulator(int width, int height, double dt) {
	this->dt = dt;
	this->width = width;
	this->height = height;
	
	psi = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * width * height);
	prop = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * width * height);
	xprop = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * width * height);
	
	LOG("Initializing FFT engine ... ");
	fft = fftwf_plan_dft_2d(width, height,
			psi, psi, FFTW_FORWARD, FFTW_MEASURE);
	LOG("done");
	
	LOG("Initializing inverse FFT engine ... ");
	ifft = fftwf_plan_dft_2d(width, height,
			psi, psi, FFTW_BACKWARD, FFTW_MEASURE);
	        
	BuildMomentumPropagator();
	
	// construct a dummy position propagator. The right propagator
	// is constructed from the track, once the user has made its choice
	// where to play.
	
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			xprop[x*height+y][0] = 0;
			xprop[x*height+y][1] = 0;
		}
	}
	
	LOG("done");
	
	Clear();
			
	GaussNorm = 0;
}

QuantumSimulator::~QuantumSimulator(void) {
	fftwf_free(psi);
	fftwf_free(prop);
	
	fftwf_free(xprop);
}

void QuantumSimulator::Clear() {
	for (int i = 0; i < width; i++)
		for (int j = 0; j < height; j++)
			psi[i*height + j][0] = psi[i*height + j][1] = 0;
}

void QuantumSimulator::BuildMomentumPropagator() {
	int x, y;
	double yscale = width / height * width / height; // scale factor to compensate for different
	// k_0 in x and y direction due to different dimensions
	
	for (x = 0; x < width / 2; x++) {
		for (y = 0; y < height / 2; y++) {
			prop[x*height+y][0] = cos(dt * (-x * x - yscale * y * y));
			prop[x*height+y][1] = sin(dt * (-x * x - yscale * y * y));
		}
		
		for (y = height / 2; y < height; y++) {
			prop[x*height+y][0] = cos(dt * (-x * x - yscale * (y - height) * (y - height)));
			prop[x*height+y][1] = sin(dt * (-x * x - yscale * (y - height) * (y - height)));
		}
	}
	
	for (x = width / 2; x < width; x++) {
		for (y = 0; y < height / 2; y++) {
			prop[x*height+y][0] = cos(dt * (-(x - width) * (x - width) - yscale * y * y));
			prop[x*height+y][1] = sin(dt * (-(x - width) * (x - width) - yscale * y * y));
		}
		
		for (y = height / 2; y < height; y++) {
			prop[x*height+y][0] = cos(dt * (-(x - width) * (x - width) - yscale * (y - height) * (y - height)));
			prop[x*height+y][1] = sin(dt * (-(x - width) * (x - width) - yscale * (y - height) * (y - height)));
		}
	}
}

// BuildPositionPropagator - build up the position from a bitmap
// with color-coded obstacle height
void QuantumSimulator::BuildPositionPropagator(const Image& V) {
	const RGBA *V_dat = V.Begin();
	ASSERT(V_dat);
	
	// extract the potential
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			uint8 dummy, red;
			
			red = V_dat->r;
			
			xprop[x*height+y][0] = cos(-.5 * (double)(red) * dt * 30000 / 255);
			xprop[x*height+y][1] = sin(-.5 * (double)(red) * dt * 30000 / 255);
			
			if (red > 250) {
				xprop[x*height+y][0] = 0;
				xprop[x*height+y][1] = 0;
			}
			
			V_dat++;
		}
	}
}

//PropagateMomentum -- FFT into k-space and apply the momentum propagator
// to the wave function
// effectively, this propagates the wavefunction by dt in a zero potential
void QuantumSimulator::PropagateMomentum() {
	volatile double tre, tim, pre, pim; // swap register and propagator real and imaginary parts
	volatile int x, y;
	
	// propagate in momentum space
	fftwf_execute(fft);
	
	for (x = 0; x < width; x++) {
		for (y = 0; y < height; y++) {
			tre = psi[x*height+y][0];
			tim = psi[x*height+y][1];
			pre = prop[x*height+y][0];
			pim = prop[x*height+y][1];
			
			psi[x*height+y][0] = tre * pre - tim * pim;
			psi[x*height+y][1] = tre * pim + tim * pre;
		}
	}
	
	fftwf_execute(ifft);
}


//PropagatePosition -- propagate in position space
// and scale the wavefunction by a factor of quench
// note that this operation is not unitary due to the
// hard erase at infinite potentials
// return value: the new norm of the propagated wavefunction
double QuantumSimulator::PropagatePosition(double quench) {
	double norm = 0;
	volatile double tre, tim, pre, pim, dnorm, mnorm = 0;
	volatile int x, y;
	
	for (x = 0; x < width; x++) {
		for (y = 0; y < height; y++) {
			tre = psi[x*height+y][0];
			tim = psi[x*height+y][1];
			
			pre = xprop[x*height+y][0];
			pim = xprop[x*height+y][1];
			
			if (pre == 0 && pim == 0) {
				psi[x*height+y][0] = 0;
				psi[x*height+y][1] = 0;
			}
			
			else {
				//propagate the wavefunction.
				// and correct for last time's shrink and the
				// FFT's scaling
				psi[x*height+y][0] = quench * (tre * pre - tim * pim);
				psi[x*height+y][1] = quench * (tim * pre + tre * pim);
			}
			
			dnorm = (psi[x*height+y][0] * psi[x*height+y][0] +
			
					 psi[x*height+y][1] * psi[x*height+y][1]);
			         
			norm += dnorm;
			
		}
	}
	
	norm /= GaussNorm * INTENS * INTENS;
	
	return norm;
}


//PositionMeasurement
// performe a position measurement, i.e., randomly pick a point x, y
// according to the probability distribution defined by the wavefunction psi
void QuantumSimulator::PositionMeasurement(int *x, int *y) {
	double norm = 0;
	double psi2;
	double cutoff = 1e-6;
	double criterion;
	double sucprob = 0;
	
	int runs = 0;
	
	int holex = 100, holey = 160;
	int holer = 30;
	
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			double psi2 = psi[i*height+j][0] * psi[i*height+j][0] +
						  psi[i*height+j][1] * psi[i*height+j][1];
			norm += psi2 * psi2;
		}
	}
	
	for (int i = holex - holer; i < holex + holer; i++) {
		for (int j = holey - holer; j < holey + holer; j++) {
			double psi2 = psi[i*height+j][0] * psi[i*height+j][0] +
						  psi[i*height+j][1] * psi[i*height+j][1];
			sucprob += psi2 * psi2;
		}
	}
	
	sucprob /= norm;
	
	do {
		*x = rand() % width;
		*y = rand() % height;
		psi2 = psi[*x*height+*y][0] * psi[*x*height+*y][0] +
			   psi[*x*height+*y][1] * psi[*x*height+*y][1];
		criterion = ((double)(rand()) / RAND_MAX) + cutoff;
		runs++;
	}
	
	while (psi2*psi2 / norm / 2 < criterion);
	
}

// GenGauss
// generate a coherent state (i.e. a Gaussian wavepacket centered around
// cx, cy in position and around kx, ky in momentum space)
void QuantumSimulator::GenGauss(int cx, int cy, double kx, double ky, double w) {
	int x, y, xeff, yeff;
	double r;
	
// commented out for uncertainty movie 070519
	GaussNorm = 0;
	
	int xlower = (int)(cx - 2.5 * w);
	
	if (xlower < 0)
		xlower = 0;
		
	int ylower = (int)(cy - 2.5 * w);
	
	if (ylower < 0)
		ylower = 0;
		
	int xupper = (int)(cx + 2.5 * w);
	
	if (xupper > width)
		xupper = width;
		
	int yupper = (int)(cy + 2.5 * w);
	
	if (yupper > height)
		yupper = height;
		
	for (x = xlower; x < xupper; x++) {
		xeff = x - cx;
		
		for (y = ylower; y < yupper; y++) {
			yeff = y - cy;
			r = exp(-.25 * (xeff * xeff + yeff * yeff) / w / w);
			psi[height*x+y][0] = r * cos(kx * xeff + ky * yeff);
			psi[height*x+y][1] = r * sin(kx * xeff + ky * yeff);
			GaussNorm += psi[height*x+y][0] * psi[height*x+y][0] +
						 psi[height*x+y][1] * psi[height*x+y][1];
		}
	}
	
	for (x = xlower; x < xupper; x++) {
		for (y = ylower; y < yupper; y++) {
			psi[height*x+y][0] *= INTENS;
			psi[height*x+y][1] *= INTENS;
		}
	}
}

//ClearWave - initialize psi with zeros
void QuantumSimulator::ClearWave(void) {
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			psi[height*x+y][0] = psi[height*x+y][1] = 0;
		}
	}
}
