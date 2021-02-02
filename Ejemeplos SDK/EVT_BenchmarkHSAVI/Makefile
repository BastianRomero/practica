CFLAG_ARCH = -m64

CXX=g++
CFLAGS = -Wall $(CFLAG_ARCH)

DIR_OUT=./out
CXXEXE = $(DIR_OUT)/EVT_BenchmarkHSAVI

DIR_EMERGENT_INC = -I$(EMERGENT_DIR)/eSDK/include/
LDLIBS_FLAGS = -lpthread -L$(EMERGENT_DIR)/eSDK/lib/  -lEmergentCamera  -lEmergentGenICam  -lEmergentGigEVision

SOURCE_CXX = $(wildcard *.cpp) 
SOURCE_CXX := $(filter-out stdafx.cpp, $(SOURCE_CXX))

OBJS_CXX = $(patsubst %.cpp,$(DIR_OUT)/%.o,$(SOURCE_CXX)) 

$(DIR_OUT)/%.o: %.cpp
	mkdir -p $(DIR_OUT)
	$(CXX) -c $(CFLAGS) $(DIR_EMERGENT_INC) $< -o $@

$(CXXEXE) : $(OBJS_CXX)
	$(CXX) -o $(CXXEXE) $(OBJS_CXX) $(LDLIBS_FLAGS)

.PHONY:all
all:  $(CXXEXE)

.PHONY:clean
clean:
	rm -fr $(CXXEXE) $(DIR_OUT)


