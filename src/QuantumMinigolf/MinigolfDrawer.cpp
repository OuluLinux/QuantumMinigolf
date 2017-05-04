#include "QuantumMinigolf.h"

#include <plugin/bz2/bz2.h>
#include <plugin/png/png.h>
#include "imgs/imgs.brc"

MinigolfDrawer::MinigolfDrawer() :
	simulator(WIDTH, HEIGHT, 0.0001) {
	state = STATE_AIMING;
	hack_state = HACKSTATE_NULL;
	track = NULL;
	
	MemReadStream cmap_mem(cmap_brc, cmap_brc_length);
	cmap = PNGRaster().LoadString(BZ2Decompress(cmap_mem));
	
	MemReadStream cmap_mono_mem(cmap_mono_brc, cmap_mono_brc_length);
	cmap_mono = PNGRaster().LoadString(BZ2Decompress(cmap_mono_mem));
	
	running = true;
	stopped = false;
	Thread::Start(THISBACK(Run));
	
	PostCallback(THISBACK(Refresher));
}

MinigolfDrawer::~MinigolfDrawer() {
	running = false;
	while (!stopped) Sleep(100);
}

void MinigolfDrawer::Refresher() {
	Refresh();
	
	PostCallback(THISBACK(Refresher));
}

void MinigolfDrawer::Run() {
	double normlast = 1;
	double nsq = WIDTH * HEIGHT;
	double vmax = 40; // maximum club speed
	double v = 0;
	
	while (running && !Thread::IsShutdownThreads()) {
		if (state == STATE_AIMING) {
			
		}
		else if (state == STATE_SETVELOCITY) {
			int duration = 1000; // ms
			
			v = 0;
			
			TimeStop ts;
			while (state == STATE_SETVELOCITY) {
				v = (double)ts.Elapsed() / (double)duration;
				
				if (v > 1)
					v = 1;
				
				racket_r = 20 + vmax * v;
				Sleep(10);
			}
		}
		else if (state == STATE_HITTING) {
			
			int duration = 1000; // ms
			double vmax = 40; // maximum club speed
			
			TimeStop ts;
			while (ts.Elapsed() < duration) {
				double elapsed = ts.Elapsed();
				double f = elapsed / duration;
				
				racket_r =
					20 + vmax * v -
					(20 - ballr + vmax * v)* elapsed * elapsed / (double)duration / (double)duration;
				
			}
			state = STATE_MOVING;
			
			
			// commented out for uncertainty movie 070519
			if (hack_state != HACKSTATE_MOVIE) {
				simulator.GenGauss(ballx, bally,
								   -2 * v  *M_PI / 2 * cos(racket_rphi) / 2,
								   -2 * v * M_PI / 2 * sin(racket_rphi) / 2,
								   10);
			} else {
				// hack for uncertainty movie 070519
				simulator.GenGauss( 200, bally,
					-.4,
					0,
					15);
				simulator.GenGauss( 400, bally,
					-.4,
					0,
					6);
				simulator.GenGauss( 600, bally,
					-.4,
					0,
					3);
			}
			
			continue;
		}
		
		else if (state == STATE_MOVING) {
			
			lock.Enter();
			
			if (hack_state != HACKSTATE_SATURATED_FULL) {
				
				// propagate in momentum space
				simulator.PropagateMomentum();
				
				// propagate in position space
				double d = 1 / nsq / sqrt(normlast);
				normlast = simulator.PropagatePosition(d);
				ASSERT(IsFin(normlast));
				
			} else {
				
				// propagate in position space
				double d = 1 / nsq / sqrt(normlast);
				normlast = simulator.PropagatePosition(d);
				ASSERT(IsFin(normlast));
				
				// propagate in momentum space
				simulator.PropagateMomentum();
				
			}
			
			lock.Leave();
		}
		else if (state == STATE_FINISHED) {
			
			
		}
		
		Sleep(10);
	}
	stopped = true;
}

void MinigolfDrawer::SetTrack(Track& track) {
	this->track = &track;
	
	ballx = 550;
	bally = 160;
	ballr = 5;
	holex = 100;
	holey = 160;
	holer = 30;
	racket_r = 20;
	racket_l = 15;
	racket_rphi = 0;
	
	simulator.Clear();
	
	simulator.BuildMomentumPropagator();
	
	// for each new track, the position propagator must be rebuilt
	simulator.BuildPositionPropagator(track.base);
	
}

void MinigolfDrawer::StopMoving() {
	lock.Enter();
	
	// Collapse position
	simulator.PositionMeasurement(&ballx, &bally);
	if ((ballx - holex)*(ballx - holex) + (bally - holey)*(bally - holey) < holer*holer)
		res = QMG_WIN;
	else
		res = QMG_LOSE;
	
	lock.Leave();
}

void MinigolfDrawer::MouseMove(Point p, dword keyflags) {
	
	if (state == STATE_AIMING) {
		double dx = p.x - ballx;
		double dy = p.y - bally;
		
		racket_rphi = atan(dy / dx);
		if (dx > 0)
			racket_rphi += M_PI;
	}
}

