# A LOT OF THIS came from https://www.gnu.org/software/make/manual/html_node/Prerequisite-Types.html

OBJDIR = intermediates
CXXFLAGS = -std=c++17 -I src/glfw/include -I src/glad/include
GAMENAME = game

OBJS = $(addprefix $(OBJDIR)/,game.o audio.o shader.o stb_image.o synth.o glad.o)

$(GAMENAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -L src/glfw/lib-x86_64 -l portaudio -lglfw -framework Cocoa -framework OpenGL -framework IOKit -o $(GAMENAME) $(OBJS)

debug: CXXFLAGS += -O0 -g
debug: GAMENAME += -dbg
debug: $(GAMENAME)

# Singling this out because it's in a different location than the rest of the source files.
$(OBJDIR)/glad.o : src/glad/src/glad.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# I want the rest of the cpp files to force rebuild so I don't have to worry about headers and shit
$(OBJDIR)/%.o : src/%.cpp FORCE
	c++ $(CXXFLAGS) -c $< -o $@

$(OBJS): | $(OBJDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

FORCE:

.PHONY : clean
clean:
	rm -f $(GAMENAME) $(OBJS)