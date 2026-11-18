ifeq ($(shell uname -s),Linux)
	HOST_OS := LINUX
else ifeq ($(OS),Windows_NT)
	HOST_OS := WIN32
endif

PROJECT := rotm

SRCDIR := src
INCLUDEDIR := include

MODE ?= debug

# apt does not provide g++ 16+, install via homebrew
ifeq ($(HOST_OS),WIN32)
	CXX := g++
else ifeq ($(HOST_OS),LINUX)
	CXX := g++-16
endif

OBJDIR := obj/$(MODE)
BINDIR := bin/$(MODE)

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

RAYLIB_SRCDIR := external/raylib/src
RAYLIB_OBJDIR := $(OBJDIR)/raylib
RAYLIB_SRCS := $(wildcard $(RAYLIB_SRCDIR)/*.c)
RAYLIB_OBJS := $(patsubst $(RAYLIB_SRCDIR)/%.c,$(RAYLIB_OBJDIR)/%.o,$(RAYLIB_SRCS))
RAYLIB_CFLAGS := -w \
-DPLATFORM_DESKTOP_GLFW -DGRAPHICS_API_OPENGL_33 \
-fno-strict-aliasing -std=c99 -MMD -O1 -I$(RAYLIB_SRCDIR) -I$(RAYLIB_SRCDIR)/external/glfw/include

ifeq ($(HOST_OS),LINUX)
	RAYLIB_CFLAGS += -D_DEFAULT_SOURCE -D_GLFW_X11
else ifeq ($(HOST_OS),WIN32)
	RAYLIB_CFLAGS += -D_GNU_SOURCE -DUNICODE
endif

IMGUI_SRCDIR := external/imgui
IMGUI_OBJDIR := $(OBJDIR)/imgui
IMGUI_SRCS := $(wildcard $(IMGUI_SRCDIR)/*.cpp)
IMGUI_OBJS := $(patsubst $(IMGUI_SRCDIR)/%.cpp,$(IMGUI_OBJDIR)/%.o,$(IMGUI_SRCS))


RLIMGUI_SRCDIR := external/rlImGui
RLIMGUI_OBJDIR := $(OBJDIR)/rlImGui
RLIMGUI_SRCS := $(wildcard $(RLIMGUI_SRCDIR)/*.cpp)
RLIMGUI_OBJS := $(patsubst $(RLIMGUI_SRCDIR)/%.cpp,$(RLIMGUI_OBJDIR)/%.o,$(RLIMGUI_SRCS))

TRACY_SRCDIR := external/tracy/public
TRACY_OBJDIR := $(OBJDIR)/tracy
TRACY_OBJ := $(TRACY_OBJDIR)/TracyClient.o

CXXFLAGS := -std=gnu++23 -MMD \
-I$(INCLUDEDIR) \
-I$(RAYLIB_SRCDIR) \
-I$(IMGUI_SRCDIR) \
-I$(RLIMGUI_SRCDIR) \
-I$(TRACY_SRCDIR) \
-Iexternal/imgui/misc/cpp \
-Iexternal/mINI/src

EXTERNAL_CXXFLAGS := -w $(CXXFLAGS)

CXXFLAGS := -std=gnu++23 -MMD \
-I$(INCLUDEDIR) \
-I$(RAYLIB_SRCDIR) \
-I$(IMGUI_SRCDIR) \
-I$(RLIMGUI_SRCDIR) \
-I$(TRACY_SRCDIR) \
-Iexternal/imgui/misc/cpp \
-Iexternal/mINI/src

WARNFLAGS := -Wall -Wextra -Wpedantic

ifeq ($(HOST_OS),LINUX)
	EXE :=
else ifeq ($(HOST_OS),WIN32)
	EXE := .exe
endif

TARGET := $(BINDIR)/$(PROJECT)$(EXE)

CXXFLAGS_DEBUG := $(WARNFLAGS) $(CXXFLAGS) -O0 -g
CXXFLAGS_PROFILE := $(WARNFLAGS) $(CXXFLAGS) -O2 -g -DTRACY_ENABLE
CXXFLAGS_RELEASE := $(WARNFLAGS) $(CXXFLAGS) -O2 -DNDEBUG

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
LDLIBS := -lstdc++exp

ifeq ($(HOST_OS),LINUX)
	LDLIBS += -lGL -lm -lpthread -ldl -lrt -lX11
else ifeq ($(HOST_OS),WIN32)
	LDLIBS += -static -lgdi32 -lwinmm -lws2_32 -ldbghelp -lsecur32
endif

all: $(TARGET)

run: $(TARGET)
	@$(TARGET) -dev

$(TARGET): $(OBJS) $(IMGUI_OBJDIR)/imgui_stdlib.o $(RAYLIB_OBJS) $(IMGUI_OBJS) $(RLIMGUI_OBJS) $(TRACY_OBJ) | $(BINDIR)
	$(CXX) \
	$(OBJS) \
	$(IMGUI_OBJDIR)/imgui_stdlib.o \
	$(RAYLIB_OBJS) \
	$(IMGUI_OBJS) \
	$(RLIMGUI_OBJS) \
	$(TRACY_OBJ) \
	-o $@ \
	$(LDFLAGS) $(LDLIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(BUILD_CXXFLAGS) -c $< -o $@

$(RAYLIB_OBJDIR)/%.o: $(RAYLIB_SRCDIR)/%.c | $(RAYLIB_OBJDIR)
	gcc $(RAYLIB_CFLAGS) -c $< -o $@

$(IMGUI_OBJDIR)/%.o: $(IMGUI_SRCDIR)/%.cpp | $(IMGUI_OBJDIR)
	$(CXX) $(EXTERNAL_CXXFLAGS) -c $< -o $@

$(IMGUI_OBJDIR)/imgui_stdlib.o: $(IMGUI_SRCDIR)/misc/cpp/imgui_stdlib.cpp | $(IMGUI_OBJDIR)
	$(CXX) $(EXTERNAL_CXXFLAGS) -c $< -o $@

$(RLIMGUI_OBJDIR)/%.o: $(RLIMGUI_SRCDIR)/%.cpp | $(RLIMGUI_OBJDIR)
	$(CXX) $(EXTERNAL_CXXFLAGS) -c $< -o $@

$(TRACY_OBJ): $(TRACY_SRCDIR)/TracyClient.cpp | $(TRACY_OBJDIR)
	$(CXX) $(EXTERNAL_CXXFLAGS) -c $< -o $@

$(OBJDIR) $(BINDIR) $(RAYLIB_OBJDIR) $(IMGUI_OBJDIR) $(RLIMGUI_OBJDIR) $(TRACY_OBJDIR):
	mkdir$(EXE) -p $@

clean:
	rm$(EXE) -rf obj bin

.PHONY: all run clean

-include $(OBJS:.o=.d) \
		 $(RAYLIB_OBJS:.o=.d) \
		 $(IMGUI_OBJS:.o=.d) \
		 $(RLIMGUI_OBJS:.o=.d) \
		 $(TRACY_OBJ:.o=.d)
