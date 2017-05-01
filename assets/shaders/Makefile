CMD=glslangValidator
FLAGS=-V
POST=grep -v -G '\(Warning, version .*$$\)\|\(^$$\)\|\(Linked .* stage:\)'



FRAG_SHADERS=  \
	diffuse.frag \

VERT_SHADERS=  \
	diffuse.vert \


all: ${VERT_SHADERS:.vert=_vert.spv} ${FRAG_SHADERS:.frag=_frag.spv}

%_vert.spv:
	@$(CMD) $(FLAGS) -o $@ ${@:_vert.spv=.vert} | $(POST)

%_frag.spv:
	@$(CMD) $(FLAGS) -o $@ ${@:_frag.spv=.frag} | $(POST)

clean:
	@$(RM) *.spv
#!/bin/sh
