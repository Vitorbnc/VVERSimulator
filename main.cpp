#define DEBUG_MAIN false
#define SHOW_MOUSE_POS false

#include <iostream>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <string>
#include "GL/gl3w.h"

#include <GLFW/glfw3.h>

#include <thread>
#include <chrono>
#include <cmath>


#define PI 3.141592654

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "reactor.h"

#include "outils.h"
#include "picontroller.h"
#include "netserver.h"

using namespace std;

const char *WIN_TITLE = "Controle do Reator PWR VVER-440";

const int DEFAULT_DISPLAY_W=1024;
const int DEFAULT_DISPLAY_H=768;
//Imagem em tela inteira
const int IMG_PLANT_LEFT_MARGIN = 30;
const float IMG_PLANT_TOP_MARGIN =7.5;
const int IMG_PLANT_W = DEFAULT_DISPLAY_W;
const int IMG_PLANT_H = 0.89*DEFAULT_DISPLAY_H;
const int IMG_PLANT_ASPECT_RATION = IMG_PLANT_W/IMG_PLANT_H;


const float FONT_DEFAULT_SIZE = 16.5f; //15
const float FONT_LARGE_SIZE = 20.0f;
const char* FONT_DEFAULT_PATH = "../fonts/DroidSans.ttf";
const char* FONT_BOLD_PATH = "../fonts/DroidSans-Bold.ttf";
float fontSize = 0;


int display_w, display_h; //Esses valores serão atualizados a cada renderização

//Image imgPlant{"../img/pwr_pc.png",1024,768};
Image imgPlant{"../img/vver_pc.png",2384,1523};
Image imgRods{"../img/rods.png",217,494};
Image imgBomba{"../img/pas_bomba.png",175,175};

ImFont *fonte;
ImFont *fonteBold;

ImFont *fonteGrande;
ImFont *fonteGrandeBold;

using Eigen::MatrixXd;
using Eigen::MatrixXf;
using Eigen::VectorXf;


MatrixXd inputs,outputs,disturbances,states;
const MatrixXd  *inputsMatrix, *outputsMatrix, *disturbancesMatrix, *statesMatrix;
int currentBuf,currentK;

//Instância do Reator
Reactor reactor;

//Servidor TCP/HTTP na porta 80
NetServer svr(&reactor,80,"0.0.0.0");

PIController *pwrController;
bool pwrControllerActive = false;

static void chartsWindow(bool *show);

//bool firstLoop = true;

const double INITIAL_W_SG_SETPOINT = 1365; //1330
double newSetpoint = 0;
double kp = 15, Ti= 20;//kp = 10, Ti= 35;//kp =0.0001, ki=10000;
//Consideramos o controlador como um sistema externo que amostra 2x mais rápido que a freqûencia da planta
double piSamplingTime = 333*reactor.getSamplingTime();

//--------------------------

