#include <omni_sequencer.h>

#include <vector>

#include <properties/MidiNoteAndName.h>
#include <beat_clock.h>
#include <audio_platform.h>
#include <midi_util.h>

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

	// HOWDY TESTING
	/*SeqTrack &track = impl->tracks[0];
	track.steps.resize(16);
	for (int stepIx = 0; stepIx < track.steps.size(); ++stepIx) {
		SeqStep &step = track.steps[stepIx];
		step.numNotes = 0;
		step.noteLength = 0.2;
		if (stepIx % 4 == 0) {
			step.numNotes = 4;
			for (int ii = 0; ii < 4; ++ii) {
				step.notes[ii].v = 1.f;
			}
			step.notes[0].note._note = GetMidiNote("C3");
			step.notes[1].note._note = GetMidiNote("E3-");
			step.notes[2].note._note = GetMidiNote("G3");
			step.notes[3].note._note = GetMidiNote("B3-");
		}
	}*/
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

void OmniSequencer::Gui(GameManager &g) {

}