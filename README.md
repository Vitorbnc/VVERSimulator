# VVERSimulator
### A Simulator for the Primary Circuit of the VVER-440/V213 Pressurized Water Reactor
The Simulator is a *C++* implementation of the model described in the article by Cs. Fazekas et al (2006)[ A simple dynamic model of the primary circuit in VVER plants for controller design purposes](https://doi.org/10.1016/j.nucengdes.2006.12.002). The model parameters used are those from the Unit 3 of the Paks Nuclear Power Plant in Hungary.

UI text is currently in Brazilian Portuguese. Translating should be relatively straightforward. Most code comments and variable names are in English and match the naming adopted in the article.
### Features
1. Equations are computed in *real time* (actually every sampling step) by Euler's method
2. All the model variables and parameters are available, though only some were shown in the UI for simplicity
3. Multi-threading capabilities
4. A PI Controller with possible synchronous sampling
5. A *restful* HTTP server for remote operation
6. A simple Python UI for remote operation
7. A friendly main UI showing the Process Diagram with graph-plotting capabilities

The main UI diagram was adapted from a [World Nuclear Association](https://www.world-nuclear.org/) diagram for a Generic PWR (please note that the original diagram does NOT show a VVER) to portray a VVER and match the description given in the article.

In the ` /img ` folder contains an earlier version of the process diagram, which shows a vertical steam generator. This agrees with most western PWR designs, but to more accurately represent the VVER, it was later replaced for a new diagram with an horizontal steam generator. If you wish to use the vertical diagram, use the file `main_vert.cpp.bkp` instead of `main.cpp`.The code changes required mostly involve repositioning the UI widgets.

### Libraries and Build
The project uses [cpp-httplib](https://github.com/yhirose/cpp-httplib) for the HTTP Server, [Dear ImGui](https://github.com/ocornut/imgui) for the User Interface, [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) for loading the process diagram and [Eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page) for matrix operations. The development branch of Eigen is required to use the new matrix slicing API.

The project was built with MinGW-w64 and GCC 8.1, and there are build scripts available which use GNU make.
There's a prebuilt binary inside ` /bin `, it was compiled and tested with Windows 10 version 18363. To build again, just run `build.bat`.

A compressed folder with all the libraries used to build is available in `libs_backup.7z` for convenience. The makefile assumes they are stored in `C:/libs`.

The python remote control app uses PySide2 for the UI and Requests for HTTP requests.

### Screenshots 

Some of the screenshots show the vertical steam generator version. Here's the new main window:

![main_screen](/screenshots/inicial_vver.png "Main Window")

Plotting real time charts:

![Charts1](/screenshots/gráfico_ultimas_3000_amostras.png "W_SG Charts")

Simple Qt for Python Remote Control:

![Remote1](/VVER_Remote/screenshots/conectado.png "Simple Remote Connected")
