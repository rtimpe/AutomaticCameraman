program_NAME := tracker
program_C_SRCS := $(wildcard *.c)
program_CXX_SRCS := $(wildcard *.cpp)
program_C_OBJS := ${program_C_SRCS:.c=.o}
program_CXX_OBJS := ${program_CXX_SRCS:.cpp=.o}
program_OBJS := $(program_C_OBJS) $(program_CXX_OBJS)
program_INCLUDE_DIRS := /opt/intel/ipp/include
program_LIBRARY_DIRS :=
program_LIBRARIES := opencv_core opencv_highgui opencv_imgproc opencv_objdetect ippi ippcv ippcc gd png z jpeg freetype m pthread riffa glut GL GLU GLEW
#CPPFLAGS += -DDTIME -g -ffast-math -mfpmath=387 -march=core2
CPPFLAGS += -g -fopenmp -ffast-math -mfpmath=387 -march=core2 -rdynamic

CPPFLAGS += $(foreach includedir,$(program_INCLUDE_DIRS),-I$(includedir))
LDFLAGS += $(foreach librarydir,$(program_LIBRARY_DIRS),-L$(librarydir))
LDFLAGS += $(foreach library,$(program_LIBRARIES),-l$(library))

.PHONY: all clean distclean

all: $(program_NAME)

$(program_NAME): $(program_OBJS)
	$(LINK.cc) $(program_OBJS) -o $(program_NAME)

clean:
	@- $(RM) $(program_NAME)
	@- $(RM) $(program_OBJS)

distclean: clean
