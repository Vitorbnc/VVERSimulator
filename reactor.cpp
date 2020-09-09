#include "reactor.h"
#include "iostream"
#include <chrono>
#include <cmath>


using namespace  std;


//Imprimir ou saídas de depuração
#define REACTOR_DEBUG false

//Reactor::Reactor(pareters p,Inputs i ,Outputs o,Disturbances d,States s, double samplingTime)
//    :par(p),in(i),out(o),dist(d),vars(s),Ts(samplingTime){}



const Reactor::INDEX Reactor::IDX_MAP[IDX_COUNT]={
		Reactor::TIME,
		Reactor::NO_INDEX, // Entradas,1
		Reactor::m_in,		Reactor::v_R,		Reactor::W_heat_PR,
		Reactor::NO_INDEX, // Dist,5
		Reactor::m_out,	Reactor::m_SG,	Reactor::M_SG,	Reactor::T_PC_I,	 Reactor::T_SG_SW,   
		Reactor::NO_INDEX, //Saídas, 11
		Reactor::l_PR,		Reactor::p_PR,		Reactor::p_SG,		Reactor::W_R,		Reactor::W_SG,   
		Reactor::NO_INDEX, //Estados, 17
		Reactor::N,	Reactor::m_PR,		Reactor::M_PC,		Reactor::T_PC,		Reactor::T_PR, 	Reactor::T_SG
	};

const string Reactor::IDX_NAME[IDX_COUNT]={
		"TIME",
		"NO_INDEX",
		"m_in",	"v_R",	"W_heat_PR",
		"NO_INDEX",
		"m_out", "m_SG", "M_SG", "T_PC_I", "T_SG_W",
		"NO_INDEX",
		"l_PR", "p_PR", "p_SG", "W_R", "W_SG",
		"NO_INDEX",
		"N", "m_PR" "M_PC", "T_PC", "T_PR", "T_SG" 
};


//Cria uma unidade do reator com parâmetros medidos na Unidade 3 da Planta Paks
Reactor::Reactor(double samplingTime):

            buf(0), //Inicializa com buffer 0 do ring buf
            Ts(samplingTime), // Tempo de amostragem em segundos
            T0(60*60*2.5), //2.5h, valor especificado no artigo para a unidade 3
            k(0), //Índice atual
            nFilledBuffers(0), //Buffers preenchidos
            removeExternalNeutronSource(false),
			sampleReady(false)
{

    samplingThreadKilled = samplingThreadShouldDie = samplingThreadStarted = false;

    #if REACTOR_DEBUG
     cout<<"Criando Reactor"<<endl;
    #endif

    for(auto& x: in)  x = MatrixXd::Zero(IN_COUNT,BUF_SIZE);
    for(auto& x: out) x = MatrixXd::Zero(OUT_COUNT,BUF_SIZE);
    for(auto& x: dist) x = MatrixXd::Zero(DIST_COUNT,BUF_SIZE);
    for(auto& x: sta) x = MatrixXd::Zero(STATE_COUNT,BUF_SIZE);



    par= new Parameters{
    .Lambda_R=1e-5, //Tempo de geração [s]
    .S_R=1938.9, //Fluxo de Nêutrons da Fonte Externa [%/s]
    .c_psi_R=13.75e6, //Razão entre Potência e Fluxo de Nêutrons [W/%]
    .p_R={-1.223e-4,-5.502e-5,-1.953e-4}, // Coef de reatividade, unidades [1/m^2],[1/m], [1]

    .cp_PC = 5355, // Calor específico do PC (a 282°C) [J/(kg*K)]
    .KT_SG = 9.5296e6, //Coef. de Transferência de Calor do PC [J/(K*s)]
    .W_loss_PC = 2.996e7, //Perda de calor do PC [J/s]
    .V0_PC = 242, //Volume nominal de água no PC [m^3]

    .cp_PR = 6873.1,//Calor específico do PR a 325°C [J/(kg*K)]
    .W_loss_PR = 1.6823e5, //Perda de calor do PR [J/s]
    .A_PR=4.52, //Seção transversal do PR [m^2]
    .M_PR=19400, //Massa de água nominal no PR [kg]

   // const double cp_L_SG, cp_V_SG,W_loss_SG;
    .cp_L_SG = 3809.9, // Calor específico da água a 260°C [J/(kg*K)]
    .cp_V_SG = 3635.6, // Calor específico do vapor a 260°C [J/(kg*K)]
    .E_evap_SG = 1.658e6, //Energia de evaporação no SG a 260°C[J/kg]
    .W_loss_SG = 1.8932e7, //Perda de calor no SG [J/s]

    .pt = {28884.78,-258.01,0.63}, //Coeficientes de pressão de saturação de vapor
    .cphi = {581.2,2.98,-0.00848} //Coeficientes que relacionam densidade da água à temperatura. [kg/m^3],[kg/(m^3K)],[kg/(m^3/K^2)]
   };


      //Inicializa todos os valores como zero
      //memset(in,0,sizeof(in));

    //Entradas e distúrbios devem permanecer constantes até que sofram alterações
    for(int i=0;i<BUF_COUNT;i++){
    //Valores iniciais extraídos da unidade 3
     in[i].row(v_R) = MatrixXd::Zero(1,BUF_SIZE); //Rod pos [cm]
     in[i].row(m_in)= MatrixXd::Constant(1,BUF_SIZE, 1.4222); //PC mass inflow [kg/s]
     in[i].row(W_heat_PR) = MatrixXd::Constant(1,BUF_SIZE,168); //kW
     //in[buf].row(ones) = MatrixXd::Ones();
     dist[i].row(m_out) = MatrixXd::Constant(1,BUF_SIZE,2.11); //kg/s
     dist[i].row(m_SG) = MatrixXd::Constant(1,BUF_SIZE,119.31); //kg/s
     dist[i].row(M_SG) = MatrixXd::Constant(1,BUF_SIZE,34920); //Water and Steam mass Second. Circuit(SC) of SG, kg
     dist[i].row(T_PC_I) = MatrixXd::Constant(1,BUF_SIZE,258.85); //PC inlet temp, °C
     dist[i].row(T_SG_SW) = MatrixXd::Constant(1,BUF_SIZE,220.85); //SC inlet water temp, °C
    }

    sta[buf](N,0) = 99.3; //neutron flux %
    sta[buf](M_PC,0) = 200000; //kg
    //sta[buf](M_PC_inv,0) = 1/20000;
    computeInitial_m_PR();
    sta[buf](T_PC,0) = 281.13;// °C
    sta[buf](T_PR,0) = 326.57;
    sta[buf](T_SG,0) = 257.78;
    //sta[buf](rho,0) = rho_of_v(in[buf](v,0));

    out[buf](W_R,0) = 13.75e8; //Potência do reator [W]
    out[buf](W_SG,0) = 222516666.666; //Potência de saída em cada um dos geradores de vapor

    //cout<<"Dist m_SG:"<<dist[0](m_SG,0)<<endl;
    //cout<<"Input one:"<<in[1](ones,100)<<endl;
	
	cout<<"par"<<Ts*(1.0/(6873.1*19400))<<endl;

    startSamplingThread();

    //cout<<"After timer"<<endl;
	
	

}
//######################################################################################### Fim do Construtor