int main(int argc, char** args) {

	std::cout<<"Iniciando..."<<endl;
	

	//Lambda para exibir erros do glfw
	glfwSetErrorCallback([](int error, const char* what) { 
		fprintf(stderr, "Glfw Error %d: %s\n", error, what);
	});

	if (!glfwInit()) return 1; // inicializar o glfw

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Usar opengl 3+
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	const char* glsl_version = "#version 130";

	// Cria janela do opengl
	GLFWwindow* window = glfwCreateWindow(DEFAULT_DISPLAY_W, DEFAULT_DISPLAY_H, WIN_TITLE,NULL,NULL);
	if (window == NULL) return 2;

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); //Ativa vsync
	
	if (gl3wInit() != 0) return 3; // Incializa o carregador do opengl

	//Contexto da ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	//(void)io; //Evita avisos do compilador com variável não usada
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

	ImGui::StyleColorsLight();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	//FONT LOAD
	
	fonte = io.Fonts->AddFontFromFileTTF(FONT_DEFAULT_PATH, FONT_DEFAULT_SIZE); // A 1a fonte carregada vira a padrão
	fonteBold = io.Fonts->AddFontFromFileTTF(FONT_BOLD_PATH, FONT_DEFAULT_SIZE);

	fonteGrande = io.Fonts->AddFontFromFileTTF(FONT_DEFAULT_PATH,FONT_LARGE_SIZE);
	fonteGrandeBold = io.Fonts->AddFontFromFileTTF(FONT_BOLD_PATH, FONT_LARGE_SIZE);


	IM_ASSERT(fonte != NULL);//Verifica se a fonte foi carregada
	
   //IMAGE LOAD
    IM_ASSERT(imgPlant.load());
	IM_ASSERT(imgBomba.load());
	IM_ASSERT(imgRods.load());

	const int readInterval = reactor.getSamplingTime()/ImGui::GetIO().DeltaTime;//.250/ImGui::GetIO().DeltaTime;
	const int writeInterval  = readInterval;
	int readCounter = readInterval;
	int writeCounter = writeInterval;

	
	//############## Variáveis permanentes da interface de usuário ########################################################
	#if DEBUG_MAIN
	bool show_demo_window = false;
	#endif
	bool showChartsWindow = false;
	bool showInfo = false;

	
	//Barras de controle
	const int MIN_IMG_RODS_POS = 246;
	const int MAX_IMG_RODS_POS = 340;
	int imgRodsPos = MIN_IMG_RODS_POS;
	const int MIN_RODS_POS = 0;
	const int MAX_RODS_POS=130;
	float rodsWScale = float(IMG_PLANT_W)/float(imgPlant.w);
	
	//Animação de Rotação 
	//omega=2*PI/T, dTheta=omega*dt
	float dT = ImGui::GetIO().DeltaTime;
	float T = 1;
	static float theta = 0;
	
	//#######################

	ImGuiStyle &style = ImGui::GetStyle();
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1,1,1,1);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.9,0.9,0.9,1);
	
	
	//double pwrSetpoint = 2.22516e+08, pwrInput =2.22516e+08;
	double pwrSetpoint = INITIAL_W_SG_SETPOINT, pwrInput =INITIAL_W_SG_SETPOINT;
	//Alternativa 1 atualizar 2x ou mais por ciclo do reator (500*reactor_sampling_time)
	//Também é possível desabilitar o semáforo, se não subirmos demais a freq. de atualização 
	pwrController = new PIController(piSamplingTime,kp,Ti,&pwrInput,&pwrSetpoint, nullptr);
	
	//Alternativa 2 atualizamos 1x por ciclo (1000*reactor_sampling_time)
	//pwrController = new PIController(1000*reactor.getSamplingTime(),kp,Ti,&pwrInput,&pwrSetpoint, []()->bool{return reactor.consumeSample();});
	

	while (!glfwWindowShouldClose(window)) {

		glfwPollEvents();
		
		
		glfwGetFramebufferSize(window, &display_w, &display_h); // Carrega o tamanho atual da janela
		glViewport(0, 0, display_w, display_h);
		//glClearColor(0.1, 0.1, 0.11, 1.0); //define o valor padrão carregado quando os buffers de cor são limpos.É a cor de fundo
		glClearColor(1, 1, 1, 1.0);
		glClear(GL_COLOR_BUFFER_BIT); // reseta o buffer de cor para o valor definido como padrão

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

	if(readCounter>=readInterval){
		readCounter=0;
		inputs = reactor.getInputs();
		outputs = reactor.getOutputs();
		disturbances = reactor.getDisturbances();
		states = reactor.getStates();
		
		if(pwrControllerActive) pwrInput = 6.0e-6*outputs(Reactor::W_SG);
		
		//pwrController->input = &outputs(Reactor::W_SG);
		//cout<<"pi input"<<*(pwrController->input)<<endl;
		//cout<<"Inputs: ";
		//for (auto x:inputs.reshaped()) cout<<to_string(x)<<"\t";
    
	}

		//GUI vai aqui
		{
			//Cria e configura janela
			ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove;
			flags |= ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus;
			flags |= ImGuiWindowFlags_MenuBar;
			//flags |= ImGuiWindowFlags_NoScrollbar |ImGuiWindowFlags_NoScrollWithMouse;
			//Adicionando pequena margem à esquerda
			
			//ImGui::SetNextWindowPos(ImVec2(0.02*display_w, 0));
			ImGui::SetNextWindowPos(ImVec2(0,0));
			
			ImGui::SetNextWindowSize(ImVec2(display_w, display_h));
			ImGui::Begin("Janela Inicial",NULL,flags);
			

			float cursorX = ImGui::GetCursorPosX();
			ImGui::SetCursorPosX(IMG_PLANT_LEFT_MARGIN);
			ImGui::SetCursorPosY(IMG_PLANT_TOP_MARGIN);
			//O cast intermediário para intptr_t é só para evitar advertência do compilador "-Wint-to-pointer-cast"
			ImGui::Image((void*)(intptr_t) imgPlant.id,ImVec2(IMG_PLANT_W,IMG_PLANT_H));
			
			//Imagem das Barras de Controle

			imgRodsPos = mapVal(inputs(Reactor::v_R),MIN_RODS_POS,MAX_RODS_POS,MIN_IMG_RODS_POS,MAX_IMG_RODS_POS);
			addImage((void*)(intptr_t)imgRods.id,ImVec2(221,imgRodsPos),ImVec2(imgRods.w*rodsWScale,imgRods.h*rodsWScale));

			//Imagem animada com rotação
			addImageRotated((void*)(intptr_t)imgBomba.id,ImVec2(500,533),ImVec2(70,70),theta+=2*PI/T*dT);
			
			if(ImGui::BeginMenuBar()){
				
				if(ImGui::BeginMenu("Exibir")){
					
					if(ImGui::MenuItem("Gráficos")){ showChartsWindow = true;}
					//if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
				ImGui::EndMenu();
				}					
				if(ImGui::BeginMenu("Ajuda")){
					if (ImGui::MenuItem("Sobre")) {showInfo = true;}
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}
			
			if(showInfo){
				ImGui::SetNextWindowSize(ImVec2(0,0));
				ImGui::Begin("Informações",&showInfo,0);
				ImGui::Text("Controle do Circuito Primário do Reator VVER-440\n" \
							"O VVER-440 é um Reator Nuclear de Fissão com circuito primário \n"
							"de Água Pressurizada (PWR)\n");
				ImGui::Text("Software criado para o Trabalho Final da Disciplina Automação\n"\
							"Avançada da Universidade Federal de Lavras - UFLA");
				ImGui::Text("Modelo físico real baseado no artigo:\n" \
							"Fazekas,C.; Szederkényi, G.; Hangos, K.M.;2006 \n" \
							"A simple dynamic model of the primary circuit in VVERplants \n" \
							"for controller design purposes");
				ImGui::Text("Diagrama do processo inspirado pelas ilustrações da World Nuclear\n" \
							"Association (www.world-nuclear.org) \n");
				ImGui::End();
			}

			
			
			//#########
			//Demo, para ver os widgets
			//Podemos descobrir a posição dos items pelo mouse com GetMousePos(), só ative DEBUG_MAIN para imprimir
			#if DEBUG_MAIN
			if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);
			ImGui::SetCursorPos(ImVec2(900,20));
			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
			#endif
			
			#if SHOW_MOUSE_POS
			ShowExampleAppSimpleOverlay(NULL);
			#endif
			
		
		//################################### Controles da Planta
			//Janelas(legenda,unidade,valor,id_unico,x,y,w,h);
			//Importante: se dois ids do mesmo tipo de janela forem iguais, uma delas não aparecerá
			//Indice para acessar as variáveis do reator
			enum Reactor::INDEX ind = Reactor::NO_INDEX;

			int x,y,w,h,leftMargin;
			x=60, y=80;
			ind = Reactor::v_R;
			inputDoubleWindow("Barras(v_R)","cm",&inputs(ind),1,x,y);
			reactor.setInput(constrain(inputs(ind),MIN_RODS_POS,MAX_RODS_POS),ind);
			
			x=453, y=123, w=105, h=100;
			leftMargin=5;
			ind=Reactor::W_heat_PR;
			inputDoubleWindow("Pot Ent PR\n(W_heat_PR)","kW",&inputs(ind),2,x,y,w,h,leftMargin);
			reactor.setInput(constrain(inputs(ind),0,400),ind);
			
			x=580, y=575, w=120, h=100;
			ind=Reactor::m_in;
			inputDoubleWindow("Vaz Entr PC\n	(m_in)","kg/s",&inputs(ind),3,x,y,w,h);
			reactor.setInput(constrain(inputs(ind),0,5),ind);
			
			x=760, y=150, w=120, h=120;
			leftMargin = 15;
			inputDoubleWindow("PI Pot W_SG","MW",&pwrSetpoint,6,x,y,w,h,leftMargin,&pwrControllerActive);
			constrainInPlace(pwrSetpoint,0,2000);
			pwrController->pause = !pwrControllerActive;
			
			x=860, y=460, w=120, h=100;
			ind=Reactor::m_SG;
			inputDoubleWindow("Vaz Liq SG (SC)\n	(m_SG)","kg/s",&disturbances(ind),4,x,y,w,h);
				if(pwrControllerActive&&pwrController->consumeSample()) disturbances(ind) = pwrController->output();

			reactor.setDisturbance(constrain(disturbances(ind),0,1e10),ind);
			
			
			x=810, y=575, w=120, h=100;
			ind=Reactor::m_out;
			inputDoubleWindow("Vaz Said PC\n   (m_out)","kg/s",&disturbances(ind),5,x,y,w,h);
			reactor.setDisturbance(constrain(disturbances(ind),0,5),ind);
			
			
			ImVec4 txtColor =  ImVec4(0.6,0.2,0.2,1);
			x=315, y=365, w=125, h=70;
			outputDoubleWindow("Pot Núcleo(W_R)","MW",outputs(Reactor::W_R)/1e6,1,x,y,w,h,&txtColor);
			
			x=453, y=50, w=120, h=70;
			outputDoubleWindow("Press PR(p_PR)","bar",outputs(Reactor::p_PR)/1e5,2,x,y,w,h);
			
			x=5, y=400, w=120, h=70;
			//Exibir temperatura colorida
			double temp = states(Reactor::T_PC);
			double tempColorRed = mapVal(temp,80,300,0,1);
			ImVec4 tempColor = ImVec4(tempColorRed,0.0,1.0-tempColorRed,1.0);
			outputDoubleWindow("Temp PC(T_PC)","°C",states(Reactor::T_PC),3,x,y,w,h,&tempColor);
			
			//Mostar temperatura média do secundário do SG
			ImGui::SetCursorPos(ImVec2(745,280));
			ImGui::BeginGroup();
			ImGui::PushFont(fonteBold);
			ImGui::Text("Temperatura SG (SC):");
			temp = states(Reactor::T_SG);
			tempColorRed = mapVal(temp,80,300,0,1);
			tempColor = ImVec4(tempColorRed,0.0,1.0-tempColorRed,1.0);
			ImGui::TextColored(tempColor,"  %.3f °C",temp);
			ImGui::PopFont();
			ImGui::EndGroup();
			
			
			x=710, y=75, w=120, h=65;//y=102
			//Há 6 Geradores de Vapor, a saída W_SG dá a potência para cada um em Watts
			outputDoubleWindow("Pot Req(W_SG)","MW",6.0*outputs(Reactor::W_SG)/1e6,4,x,y,w,h);
			
			if(showChartsWindow){
				chartsWindow(&showChartsWindow);
			}

			//ImGui::Text("Reactor: %.3d");
			
			//Adicione Outros Widgets Aqui 
			ImGui::SetCursorPosX(cursorX);
			ImGui::SetCursorPosY(10);
			
			
			
			ImGui::End();
		}
		
		readCounter++;
		writeCounter++;

		#if(DEBUG_MAIN)
		ImVec2 screenPos = ImGui::GetMousePos();
		cout<<"Mouse-> X: "<<screenPos.x<<"\t Y: "<<screenPos.y<<endl;
		#endif

		//Renderização
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		//Os gráficos são renderizados para um buffer fora da tela (offscreen), esta função coloca o buffer offscreen na tela
		glfwSwapBuffers(window);
		
		//firstLoop = false;

	}
	
	//Limpeza de threads aqui
	svr.killServer();
	reactor.killSamplingThread();
	//Delete objetos criados dinamicamente
	//O destrutor já finaliza a thread
	delete pwrController;
	
	//Limpeza
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
//######### Janela dos Gráficos #####################################################################################
static void chartsWindow(bool *show){
	
	currentBuf = reactor.getCurrentBuffer();
	currentK = reactor.getK();
	inputsMatrix = reactor.getInputsMatrix();
	outputsMatrix = reactor.getOutputsMatrix();
	disturbancesMatrix = reactor.getDisturbancesMatrix();
	statesMatrix = reactor.getStatesMatrix();
	
	
	
	//IMPORTANTE: CHEQUE SE NÃO FALTA VÍRGULA ENTRE OS ELEMENTOS
	//Se faltar, o programa vai fechar ao tentar selecionar qualquer coisa
	const int itemCount = 24;
	
	const char* items[itemCount] ={
		"Amostra(k)",
		"ENTRADAS 1",
		"m_in: Vazão Mássica de Entrada no Circuito Primário [kg/s]",
		"v_R: Posição das Barras de Controle [cm]",
		"W_heat_PR: Potência de Aquecimento do Pressurizador [W]",
		"ENTRADAS 2/DISTÚRBIOS",
		"m_out: Vazão Mássica de Saída no Circuito Primário [kg/s]",
		"m_SG: Vazão Mássica do Circuito Secundário no Gerador de Vapor[kg/s]",
		"M_SG: Massa de água do Circuito Primário no Gerador de Vapor [kg]",
		"T_SG_SW: Temperatura da Água na Entrada do Gerador de Vapor [°C]",
		"T_PC_I: Temperatura na Entrada do Circuito Primário [°C]",
		"SAÍDAS",
		"l_PR: Nível de água no pressurizador [m]",
		"p_PR: Pressão no Pressurizador [Pa]",
		"p_SG: Pressão no Circuito Primário do Gerador de Calor [Pa]",
		"W_R: Potência do Reator [W]",
		"W_SG: Potência transferida para cada Gerador de Vapor [W]",
		"ESTADOS",
		"N: Fluxo de Nêutrons no Núcleo do Reator [%]",
		"m_PR: Vazão mássica no Pressurizador [kg/s]",	
		"M_PC: Massa no Circuito Primário [kg]",
		"T_PC: Temperatura no Circuito Primário [°C]",
		"T_PR: Temperatura no Pressurizador [°C]",
		"T_SG: Temperatura no Gerador de Vapor [°C]",
	 };
	
	
	const int nCharts = 2;
	
	static int selY[nCharts];
	static int nSamples[nCharts];
	static bool allSamples[nCharts];
	
	static bool firstRun=true;
	if(firstRun){
		for(int i=0;i<nCharts;i++){
			selY[i] = (i==0)?16: 7;
			nSamples[i] = 50;
			allSamples[i] = false;
		}
		firstRun = false;
	}

	MatrixXf mat[nCharts];

	
	ImGui::SetNextWindowPos(ImVec2(DEFAULT_DISPLAY_W/3,DEFAULT_DISPLAY_H/5),ImGuiCond_FirstUseEver,ImVec2(0.5f,0.5f));
				ImGui::SetNextWindowSize(ImVec2(500,635));
				ImGui::Begin("Gráficos",show,0);
				
			for(int i=0;i<nCharts;i++){
				
				ImGui::Combo(to_string(i).insert(0,"Eixo Y##").c_str(),&selY[i],items,itemCount);
				ImGui::InputInt(to_string(i).insert(0,"Número de Amostras##").c_str(), &nSamples[i], 1,10);
				ImGui::Checkbox(to_string(i).insert(0,"Todas as Amostras##").c_str(),&allSamples[i]);
				if(allSamples[i]) nSamples[i] = currentK;
				nSamples[i] = constrain(nSamples[i],0,currentK-1);
				
				// Recebe o valor atual da seleção do ComboBox (selY), que contém um índice único, e passa para os índices das matrizes com IDX_MAP
				//Depois, checamos se é entrada ou saída para copiar a linha da matriz adequada (matriz de saídas, entradas, ...)
				//A quantidade de colunas copiadas corresponde ao número de amostras para plotagem
				
				
				//if(selY[i]>1&&selY[i]<5) //Entrada
				if(Reactor::isInput(selY[i]))
					mat[i] = (*inputsMatrix)(int(Reactor::IDX_MAP[selY[i]]),Eigen::seq(currentK-nSamples[i],currentK)).cast<float>();
				//else if(selY[i]>5&&selY[i]<10)
				else if (Reactor::isDisturbance(selY[i]))
					mat[i] = (*disturbancesMatrix)(int(Reactor::IDX_MAP[selY[i]]),Eigen::seq(currentK-nSamples[i], currentK)).cast<float>();
				//else if(selY[i]>11&&selY[i]<17)
				else if (Reactor::isOutput(selY[i]))
					mat[i] = (*outputsMatrix)(int(Reactor::IDX_MAP[selY[i]]),Eigen::seq(currentK-nSamples[i], currentK)).cast<float>();
				//else if(selY[i]>17&&selY[i]<itemCount)
				else if (Reactor::isState(selY[i]))
					mat[i] = (*statesMatrix)(int(Reactor::IDX_MAP[selY[i]]),Eigen::seq(currentK-nSamples[i], currentK)).cast<float>();
				
				else
					mat[i]= MatrixXf::Zero(1,nSamples[i]);
				
					//mat = mat.cast<float>();
					//xData = vector<float>(mat.data(),mat.data()+mat.size());
				
				ImGui::Text(to_string(i).insert(0,"			Gráfico ").c_str());
				ImGui::PlotLines(to_string(i).insert(0,"Figura ").c_str(), mat[i].data(),mat[i].size(),0,NULL,FLT_MAX,FLT_MAX,ImVec2(ImGui::GetWindowSize().x,200));
				
				}

				//ImGui::PlotLines("Figura 1", const float* values, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0, 0), int stride = sizeof(float));

				
				ImGui::End();
}

// ################################## Charts Window End ##############################################


