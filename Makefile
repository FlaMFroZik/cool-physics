# cool-physics — 2D physics sandbox (long double, GLFW + OpenGL)
#
# The simulation flavour is chosen at compile time:
#
#   make               build BOTH binaries (constant + Newtonian gravity)
#   make const         constant gravity:  bin/cool-physics-const
#   make newton        Newtonian N-body:  bin/cool-physics-newton
#   make run           build & run  (SIM=const by default)
#   make run SIM=newton
#   make clean
#
# Requirements: a C11 compiler, GLFW 3 dev package and OpenGL headers, e.g.
#   Debian/Ubuntu: sudo apt install build-essential libglfw3-dev libgl1-mesa-dev
#   Fedora:        sudo dnf install gcc make glfw-devel mesa-libGL-devel
#   macOS:         brew install glfw
#   MSYS2:         pacman -S mingw-w64-ucrt-x86_64-glfw

CC      ?= cc
CFLAGS  ?= -O2
CFLAGS  += -std=c11 -Wall -Wextra -Wpedantic -I. -MMD -MP

OS := $(shell uname -s 2>/dev/null)
ifeq ($(OS),Darwin)
  LDLIBS = -lglfw -framework OpenGL -lm
else ifneq ($(findstring MINGW,$(OS)),)
  LDLIBS = -lglfw3 -lopengl32 -lgdi32 -lm
else
  LDLIBS = -lglfw -lGL -lm
endif

SIM ?= const

CONST_FLAGS  = -DSIM_CONSTANT=1
NEWTON_FLAGS = -DSIM_NEWTON=1

CONST_EXE  = bin/cool-physics-const
NEWTON_EXE = bin/cool-physics-newton

SOURCES = main.c render/render.c render/gl.c physics/physics.c physics/collisions.c

CONST_OBJ  = $(addprefix build/const/,  $(SOURCES:.c=.o))
NEWTON_OBJ = $(addprefix build/newton/, $(SOURCES:.c=.o))

ifeq ($(SIM),newton)
  RUN_EXE = $(NEWTON_EXE)
else ifeq ($(SIM),const)
  RUN_EXE = $(CONST_EXE)
else
  $(error Unknown SIM="$(SIM)": use 'const' or 'newton')
endif

all: $(CONST_EXE) $(NEWTON_EXE)
const: $(CONST_EXE)
newton: $(NEWTON_EXE)

$(CONST_EXE): $(CONST_OBJ) | bin
	$(CC) $(LDFLAGS) -o $@ $(CONST_OBJ) $(LDLIBS)

$(NEWTON_EXE): $(NEWTON_OBJ) | bin
	$(CC) $(LDFLAGS) -o $@ $(NEWTON_OBJ) $(LDLIBS)

build/const/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CONST_FLAGS) -c $< -o $@

build/newton/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(NEWTON_FLAGS) -c $< -o $@

bin:
	mkdir -p bin

run: $(RUN_EXE)
	./$(RUN_EXE) $(ARGS)

clean:
	rm -rf build bin

-include $(wildcard build/*/*.d)

.PHONY: all const newton run clean
