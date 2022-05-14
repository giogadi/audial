# A LOT OF THIS came from https://www.gnu.org/software/make/manual/html_node/Prerequisite-Types.html

OBJDIR = intermediates
CXXFLAGS = -std=c++17 -I src/glfw/include -I src/glad/include -I src/ -I src/imgui
GAMENAME = game

OBJS = $(addprefix $(OBJDIR)/,game.o audio.o shader.o stb_image.o synth.o glad.o matrix.o component.o renderer.o collisions.o rigid_body.o beep_on_hit.o player_controller.o imgui.o imgui_demo.o imgui_draw.o imgui_tables.o imgui_widgets.o imgui_impl_glfw.o imgui_impl_opengl3.o)

$(GAMENAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -L src/glfw/lib-x86_64 -l portaudio -lglfw -framework Cocoa -framework OpenGL -framework IOKit -o $(GAMENAME) $(OBJS)

debug: CXXFLAGS += -O0 -g
debug: GAMENAME += -dbg
debug: $(GAMENAME)

# Singling this out because it's in a different location than the rest of the source files.
$(OBJDIR)/glad.o : src/glad/src/glad.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o : src/imgui/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o : src/imgui/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/stb_image.o : src/stb_image.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

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