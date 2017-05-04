#include "QuantumMinigolf.h"

#include <plugin/bz2/bz2.h>
#include <plugin/bmp/bmp.h>
#include "imgs/imgs.brc"

inline int Area(const Size& sz) {return sz.cx * sz.cy;}

void TrackImage::Paint(Draw& w, const Rect& r, const Value& q, Color ink, Color paper, dword style) const {
	w.DrawRect(r, paper);
	int i = q;
	QuantumMinigolf& qm = GetQuantumMinigolf();
	const Image& img = qm.GetTrack(i);
	Size sz = img.GetSize();
	if (sz.cx <= 0 || sz.cy <= 0) return;
	sz /=  (double)sz.cy / r.GetHeight();
	Image img_scaled = Rescale(img, sz);
	w.DrawImage(r.left + r.Width() / 2 - sz.cx / 2, r.top, img_scaled);
}



QuantumMinigolf::QuantumMinigolf() {
	CtrlLayout(*this, "Quantum Minigolf");
	Icon(Imgs::icon());
	Sizeable();
	
	tracksctrl.AddColumn("Track");
	tracksctrl.SetLineCy(100);
	
	LoadTracks();
	RefreshTracks();
	
	tracksctrl <<= THISBACK(SetTrack);
	tracksctrl.SetCursor(0);
	
}

void QuantumMinigolf::LoadTracks() {
	for(int i = 0; i < tracks_all_count; i++) {
		int size = tracks_all_length[i];
		
		String name = tracks_all_files[i];
		
		int a = name.Find(".");
		int b = name.Find("_soft");
		int c = name.Find("_hard");
		bool soft = b != -1;
		bool hard = c != -1;
		String title = soft ? name.Left(b) : hard ? name.Left(c) : name.Left(a);
		LOG(title << ": " << name);
		
		MemReadStream bmpbz2_stream(tracks_all[i], tracks_all_length[i]);
		
		StringStream bmp_stream;
		
		BZ2Decompress(bmp_stream, bmpbz2_stream);
		
		BMPRaster bmp;
		
		bmp_stream.Seek(0);
		bmp_stream.SetLoading();
		
		Image img = bmp.Load(bmp_stream);
		
		Track& t = tracks.GetAdd(title);
		t.title = title;
		if (soft)
			t.soft = img;
		else if (hard)
			t.hard = img;
		else
			t.base = img;
	}
	
	for(int i = 0; i < tracks.GetCount(); i++) {
		Track& t = tracks[i];
		LOG(i << ": " << t.title);
		ASSERT(Area(t.base.GetSize()) > 0);
	}
}

void QuantumMinigolf::RefreshTracks() {
	for(int i = 0; i < tracks.GetCount(); i++) {
		tracksctrl.Set(i, 0, i);
		tracksctrl.SetDisplay(i, 0, Single<TrackImage>());
	}
}

void QuantumMinigolf::SetTrack() {
	int track_id = tracksctrl.GetCursor();
	Track& t = tracks[track_id];
	game.SetTrack(t);
}
