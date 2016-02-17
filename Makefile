# -----------------------------
# Make settings
# -----------------------------

MAKEFLAGS=

# -----------------------------
# Compiler settings
# -----------------------------

CC  = clang
CXX = clang++
LD  = clang++
RM  = rm

# -----------------------------
# Project settings
# -----------------------------

PROJNAME         = statorange
PROJNAME_DEBUG   = $(PROJNAME)_debug
PROJNAME_RELEASE = $(PROJNAME)

# -----------------------------
# Compiler flags
# -----------------------------



GCCWARNINGS  = all extra double-promotion format=2 init-self \
               missing-include-dirs switch-default switch-enum \
               unused-local-typedefs unused uninitialized \
               suggest-attribute=pure suggest-attribute=const \
               suggest-attribute=noreturn float-equal undef shadow \
               unsafe-loop-optimizations pointer-arith cast-qual cast-align \
               write-strings conversion logical-op missing-field-initializers \
               missing-format-attribute packed redundant-decls \
               vector-operation-performance disabled-optimization \
               stack-protector
GPPWARNINGS  = c++-compat bad-function-cast strict-prototypes nested-externs \
               unsuffixed-float-constants
GPPUNUSED    = declaration-after-statement missing-prototypes inline \
               zero-as-null-pointer-constant strict-overflow=5 padded

CLANGENABLE  = everything
CLANGDISABLE = padded c++98-compat old-style-cast missing-prototypes \
               reserved-id-macro weak-vtables global-constructors \
               exit-time-destructors disabled-macro-expansion
CLANGWFLAGS  = $(foreach warning,$(CLANGENABLE),-W$(warning)) \
               $(foreach warning,$(CLANGDISABLE),-Wno-$(warning))

CWFLAGS      = $(CLANGWFLAGS)
CXXWFLAGS    = $(CLANGWFLAGS)

DEBUGFLAGS   = -g3 -O0 -pthread
RELEASEFLAGS = -g0 -O2 -pthread
CFLAGS       = -std=c11
CXXFLAGS     = -std=c++14

# ---------
# AI-Stuff
# ---------

#AI_FLAGS = -shared -g3 -O0 -fPIC

# -----------------------------
# Linker flags
# -----------------------------

LIBS = pthread m dl asound ssl crypto

# -----------------------------
# Some automatic stuff
# -----------------------------

LFLAGS = $(foreach lib,$(LIBS),-l$(lib))

CSRCFILES := $(wildcard ./src/*.c) $(wildcard ./src/*/*.c)
COBJFILES := $(patsubst %.c,%.o,$(CSRCFILES))
CDEPFILES := $(patsubst %.c,%.d,$(CSRCFILES))
CXXSRCFILES := $(wildcard ./src/*.cpp) $(wildcard ./src/*/*.cpp)
CXXOBJFILES := $(patsubst %.cpp,%.o,$(CXXSRCFILES))
CXXDEPFILES := $(patsubst %.cpp,%.d,$(CXXSRCFILES))

#AI_SRCFILES := $(wildcard ./$(AI_FOLDER)/*.cpp)
#AI_OBJFILES := $(patsubst %.cpp,%.so,$(AI_SRCFILES))
#AI_DEPFILES := $(patsubst %.cpp,%.d,$(AI_SRCFILES))

SRCFILES = $(CSRCFILES) $(CXXSRCFILES)
OBJFILES = $(COBJFILES) $(CXXOBJFILES)
DEPFILES = $(CDEPFILES) $(CXXDEPFILES)

OBJFILES_DEBUG = $(patsubst %.o,debug/%.o,$(OBJFILES))
OBJFILES_RELEASE = $(patsubst %.o,release/%.o,$(OBJFILES))

DEPFILES_DEBUG = $(patsubst %.d,debug/%.d,$(DEPFILES))
DEPFILES_RELEASE = $(patsubst %.d,release/%.d,$(DEPFILES))

# -----------------------------
# Make targets
# -----------------------------

.PHONY: all debug relase clean

all: $(PROJNAME_DEBUG) $(PROJNAME_RELEASE) Makefile
	@echo "  [ Finished ]"

debug: $(PROJNAME_DEBUG) Makefile
	@echo "  [ Done ]"

release: $(PROJNAME_RELEASE) Makefile
	@echo "  [ Done ]"

rebuild:
	make clean && \
	make all

cleanall: clean

-include $(DEPFILES_DEBUG)
-include $(DEPFILES_RELEASE)

$(PROJNAME_DEBUG): $(OBJFILES_DEBUG) $(SRCFILES) Makefile
	@echo "  [ Linking $@ ]" && \
	$(LD) $(OBJFILES_DEBUG) -o $@ $(LFLAGS)

$(PROJNAME_RELEASE): $(OBJFILES_RELEASE) $(SRCFILES) Makefile
	@echo "  [ Linking $@ ]" && \
	$(LD) $(OBJFILES_RELEASE) -o $@ $(LFLAGS)

debug/%.o: %.c Makefile
	@echo "  [ Compiling $< ]" && \
	mkdir debug/$(dir $<) -p && \
	$(CC) $(CFLAGS) $(WFLAGS) $(CWFLAGS) $(DEBUGFLAGS) -MMD -MP -c $< -o $@

debug/%.o: %.cpp Makefile
	@echo "  [ Compiling $< ]" && \
	mkdir debug/$(dir $<) -p && \
	$(CXX) $(CXXFLAGS) $(CXXWFLAGS) $(DEBUGFLAGS) -MMD -MP -c $< -o $@

release/%.o: %.c Makefile
	@echo "  [ Compiling $< ]" && \
	mkdir release/$(dir $<) -p && \
	$(CC) $(CFLAGS) $(CWFLAGS) $(RELEASEFLAGS) -MMD -MP -c $< -o $@

release/%.o: %.cpp Makefile
	@echo "  [ Compiling $< ]" && \
	mkdir release/$(dir $<) -p && \
	$(CXX) $(CXXFLAGS) $(CXXWFLAGS) $(RELEASEFLAGS) -MMD -MP -c $< -o $@

clean:
	-@$(RM) -f $(wildcard $(OBJFILES_DEBUG) $(OBJFILES_RELEASE) $(DEPFILES_DEBUG) $(DEPFILES_RELEASE) $(PROJNAME_DEBUG) $(PROJNAME_RELEASE)) && \
	$(RM) -rf debug release log && \
	echo "  [ Clean done ]"