//Funções que auxiliam no cálculo dos estados

//Reatividade do núcleo em função da posição das barras
double Reactor::rho_of_v(double v){
    //Potencias de 10 convertem de cm para metro
    return par->p_R[0]*1e-4*v*v +par->p_R[1]*1e-2*v +par->p_R[2];
}

//Densidade em função da temperatura
double Reactor::phi_of_T(double T){
    return par->cphi[0]+par->cphi[1]*T + par->cphi[2]*T*T;
}

//Pressão em função de temperatura
double Reactor::p_of_T(double T){
    return (par->pt[0] + par->pt[1]*T + par->pt[2]*T*T)*1e3;
}
//Precisamos de pelo menos o m_PR inicial para calcular os estados
void Reactor::computeInitial_m_PR(){
    double dTPCdt =  (1/(par->cp_PC*sta[buf](M_PC,0))) \
    *(par->cp_PC*in[buf](m_in,0)*(dist[buf](T_PC_I,0)-sta[buf](T_PC)) \
      +par->cp_PC*dist[buf](m_out,0)*15 + out[buf](W_R,0) -6*out[buf](W_SG,0) -par->W_loss_SG);

    sta[buf](m_PR,0)= in[buf](m_in,0) -dist[buf](m_out,0) \
    -par->V0_PC*(par->cphi[0]+2*par->cphi[1]*sta[buf](T_PC,0))*dTPCdt;
}

void Reactor::updateOutputs(){

    out[buf](W_R,k) = par->c_psi_R*sta[buf](N,k);
    out[buf](p_SG,k) = p_of_T(sta[buf](T_SG,k));
    out[buf](l_PR,k) = 1/par->A_PR*(sta[buf](M_PC,k)/phi_of_T(sta[buf](T_PC,k))-par->V0_PC);
    out[buf](p_PR,k) = p_of_T(sta[buf](T_PR,k));//sta[buf](T_PR,k);//
    out[buf](W_SG,k) = par->KT_SG*(sta[buf](T_PC,k)-sta[buf](T_SG,k));
	
	//cout<<"p_PR:"<<sta[buf](T_SG,k)<<endl;

}

