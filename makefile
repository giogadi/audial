# A LOT OF THIS came from https://www.gnu.org/software/make/manual/html_node/Prerequisite-Types.html

OBJDIR = intermediates
CXXFLAGS = -std=c++17 -I src/glfw/include -I src/glad/include -I src/ -I src/imgui -I .
GAMENAME = game

OBJS = $(addprefix $(OBJDIR)/,stb_image.o imgui.o imgui_demo.o imgui_draw.o imgui_tables.o imgui_widgets.o imgui_impl_glfw.o imgui_impl_opengl3.o entity_loader.o audio_loader.o game.o audio_util.o audio.o shader.o synth.o glad.o matrix.o component.o renderer.o collisions.o rigid_body.o beep_on_hit.o player_controller.o audio_EventType.o audio_SynthParamType.o)

$(GAMENAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -L src/glfw/lib-x86_64 -l portaudio -lglfw -framework Cocoa -framework OpenGL -framework IOKit -o $(GAMENAME) $(OBJS)

debug: CXXFLAGS += -O0 -g
debug: GAMENAME += -dbg
debug: $(GAMENAME)

bin/enum_codegen: src/enum_codegen.cpp
	$(CXX) -std=c++17 src/enum_codegen.cpp -o bin/enum_codegen

# Singling this out because it's in a different location than the rest of the source files.
$(OBJDIR)/glad.o : src/glad/src/glad.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o : src/imgui/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o : src/imgui/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/stb_image.o : src/stb_image.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/entity_loader.o : src/entity_loader.cpp src/entity_loader.h src/serialize.h src/matrix.h src/component.h src/components/rigid_body.h src/components/beep_on_hit.h src/components/player_controller.h src/components/sequencer.h src/renderer.h src/audio_util.h src/audio_serialize.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/audio_loader.o : src/audio_loader.cpp src/audio_serialize.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o : src/enums/%.cpp
	c++ $(CXXFLAGS) -c $< -o $@

# I want the rest of the cpp files to force rebuild so I don't have to worry about headers and shit
$(OBJDIR)/%.o : src/%.cpp FORCE
	c++ $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o : src/components/%.cpp FORCE
	c++ $(CXXFLAGS) -c $< -o $@

$(OBJS): | $(OBJDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

FORCE:

.PHONY : clean
clean:
	rm -f $(GAMENAME) $(OBJS)