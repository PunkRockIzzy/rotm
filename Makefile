PROJECT := rotm

SRCDIR := src
INCLUDEDIR := include

CXX := g++
CXXFLAGS := -Wall -Wextra -Wpedantic -std=gnu++26 -MMD \
-I$(INCLUDEDIR) \
-Iexternal/raylib/include \
-Iexternal/rlImGui/include \
-Iexternal/mINI/include

MODE ?= debug

OBJDIR := obj/$(MODE)
BINDIR := bin/$(MODE)

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

EXE := .exe

TARGET := $(BINDIR)/$(PROJECT)$(EXE)

CXXFLAGS_DEBUG := $(CXXFLAGS) -O0 -g
CXXFLAGS_PROFILE := $(CXXFLAGS) -O2 -g
CXXFLAGS_RELEASE := $(CXXFLAGS) -O2 -DNDEBUG

ifeq ($(MODE),debug)
    BUILD_CXXFLAGS := $(CXXFLAGS_DEBUG)
else ifeq ($(MODE),profile)
    BUILD_CXXFLAGS := $(CXXFLAGS_PROFILE)
else ifeq ($(MODE),release)
    BUILD_CXXFLAGS := $(CXXFLAGS_RELEASE)
else
    $(error Invalid MODE '$(MODE)'. Use debug, profile, or release)
endif

LDFLAGS := -Lexternal/raylib/lib -Lexternal/rlImGui/lib
LDLIBS := -lstdc++exp -lraylib -lrlImGui -lgdi32 -lwinmm

all: $(TARGET)

run: $(TARGET)
	@$(TARGET) -dev

$(TARGET): $(OBJS) $(OBJDIR)/imgui_stdlib.o | $(BINDIR)
	g++ $(OBJS) $(OBJDIR)/imgui_stdlib.o -o $@ $(LDFLAGS) $(LDLIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(BUILD_CXXFLAGS) -c $< -o $@

$(OBJDIR)/imgui_stdlib.o: external/rlImGui/src/imgui_stdlib.cpp
	$(CXX) $(BUILD_CXXFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir$(EXE) -p $(OBJDIR)

$(BINDIR):
	mkdir$(EXE) -p $(BINDIR)

clean:
	rm$(EXE) -rf obj bin

.PHONY: all clean

-include $(OBJS:.o=.d)
