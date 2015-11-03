MAKEFLAGS=

CXX = g++
RM  = rm

# FLAGS

WFLAGS       = -Wall -Wextra -pedantic -Wdouble-promotion -Wformat=2 -Winit-self \
               -Wmissing-include-dirs -Wswitch-default -Wswitch-enum -Wunused-local-typedefs \
               -Wunused -Wuninitialized -Wstrict-overflow=5 -Wsuggest-attribute=pure \
               -Wsuggest-attribute=const -Wsuggest-attribute=noreturn -Wfloat-equal \
               -Wundef -Wshadow -Wunsafe-loop-optimizations \
               -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings \
               -Wconversion -Wlogical-op \
               -Wmissing-field-initializers \
               -Wmissing-format-attribute -Wpacked -Winline -Wredundant-decls \
               -Wvector-operation-performance -Wdisabled-optimization \
               -Wstack-protector

#CWFLAGS      = -Wdeclaration-after-statement -Wc++-compat -Wbad-function-cast \
#               -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs -Wunsuffixed-float-constants

#FLAGS        = -shared -g3 -O0 -fPIC
FLAGS         = -std=c++11 -g3 -O0

# SOURCEFILES

SRCFILES := $(wildcard ./*.cpp)
OBJFILES := $(patsubst %.cpp,%.o,$(SRCFILES))
SOFILES := $(patsubst %.cpp,%.so,$(SRCFILES))
DEPFILES := $(patsubst %.cpp,%.d,$(SRCFILES))

# TARGETS

.PHONY: all clean

all: #$(OBJFILES) Makefile
	@echo " NOT CONFIGURED YET"
	@echo "  [ Done ]"

-include $(DEPFILES)

%.o: %.cpp Makefile
	@echo "  [ Compiling $< ]" && \
	$(CXX) $(FLAGS) $(WFLAGS) -MMD -MP $< -o $@ -c

clean:
	-@$(RM) -f $(wildcard $(OBJFILES) $(DEPFILES)) && \
	echo "  [ Clean StateItems done ]"






