#include <omni_sequencer.h>

#include <vector>

#include <properties/MidiNoteAndName.h>
#include <beat_clock.h>
#include <audio_platform.h>
#include <midi_util.h>
#include <renderer.h>
#include <geometry.h>
#include <math_util.h>
#include <editor.h>

struct NoteAndVel {
	MidiNoteAndName note;
	float v;
};

#define MAX_NUM_NOTES 4
struct SeqStep {
	NoteAndVel notes[MAX_NUM_NOTES];
	int numNotes;
	double noteLength;
};

struct SeqTrack {
	std::vector<SeqStep> steps;
	double stepLength; // beat time
	int nextStepIx;
	double nextStepTime; // beat time
	int synthIx;
	bool mute;
};

struct OmniSequencerImpl {
	std::vector<SeqTrack> tracks;
};

void OmniSequencer::Init(GameManager &g) {
	impl = new OmniSequencerImpl();

	BeatClock &beatClock = *g._beatClock;
	double beatTime = beatClock.GetBeatTimeFromEpoch();

	int numSynths = g._audioContext->_state.synths.size();
	impl->tracks.resize(numSynths);
	for (int trackIx = 0; trackIx < impl->tracks.size(); ++trackIx) {
		SeqTrack &track = impl->tracks[trackIx];
		track.synthIx = trackIx;
		track.stepLength = 0.25;
		track.nextStepIx = 0;
		track.nextStepTime = beatTime;
	}

	SeqTrack &track = impl->tracks[0];
	track.steps.resize(16);
	for (int stepIx = 0; stepIx < track.steps.size(); ++stepIx) {
		SeqStep &step = track.steps[stepIx];
		step.numNotes = 0;
		step.noteLength = 0.2;
		for (int ii = 0; ii < MAX_NUM_NOTES; ++ii) {
			step.notes[ii].v = 1.f;
		}

		// HOWDY TESTING
		/*if (stepIx % 4 == 0) {
			step.numNotes = 4;			
			step.notes[0].note._note = GetMidiNote("C3");
			step.notes[1].note._note = GetMidiNote("E3-");
			step.notes[2].note._note = GetMidiNote("G3");
			step.notes[3].note._note = GetMidiNote("B3-");
		}*/
	}
}

void OmniSequencer::Destroy() {
	delete impl;
}

void OmniSequencer::Update(GameManager &g) {
	BeatClock &beatClock = *g._beatClock;
	double beatTime = beatClock.GetBeatTimeFromEpoch();
	for (int trackIx = 0; trackIx < impl->tracks.size(); ++trackIx) {
		SeqTrack &track = impl->tracks[trackIx];
		if (track.nextStepIx < 0 || track.nextStepIx >= track.steps.size() || beatTime < track.nextStepTime) {
			continue;
		}
		SeqStep &step = track.steps[track.nextStepIx];		
		for (int noteIx = 0; noteIx < step.numNotes; ++noteIx) {
			audio::Event e;
			e.type = audio::EventType::NoteOn;
			e.channel = track.synthIx;
			e.delaySecs = 0.0;
			NoteAndVel &noteAndVel = step.notes[noteIx];
			e.midiNote = noteAndVel.note._note;
			e.velocity = noteAndVel.v;
			static int sNoteOnId = 1;
			e.noteOnId = sNoteOnId++;
			g._audioContext->AddEvent(e);

			e.type = audio::EventType::NoteOff;
			e.delaySecs = beatClock.BeatTimeToSecs(step.noteLength);
			g._audioContext->AddEvent(e);
		}
		track.nextStepTime += track.stepLength;
		++track.nextStepIx;
		if (track.nextStepIx >= track.steps.size()) {
			track.nextStepIx = 0;
		}
	}
}

namespace {
int sTrackIx = 0;
int sSelectedStepIx = 0;
}

void OmniSequencer::Gui(GameManager &g, Editor &editor) {
	if (sTrackIx >= impl->tracks.size()) {
		return;
	}
	SeqTrack &track = impl->tracks[sTrackIx];

	InputManager &input = *g._inputManager;

	if (input.IsKeyPressedThisFrame(InputManager::Key::M)) {
		track.mute = !track.mute;
	}

	if (input.IsKeyPressedThisFrame(InputManager::Key::Right)) {
		++sSelectedStepIx;
	}
	if (input.IsKeyPressedThisFrame(InputManager::Key::Left)) {
		--sSelectedStepIx;
	}
	sSelectedStepIx = math_util::Clamp(sSelectedStepIx, 0, track.steps.size()-1);

	if (input.IsKeyPressedThisFrame(InputManager::Key::Up)) {
		editor.IncreasePianoOctave();
	} else if (input.IsKeyPressedThisFrame(InputManager::Key::Down)) {
		editor.DecreasePianoOctave();
	}
	if (sSelectedStepIx < track.steps.size()) {
		SeqStep &step = track.steps[sSelectedStepIx];
		if (input.IsKeyPressedThisFrame(InputManager::Key::Delete)) {
			step.numNotes = 0;
		}

		int midiNotes[MAX_NUM_NOTES];
		int numNotes = editor.GetPianoNotes(midiNotes, MAX_NUM_NOTES);
		if (input.IsShiftPressed()) {
			for (int ii = 0; ii < numNotes && step.numNotes < MAX_NUM_NOTES; ++ii) {
				step.notes[step.numNotes++].note._note = midiNotes[ii];
			}
		} else if (numNotes > 0) {
			std::string name;
			GetNoteName(midiNotes[0], name);
			printf("got: %s\n", name.c_str());
			step.notes[0].note._note = midiNotes[0];
			step.numNotes = 1;
		}
	}	

	Vec3 centerPos;
	geometry::ProjectScreenPointToXZPlane(g._windowWidth / 2, g._windowHeight / 2, g._windowWidth, g._windowHeight, g._scene->_camera, &centerPos);

	float constexpr kSeqXWidth = 10.f;
	float constexpr kSeqZWidth = 8.f;
	Transform bgTrans;
	bgTrans.SetScale(Vec3(kSeqXWidth, 1.f, kSeqZWidth));
	bgTrans.SetPos(centerPos);
	bgTrans.Translate(Vec3(0.f, -0.5f, 0.f));
	Vec4 bgColor(0.f, 0.f, 0.2f, 1.f);
	g._scene->DrawCube(bgTrans.Mat4Scale(), bgColor);
	
	float trackWidth = 0.9f * kSeqXWidth;
	float trackHeight = 1.f;
	float cellSize = std::min(0.8f * trackHeight, 0.8f * trackWidth / 16.f);
	float trackStartX = centerPos._x - 0.5f * trackWidth;
	float trackEndX = trackStartX + trackWidth;
	float cellStartX = trackStartX + 0.5f * cellSize;
	float cellEndX = trackEndX - 0.5f * cellSize;
	float cellStepSize = (cellEndX - cellStartX) / 16;
	Transform cellTrans;
	cellTrans.SetScale(Vec3(cellSize, 1.f, cellSize));
	float cellZ = centerPos._z;
	Vec4 cellColor(1.f, 1.f, 1.f, 1.f);
	Vec4 selectedCellColor(1.f, 0.3f, 0.3f, 1.f);
	for (int stepIx = 0; stepIx < track.steps.size(); ++stepIx) {
		float cellX = cellStartX + stepIx * cellStepSize;
		cellTrans.SetPos(Vec3(cellX, 0.f, cellZ));
		g._scene->DrawCube(cellTrans.Mat4Scale(), stepIx == sSelectedStepIx ? selectedCellColor : cellColor);
	}
}