void Reactor::updateStates(){

    // Computa os estados com base no método de Euler

    sta[buf](N,k+1) = sta[buf](N,k)+ Ts*(1.0/par->Lambda_R*(rho_of_v(in[buf](v_R,k)))*sta[buf](N,k)+(removeExternalNeutronSource?0.0:par->S_R));

    sta[buf](M_PC,k+1) = sta[buf](M_PC,k) + Ts*(in[buf](m_in,k) - dist[buf](m_out,k));

    sta[buf](T_PC,k+1) = sta[buf](T_PC,k) + Ts*((1.0/(par->cp_PC*sta[buf](M_PC,k))) \
		   *(  par->cp_PC*in[buf](m_in,k)*(dist[buf](T_PC_I,k)-sta[buf](T_PC,k)) \
               +par->cp_PC*dist[buf](m_out,k)*15.0 + out[buf](W_R,k) -6.0*out[buf](W_SG,k) -par->W_loss_SG)
			);

    sta[buf](m_PR,k+1) = sta[buf](m_PR,k) + Ts*(sta[buf](M_PC,k)-par->V0_PC\
            *(par->cphi[0]+2.0*par->cphi[1]*sta[buf](T_PC,k))*sta[buf](T_PC,k+1));

    sta[buf](T_PR,k+1) = sta[buf](T_PR,k) + Ts*(1.0/(par->cp_PR*par->M_PR))\
			*(1e3*in[buf](W_heat_PR,k) 		-par->W_loss_PR		-par->cp_PR*sta[buf](m_PR,k+1)*sta[buf](T_PR,k)+\
					((sta[buf](m_PR,k+1)>0.0)?(par->cp_PC*sta[buf](m_PR,k+1)*(15.0+sta[buf](T_PC,k))):(par->cp_PR*sta[buf](m_PR,k+1)*sta[buf](T_PR,k)))
			  );


    sta[buf](T_SG,k+1) = sta[buf](T_SG,k) + Ts*(1/(par->cp_L_SG*dist[buf](M_SG,k)))*(par->cp_L_SG*dist[buf](m_SG,k)*dist[buf](T_SG_SW,k) \
            -par->cp_V_SG*dist[buf](m_SG,k)*sta[buf](T_SG,k) -dist[buf](m_SG,k)*par->E_evap_SG \
            -par->W_loss_SG +out[buf](W_SG,k));
}


//Mudar de buffer quando um encher
void Reactor::switchBuffers(int oldBuf, int newBuf){
    #if REACTOR_DEBUG 
        cout<<"Filled a buffer"<<endl;
    #endif
	
	in[newBuf] = in[oldBuf].col(k).replicate(1,BUF_SIZE);
	dist[newBuf] = dist[oldBuf].col(k).replicate(1,BUF_SIZE);
	
	out[newBuf].col(0) = out[oldBuf].col(k);
	sta[newBuf].col(0) = sta[oldBuf].col(k);
	
	
}

