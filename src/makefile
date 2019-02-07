LIBNAME=swan

ifeq ($(OS),Windows_NT)
EXT_EXE=.exe
EXT_DLL=.dll
else
EXT_EXE=
EXT_DLL=.so
endif

ifeq ($(mode),release)
NAME_SUFFIX=
CXXOPTFLAGS=-s -O3
else
NAME_SUFFIX=d
CXXOPTFLAGS=-g
endif

CLI_EXEC=swan$(NAME_SUFFIX)$(EXT_EXE)
IMP_LIB=lib$(LIBNAME)$(NAME_SUFFIX).a
DLL_LIB=$(LIBNAME)$(NAME_SUFFIX)$(EXT_DLL)
OBJDIR=obj$(NAME_SUFFIX)/

CXX=g++
CXXFLAGS=-std=gnu++17 -Wextra
DLL_LDFLAGS=-shared -Wl,--out-implib,$(IMP_LIB) -lboost_regex_1_61_0
CLI_LDFLAGS=-L. -l$(LIBNAME)$(NAME_SUFFIX)

SRCS=$(wildcard swan/vm/*.cpp) $(wildcard swan/lib/*.cpp) $(wildcard swan/misc/*.cpp) $(wildcard swan/parser/*.cpp)
CPPRINTF_SRCS=$(wildcard cpprintf/*.cpp)
CLI_SRCS=$(wildcard cli/*.cpp)
SWANS=$(wildcard swan/lib/*.swan)
OBJS=$(addprefix $(OBJDIR),$(SRCS:.cpp=.o))
CPPRINTF_OBJS=$(addprefix $(OBJDIR),$(CPPRINTF_SRCS:.cpp=.o))
CLI_OBJS=$(addprefix $(OBJDIR),$(CLI_SRCS:.cpp=.o))
PERCENT=%

all: dll cli

.PHONY: dll cli

clean:
	rm -r $(OBJDIR)

cli: $(CLI_EXEC)

dll: $(DLL_LIB)

$(CLI_EXEC): $(CLI_OBJS) $(CPPRINTF_OBJS)
	@$(CXX) $(CXXFLAGS) $(CXXOPTFLAGS) -o $@ $^ $(CLI_LDFLAGS)

$(DLL_LIB): $(OBJS) $(CPPRINTF_OBJS)
	@$(CXX) $(CXXFLAGS) $(CXXOPTFLAGS) -o $@ $^ $(DLL_LDFLAGS)

swan/lib/builtin-code.h: $(SWANS)
	@luajit fileToCStr.lua BUILTIN_CODE $^ >$@

$(OBJDIR)swan/vm/VM_Init.o: swan/lib/builtin-code.h

$(OBJDIR)%.o: %.cpp $(wildcard %.hpp)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(CXXOPTFLAGS) -c -o $@ $<

