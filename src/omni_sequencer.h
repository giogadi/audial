#pragma once

#include "game_manager.h"

struct OmniSequencerImpl;
struct OmniSequencer {
	OmniSequencerImpl *impl;

	void Init(GameManager &g);
	void Destroy();
	void Update(GameManager &g);
	
	void Gui(GameManager &g);
};