# This Makefile finds all *.cpp source files in the current directory and
# compiles them into corresponding *.o object files. Then all these object files
# are linked to a single executable called main.
#
# The following targets are defined:
#
# - all        Builds the application. Remember to run make depend first.
#
# - depend     Scans through the source files to find the dependencies between
#              the source files.
#
# - clean      Cleans the directory, removes object files and the executable.
#
# - distclean  Cleans the directory extensively. Use this before submitting a
#              compulsory exercise.
#
#
# Some useful CXXFLAGS:
#
# -I<dir>                       Also look in <dir> for header files
# -g2                           produce debugging information
# -Wall                         turn on important warnings
# -Werror                       treat warnings as errors
# -O3                           optimize for speed
# -ffast-math                   avoids some checks in math-routines
# -fsingle-precision-constant   use float constants (instead of double)
# -pedantic                     make gcc picky
# -fprofile-arcs                Does profiling in order to optimize branching
# -fbranch-probabilities        Uses the result of profile-arcs to do the actual
#                               branch prediction
#
# Some useful LDFLAGS
#
# -l<lib>                       Link with library
# -L<dir>                       Also look in <dir> for libraries

APP      := main
CXX      := g++
LD       := g++
SOURCES  := $(wildcard src/*.cpp)
OBJECTS  := $(patsubst %.cpp, %.o, $(SOURCES))

CXXFLAGS := $(CXXFLAGS) -Wall -g2 -DDEBUG -std=c++11 
LDFLAGS  := $(LDFLAGS) -lm -lGLEW -lGLFW -lIL -lILU 
# OS X
ifeq "$(shell uname)" "Darwin"
    LDFLAGS := $(LDFLAGS) -framework OpenGL
else
    LDFLAGS := $(LDFLAGS) -lGL -lGLU
endif

.PHONY: all depend clean

all: depend $(APP)

$(APP): $(OBJECTS)
	$(LD) -o $(APP) $(LDFLAGS) $(OBJECTS)

depend: make.dep

make.dep:
#	touch $@
#	makedepend -f$@ -- $(CXXFLAGS) -- $(SOURCES)
	for file in $(SOURCES); do $(CXX) $(CXXFLAGS) -M $$file >> $@; done

include make.dep

clean:
	rm -f src/*.o src/*.a src/*~ core $(APP)

distclean: clean
	rm -f make.dep src/*.bak
