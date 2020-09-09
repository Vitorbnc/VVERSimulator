#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#
# You will need GLFW (http://www.glfw.org):
# Linux:
#   apt-get install libglfw-dev
# Mac OS X:
#   brew install glfw
# MSYS2:
#   pacman -S --noconfirm --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-glfw
#

#CXX = g++
#CC = g++
#CXX = clang++

EXE = bin/main
SOURCES = main.cpp
SOURCES +=reactor.cpp
SOURCES += C:/libs/imgui/examples/imgui_impl_glfw.cpp  C:/libs/imgui/examples/imgui_impl_opengl3.cpp
SOURCES += C:/libs/imgui/imgui.cpp C:/libs/imgui/imgui_draw.cpp C:/libs/imgui/imgui_widgets.cpp
#SOURCES += C:/libs/imgui/imgui.cpp C:/libs/imgui/imgui_draw.cpp C:/libs/imgui/imgui_widgets.cpp C:/libs/imgui/imgui_demo.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)

CXXFLAGS = -IC:/libs/imgui/examples -IC:/libs/imgui
CXXFLAGS += -g -Wall -Wformat
LIBS =

## Other Libs
CXXFLAGS+= -IC:/libs/Eigen -IC:/libs/stb
CXXFLAGS+= -IC:/libs/cpp-httplib

##-------

## GLFW OpenGL Library

CXXFLAGS+=-IC:/libs/glfw/include  
LIBS+=-LC:/libs/glfw/lib  

##

##---------------------------------------------------------------------
## OPENGL LOADER
##---------------------------------------------------------------------

## Using OpenGL loader: gl3w [default]
SOURCES += C:/libs/gl3w/src/gl3w.c 
CXXFLAGS += -IC:/libs/gl3w/include -DIMGUI_IMPL_OPENGL_LOADER_GL3W

## Using OpenGL loader: glew
## (This assumes a system-wide installation)
# CXXFLAGS += -DIMGUI_IMPL_OPENGL_LOADER_GLEW
# LIBS += -lGLEW

## Using OpenGL loader: glad
# SOURCES += ../libs/glad/src/glad.c
# CXXFLAGS += -I../libs/glad/include -DIMGUI_IMPL_OPENGL_LOADER_GLAD

## Using OpenGL loader: glad2
# SOURCES += ../libs/glad/src/gl.c
# CXXFLAGS += -I../libs/glad/include -DIMGUI_IMPL_OPENGL_LOADER_GLAD2

## Using OpenGL loader: glbinding
## This assumes a system-wide installation
## of either version 3.0.0 (or newer)
# CXXFLAGS += -DIMGUI_IMPL_OPENGL_LOADER_GLBINDING3
# LIBS += -lglbinding
## or the older version 2.x
# CXXFLAGS += -DIMGUI_IMPL_OPENGL_LOADER_GLBINDING2
# LIBS += -lglbinding

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += -lGL `pkg-config --static --libs glfw3`

	CXXFLAGS += `pkg-config --cflags glfw3`
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	LIBS += -L/usr/local/lib -L/opt/local/lib
	#LIBS += -lglfw3
	LIBS += -lglfw

	CXXFLAGS += -I/usr/local/include -I/opt/local/include
	CFLAGS = $(CXXFLAGS)
endif


ifeq ($(OS),Windows_NT)
	ECHO_MESSAGE = "MinGW"
	LIBS += -lglfw3dll -lws2_32
	#Habilite para não mostrar o console
	CXXFLAGS+=-mwindows
	#CXXFLAGS+=-m32
	#-lglfw3 -lgdi32 -lopengl32 -limm32

	#CXXFLAGS += `pkg-config --cflags glfw3`
	CXX = g++
	CC = gcc
	CFLAGS = $(CXXFLAGS)
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------
#LIBS += -lglfw3dll -lglfw3 -lgdi32 -lopengl32 -limm32



CFLAGS = $(CXXFLAGS)

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:C:/libs/imgui/examples/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:C:/libs/imgui/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:C:/libs/gl3w/src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o:../libs/glad/src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
