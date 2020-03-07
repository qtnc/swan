LIBNAME=swan
DEFINES=$(options)

ifeq ($(OS),Windows_NT)
EXT_EXE=.exe
EXT_DLL=.dll
else
EXT_EXE=
EXT_DLL=.so
endif

ifeq ($(mode),release)
NAME_SUFFIX=
DEFINES += RELEASE
CXXOPTFLAGS=-s -O3
else
NAME_SUFFIX=d
DEFINES += DEBUG
CXXOPTFLAGS=-g
endif

ifeq ($(regex),boost)
OPT_BOOST_REGEX_LIB=-L. -lboost_regex-mgw81-mt-x32-1_70
DEFINES += USE_BOOST_REGEX
else
OPT_BOOST_REGEX_LIB=
endif

ifeq ($(regex),none)
DEFINES += NO_REGEX
endif

CLI_EXEC=swan$(NAME_SUFFIX)$(EXT_EXE)
EXEBIN_EXEC=swan2exe$(NAME_SUFFIX).bin
IMP_LIB=lib$(LIBNAME)$(NAME_SUFFIX).a
DLL_LIB=$(LIBNAME)$(NAME_SUFFIX)$(EXT_DLL)
OBJDIR=obj$(NAME_SUFFIX)/

CXX=g++
CXXFLAGS=-std=gnu++17 -Wextra $(addprefix -D,$(DEFINES))
DLL_LDFLAGS=-shared -Wl,--out-implib,$(IMP_LIB) $(OPT_BOOST_REGEX_LIB)
CLI_LDFLAGS=-L. -l$(LIBNAME)$(NAME_SUFFIX) -lboost_iostreams-mgw81-mt-x32-1_70 -lboost_filesystem-mgw81-mt-x32-1_70 -lboost_date_time-mgw81-mt-x32-1_70
EXEBIN_LDFLAGS=$(CLI_LDFLAGS)

SRCS=$(wildcard swan/vm/*.cpp) $(wildcard swan/lib/*.cpp) $(wildcard swan/misc/*.cpp) $(wildcard swan/parser/*.cpp)
CPPRINTF_SRCS=$(wildcard cpprintf/*.cpp)
CLI_SRCS=$(wildcard cli/*.cpp)
EXEBIN_SRCS=$(wildcard exebin/*.cpp)
MODULES_SRCS=$(wildcard modules/*.cpp)
SWANS=$(wildcard swan/lib/*.swan)
OBJS=$(addprefix $(OBJDIR),$(SRCS:.cpp=.o))
CPPRINTF_OBJS=$(addprefix $(OBJDIR),$(CPPRINTF_SRCS:.cpp=.o))
CLI_OBJS=$(addprefix $(OBJDIR),$(CLI_SRCS:.cpp=.o))
EXEBIN_OBJS=$(addprefix $(OBJDIR),$(EXEBIN_SRCS:.cpp=.o))
MODULES_OBJS=$(addprefix $(OBJDIR),$(MODULES_SRCS:.cpp=.o))
PERCENT=%

all: dll cli exebin

.PHONY: dll cli exebin

clean:
	rm -r $(OBJDIR)

cli: $(CLI_EXEC)

exebin: $(EXEBIN_EXEC)

dll: $(DLL_LIB)

$(CLI_EXEC): $(MODULES_OBJS) $(CLI_OBJS) $(CPPRINTF_OBJS)
	@$(CXX) $(CXXFLAGS) $(CXXOPTFLAGS) -o $@ $^ $(CLI_LDFLAGS)

$(EXEBIN_EXEC): $(MODULES_OBJS) $(EXEBIN_OBJS) $(CPPRINTF_OBJS)
	@$(CXX) $(CXXFLAGS) $(CXXOPTFLAGS) -o $@ $^ $(EXEBIN_LDFLAGS)

$(DLL_LIB): $(OBJS) $(CPPRINTF_OBJS)
	@$(CXX) $(CXXFLAGS) $(CXXOPTFLAGS) -o $@ $^ $(DLL_LDFLAGS)

swan/lib/builtin-code.h: $(SWANS)
	@python fileToCStr.py BUILTIN_CODE $^ >$@

$(OBJDIR)swan/vm/VM_Init.o: swan/lib/builtin-code.h

$(OBJDIR)%.o: %.cpp $(wildcard %.hpp)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(CXXOPTFLAGS) -c -o $@ $<

