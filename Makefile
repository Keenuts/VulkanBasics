LIBS=x11-xcb

CXXFLAGS=-Wall -Og -g -std=c++14 `pkg-config --cflags $(LIBS)`

CPPFLAGS= -I $(VULKAN_SDK)/include

LDLIBS= -L $(VULKAN_SDK)/lib `pkg-config --static --libs $(LIBS)` -lvulkan

OBJ=viewer.o engine.o window.o



viewer: $(OBJ)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	$(RM) $(OBJ) viewer