void MinigolfDrawer::RightDown(Point p, dword keyflags) {
	hack_state++;
	if (hack_state >= HACKSTATE_COUNT)
		hack_state = 0;
}

void MinigolfDrawer::LeftDown(Point p, dword keyflags) {
	if (state == STATE_AIMING) {
		state = STATE_SETVELOCITY;
	}
	else if (state == STATE_MOVING) {
		state = STATE_FINISHED;
		StopMoving();
	}
	else if (state == STATE_FINISHED) {
		state = STATE_AIMING;
		SetTrack(*track);
	}
}

void MinigolfDrawer::LeftUp(Point p, dword keyflags) {
	if (state == STATE_SETVELOCITY) {
		state = STATE_HITTING;
	}
}

void MinigolfDrawer::Paint(Draw& w) {
	Size sz = GetSize();
	
	if (!track) {w.DrawRect(sz, Black()); return;}
	
	int width	= track->base.GetWidth();
	int height	= track->base.GetHeight();
	
	//ImageDraw id(sz);
	ImageDraw id(width, height);
	id.DrawRect(sz, White());
	
	
	
	// Render Track
	id.DrawImage(0, 0, track->base);
	
	
	// Render Ball
	if (state != STATE_MOVING) {
		// Render hole
		id.DrawEllipse(holex - holer, holey - holer, holer*2, holer*2, Black(), 2, Color(0, 0, 255));
		
		// Render ball
		id.DrawEllipse(ballx - ballr, bally - ballr, ballr*2, ballr*2, Color(255, 255, 0));
	}
	
	// Render Racket
	if (state < STATE_MOVING) {
		double xl, xo, yl, yo; // upper and lower coordinate bounds of the racket
		
		xo = ballx + racket_r * cos(racket_rphi) - .5 * racket_l * sin(racket_rphi);
		xl = ballx + racket_r * cos(racket_rphi) + .5 * racket_l * sin(racket_rphi);
		yo = bally + racket_r * sin(racket_rphi) + .5 * racket_l * cos(racket_rphi);
		yl = bally + racket_r * sin(racket_rphi) - .5 * racket_l * cos(racket_rphi);
		
		id.DrawLine(xo, yo, xl, yl, 1, White());
	}
	
	
	// Render wave
	if (state == STATE_MOVING) {
		
		// Render hole
		id.DrawEllipse(holex - holer, holey - holer, holer*2, holer*2, Black(), 2, Color(0, 0, 255));
		
		Image& cmap = hack_state != HACKSTATE_NULL? this->cmap : this->cmap_mono;
		int cw = cmap.GetWidth();
		int ch = cmap.GetHeight();
		uint8 red, dummy;
		
		//ImageBuffer wave(width, height);
		ImageBuffer wave(id);
		
		const RGBA* cmap_dat = cmap.Begin();
		RGBA* wave_dat = wave.Begin();
		
		lock.Enter();
		
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
			
				int cx = (int)(simulator.psi[x*height + y][1]) + 128;
				int cy = (int)(simulator.psi[x*height + y][0]) + 128;
				
				if (cx > 240)
					cx = 240;
					
				if (cx < 0)
					cx = 0;
					
				if (cy > 240)
					cy = 240;
					
				if (cy < 0)
					cy = 0;
				
				const RGBA* src = (cmap_dat + cy*cw + cx);
				
				if (hack_state == HACKSTATE_SATURATED_PARTIAL) {
					wave_dat->r = min(255, (int)wave_dat->r + (int)src->r * 10000);
					wave_dat->g = min(255, (int)wave_dat->g + (int)src->g * 10000);
					wave_dat->b = min(255, (int)wave_dat->b + (int)src->b * 10000);
				}
				else if (hack_state == HACKSTATE_SATURATED_FULL) {
					wave_dat->r = src->r != 0 ? 0 : 255;
					wave_dat->g = src->g != 0 ? 0 : 255;
					wave_dat->b = src->b != 0 ? 0 : 255;
				} else {
					wave_dat->r = min(255, (int)wave_dat->r + (int)src->r);
					wave_dat->g = min(255, (int)wave_dat->g + (int)src->g);
					wave_dat->b = min(255, (int)wave_dat->b + (int)src->b);
				}
				
				wave_dat++;
			}
		}
		lock.Leave();
		
		
		w.DrawImage(0, 0, wave);
		return;
	}
	
	
	// Render text
	if (state == STATE_FINISHED) {
		String txt;
		Color clr;
		if (res == QMG_WIN) {
			txt = "You won!";
			clr = Color(0, 255, 0);
		} else {
			txt = "You lost";
			clr = Color(255, 0, 0);
		}
		
		Font fnt = SansSerif(45);
		Size txt_sz = GetTextSize(txt, fnt);
		
		id.DrawText((sz.cx - txt_sz.cx) / 2, (sz.cy - txt_sz.cy) / 2, txt, fnt, clr);
	}
	
	w.DrawImage(0,0,id);
}
