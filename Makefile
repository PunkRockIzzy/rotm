PROJECT := rotm

SRCDIR := src
INCLUDEDIR := include

CXX := g++
CXXFLAGS := -Wall -Wextra -Wpedantic -std=gnu++26 -MMD \
-I$(INCLUDEDIR) \
-Iexternal/raylib/src \
-Iexternal/rlImGui/include \
-Iexternal/mINI/include

MODE ?= debug

OBJDIR := obj/$(MODE)
BINDIR := bin/$(MODE)

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

RAYLIB_SRCDIR := external/raylib/src
RAYLIB_OBJDIR := $(OBJDIR)/raylib
RAYLIB_SRCS := $(wildcard $(RAYLIB_SRCDIR)/*.c)
RAYLIB_OBJS := $(patsubst $(RAYLIB_SRCDIR)/%.c,$(RAYLIB_OBJDIR)/%.o,$(RAYLIB_SRCS))
RAYLIB_CFLAGS := -Wall -D_GNU_SOURCE -DPLATFORM_DESKTOP_GLFW -DGRAPHICS_API_OPENGL_33 \
-Wno-missing-braces -Werror=pointer-arith -fno-strict-aliasing -std=c99 -DUNICODE -MMD \
-O1 -Werror=implicit-function-declaration -Iexternal/raylib/src \
-Iexternal/raylib/src/external/glfw/include

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
LDLIBS := -lstdc++exp -lrlImGui -lgdi32 -lwinmm

all: $(TARGET)

run: $(TARGET)
	@$(TARGET) -dev

$(TARGET): $(OBJS) $(OBJDIR)/imgui_stdlib.o $(RAYLIB_OBJS) | $(BINDIR)
	g++ $(OBJS) $(OBJDIR)/imgui_stdlib.o $(RAYLIB_OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(BUILD_CXXFLAGS) -c $< -o $@

$(OBJDIR)/imgui_stdlib.o: external/rlImGui/src/imgui_stdlib.cpp
	$(CXX) $(BUILD_CXXFLAGS) -c $< -o $@

$(RAYLIB_OBJDIR)/%.o: $(RAYLIB_SRCDIR)/%.c | $(RAYLIB_OBJDIR)
	gcc $(RAYLIB_CFLAGS) -c $< -o $@

$(OBJDIR) $(RAYLIB_OBJDIR) $(BINDIR):
	mkdir$(EXE) -p $@

clean:
	rm$(EXE) -rf obj bin

.PHONY: all run clean

-include $(OBJS:.o=.d) $(RAYLIB_OBJS:.o=.d)
