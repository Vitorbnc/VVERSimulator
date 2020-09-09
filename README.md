# VVERSimulator
### A Simulator for the Primary Circuit of the VVER-440/V213 Pressurized Water Reactor
The Simulator is a *C++* implementation of the model described in the article by Cs. Fazekas et al (2006)[ A simple dynamic model of the primary circuit in VVER plants for controller design purposes](https://doi.org/10.1016/j.nucengdes.2006.12.002). The model parameters used are those from the Unit 3 of the Paks Nuclear Power Plant in Hungary.
### Features
1. Equations are computed in *real time* (actually every sampling step) by Euler's method
2. All the model variables and parameters are available, though only some were shown in the UI for simplicity
3. Multi-threading capabilities
4. A PI Controller with possible synchronous sampling
5. A *restful* HTTP server for remote operation
6. A simple Python UI for remote operation
7. A friendly main UI showing the Process Diagram with graph-plotting capabilities

The main UI diagram was adapted from a [World Nuclear Association](https://www.world-nuclear.org/) diagram for a Generic PWR (please note that the original diagram does NOT show a VVER) to portray a VVER and match the description given in the article.

### Libraries and Build
The project uses [cpp-httplib](https://github.com/yhirose/cpp-httplib) for the HTTP Server, [Dear ImGui](https://github.com/ocornut/imgui) for the User Interface, [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) for loading the process diagram and [Eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page) for matrix operations. The development branch of Eigen is required to use the new matrix slicing API.

The project was built with MinGW-w64 and GCC 8.1, and there are build scripts available which use GNU make.
There's a prebuilt binary in the */bin* folder, it was compiled and tested with Windows 10 version 18363. To build again, just run `buiild.bat`.

### Screenshots 
![main_screen](/screenshots/inicial_vver.png "Main Window")

![Charts1](/screenshots/gráfico_ultimas_3000_amostras.png "W_SG Charts")