//Chamado a cada período de amostragem
//Roda na sua própria thread
void Reactor::onSample(){
    while(!samplingThreadShouldDie){

	muxInputs.lock();
	muxDisturbances.lock();
	//updateStates atualiza os próximos estados, e depende de algumas saídas atuais
    updateOutputs();
	updateStates();
	muxInputs.unlock();
	muxDisturbances.unlock();
	

    //cout<<"k: "<<k<<"\t"<<"Var: "<<out[buf](W_SG,k)<<endl;//sta[buf](N,k)<<endl;
    #if REACTOR_DEBUG
        cout<<"k: "<<k<<"\t"<<"N: "<<sta[buf](N,k)<<"\t"<<"W_R:" <<out[buf](W_R,k)<<"\t"<<"T_PC: "<<sta[buf](T_PC,k)<<"\t"<<"T_SG: "<<sta[buf](T_SG,k)<<"\t"<<"W_SG: "<<out[buf](W_SG,k)<<endl;
    #endif
	
	bool reset_k=false;
	
	if(k>=BUF_SIZE-2){
	int oldBuf = buf;
	switchBuffers(oldBuf, buf = (buf+1)%BUF_COUNT);
    nFilledBuffers++;
    reset_k=true;
	}

	sampleReady=true;
	
    //testes
	//if(k==150) in[buf]((int)W_heat_PR,Eigen::seq(k,Eigen::last)) = MatrixXd::Constant(1,in[buf].cols()-k,200);

    /*
    if(k==50) in[buf]((int)v_R,Eigen::seq(k,Eigen::last)) = MatrixXd::Constant(1,in[buf].cols()-k,0.05);
    if(k==100) in[buf]((int)v_R,Eigen::seq(k,Eigen::last)) = MatrixXd::Constant(1,in[buf].cols()-k,0.08);
    if(k==150) in[buf]((int)v_R,Eigen::seq(k,Eigen::last)) = MatrixXd::Constant(1,in[buf].cols()-k,0.5);
    if(k==200) in[buf]((int)v_R,Eigen::seq(k,Eigen::last)) = MatrixXd::Constant(1,in[buf].cols()-k,0.7);
    if(k==250) in[buf]((int)v_R,Eigen::seq(k,Eigen::last)) = MatrixXd::Constant(1,in[buf].cols()-k,0.8);
    if(k==300) in[buf]((int)v_R,Eigen::seq(k,Eigen::last)) = MatrixXd::Constant(1,in[buf].cols()-k,0.9);
    if(k==350) in[buf]((int)v_R,Eigen::seq(k,Eigen::last)) = MatrixXd::Constant(1,in[buf].cols()-k,1.1);
    if(k==420) in[buf]((int)v_R,Eigen::seq(k,Eigen::last)) = MatrixXd::Constant(1,in[buf].cols()-k,1.3);
    */
    //if(k==550) removeExternalNeutronSource = true;

    //if(k==800) removeExternalNeutronSource = false;

	this_thread::sleep_for(chrono::milliseconds(int(1000*Ts)));
    //Incrementar só no final, para que de tempo de pegar o  valor
    if(reset_k) k=0; else k++;
    }
}
//################################### Funções para interface com a UI em Qml
/*
vector<double> Reactor::getInputs(){
    //Usando const & para não criar cópias extras
    const auto& vec = in[buf].col(k);
    //QVector<type> v( type *primeiro, type *ultimo);
    //matrix.data() retorna um ponteiro para a localidade de memória do primeiro elemento da matriz
    return vector<double>(vec.data(),vec.data()+vec.size());
}
vector<double> Reactor::getOutputs(){
    const auto& v = out[buf].col(k);
    return vector<double>(v.data(),v.data()+v.size());
    //return vector<double>();
}
vector<double> Reactor::getStates(){
    const auto& v = sta[buf].col(k);
    return vector<double>(v.data(),v.data()+v.size());
    //return vector<double>();
}
vector<double> Reactor::getDisturbances(){
    const auto& v = dist[buf].col(k);
    return vector<double>(v.data(),v.data()+v.size());
    //return vector<double>();
}
*/
MatrixXd Reactor::getInputs(){
 return in[buf].col(k);
}
const MatrixXd* Reactor::getInputsMatrix(){
	return &in[buf];
}

const MatrixXd* Reactor::getOutputsMatrix(){
	return &out[buf];
}

const MatrixXd* Reactor::getStatesMatrix(){
	return &sta[buf];
}

const MatrixXd* Reactor::getDisturbancesMatrix(){
	return &dist[buf];
}


MatrixXd Reactor::getOutputs(){
 return out[buf].col(k);

}
MatrixXd Reactor::getStates(){
 return sta[buf].col(k);

}
MatrixXd Reactor::getDisturbances(){
 return dist[buf].col(k);

}
//Carrega uma única entrada
void Reactor::setInput(const double &val, INDEX idx){
    if(in[buf](idx,k)==val) return; //Se a entrada for igual, não mude
	muxInputs.lock();
    in[buf](int(idx),Eigen::seq(k,Eigen::last)) = MatrixXd::Constant(1,in[buf].cols()-(k),val);
	muxInputs.unlock();
}
void Reactor::setDisturbance(const double &val, INDEX idx){
    if(dist[buf](idx,k)==val) return; //Se a entrada for igual, não mude
	muxDisturbances.lock();
    dist[buf](int(idx),Eigen::seq(k,Eigen::last)) = MatrixXd::Constant(1,dist[buf].cols()-(k),val);
	muxDisturbances.unlock();
}

void Reactor::startSamplingThread(){
        if(!samplingThreadStarted){ //Evitar a recriação de threads
        #if(REACTOR_DEBUG)
            cout<<"Starting Thread"<<endl;
        #endif
		samplingThread=thread([this]{onSample();});
		samplingThreadShouldDie = false; //limpar as flags
		samplingThreadStarted = true;
	}
}

void Reactor::killSamplingThread(){
    // t1.detach libera a thread principal de esperar a execução da thread t1
	// Quando a execuçao termina, os recursos são limpos da memória
	if(samplingThreadStarted){
		samplingThread.detach();
		samplingThreadShouldDie = true;
		samplingThreadStarted = false;
	}
}

Reactor::~Reactor(){
    killSamplingThread();
    if(par!=nullptr) delete par;
}