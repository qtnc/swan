CXX=g++
CXXFLAGS=-std=gnu++17 -Wextra -g
LDFLAGS=-lboost_regex_1_61_0
CXXCFLAGS=-c $(CXXFLAGS)
SRC=$(wildcard qscript/*.cpp) $(wildcard qscript/lib/*.cpp) $(wildcard cpprintf/*.cpp) $(wildcard cli/*.cpp)
OBJDIR=obj/
#OBJ=$(addprefix $(OBJDIR),$(notdir $(SRC:.cpp=.o)))
OBJ=$(addprefix $(OBJDIR),$(SRC:.cpp=.o))
EXEC=a.exe
PERCENT=%

all: $(EXEC)

$(EXEC): $(OBJ)
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXCFLAGS) -o $@ $<
