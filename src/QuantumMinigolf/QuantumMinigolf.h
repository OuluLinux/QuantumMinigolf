#ifndef _QuantumMinigolf_QuantumMinigolf_h
#define _QuantumMinigolf_QuantumMinigolf_h

#include "QuantumSimulator.h"

#define IMAGECLASS Imgs
#define IMAGEFILE <QuantumMinigolf/QuantumMinigolf.iml>
#include <Draw/iml_header.h>

#define QMG_WIN 0
#define QMG_LOSE 1

// dimensions of the playing field
#define WIDTH 640
#define HEIGHT 320

struct Track : Moveable<Track> {
	Image base, soft, hard;
	String title;
};

class MinigolfDrawer : public Ctrl {
	
	enum {STATE_AIMING, STATE_SETVELOCITY, STATE_HITTING, STATE_MOVING, STATE_FINISHED};
	enum {HACKSTATE_NULL, HACKSTATE_COLOR, HACKSTATE_SATURATED_PARTIAL, HACKSTATE_SATURATED_FULL, HACKSTATE_COUNT, HACKSTATE_MOVIE};
	
	QuantumSimulator simulator;
	SpinLock lock;
	Track* track;
	Image cmap, cmap_mono;
	double racket_rphi;
	int holex, holey, holer;
	int ballx, bally, ballr;
	int racket_r, racket_l;
	int state;
	int hack_state;
	int res;
	bool running, stopped;
	
public:
	typedef MinigolfDrawer CLASSNAME;
	MinigolfDrawer();
	~MinigolfDrawer();
	
	void Run();
	void Refresher();
	void StopMoving();
	void SetTrack(Track& track);
	
	virtual void Paint(Draw& w);
	virtual void MouseMove(Point p, dword keyflags);
	virtual void LeftDown(Point p, dword keyflags);
	virtual void RightDown(Point p, dword keyflags);
	virtual void LeftUp(Point p, dword keyflags);

};

struct TrackImage : public Display {
	virtual void Paint(Draw& w, const Rect& r, const Value& q, Color ink, Color paper, dword style) const;
};

#define LAYOUTFILE <QuantumMinigolf/QuantumMinigolf.lay>
#include <CtrlCore/lay.h>

class QuantumMinigolf : public WithQuantumMinigolfLayout<TopWindow> {
	VectorMap<String, Track> tracks;
	
	void LoadTracks();
public:
	typedef QuantumMinigolf CLASSNAME;
	QuantumMinigolf();
	
	void RefreshTracks();
	void SetTrack();
	
	const Image& GetTrack(int i) const {return tracks[i].base;}
	
};

inline QuantumMinigolf& GetQuantumMinigolf() {return Single<QuantumMinigolf>();}



#endif
