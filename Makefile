LIBS=x11-xcb

CXXFLAGS=-Wall -g -std=c++17 `pkg-config --cflags $(LIBS)`

CPPFLAGS= -I $(VULKAN_SDK)/include

LDLIBS= -L $(VULKAN_SDK)/lib `pkg-config --static --libs $(LIBS)` -lvulkan

OBJ=viewer.o vulkan.o window.o objloader.o

release: CXXFLAGS+=-O3
release: viewer

debug: CXXFLAGS+=-DLOG_VERBOSE -O0 -Werror
debug: viewer

shaders:
	@glslangValidator -V -o cube_vert.spv cube.vert
	@glslangValidator -V -o cube_frag.spv cube.frag

viewer: $(OBJ)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	$(RM) $(OBJ) viewer *.spv
