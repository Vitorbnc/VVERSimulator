#ifndef REACTOR_H
#define REACTOR_H

#include "Eigen/Dense"
#include <vector>
#include <string>
#include <thread>
#include <mutex>


using namespace std;

//Dynamic double matrix
using Eigen::MatrixXd;

//Dynamic float matrix
//using Eigen::MatrixXf;


#define DEFAULT_SAMPLING_TIME 0.04//até 0.05 é um bom valor, mais que isso o sistema pode ficar instável //In seconds
#define BUF_TIME 60*60*.5 // 30min por buffer
#define BUF_SIZE  (int)(BUF_TIME/DEFAULT_SAMPLING_TIME) //Tamanho para 2h por cada buffer, há 2 buffers



/*
 * Implementação do Modelo de um Reator a Água Pressurizada (PWR) modelo VVER-440, descrito em:
 * Fazekas,C.; Szederkényi, G.; Hangos, K.M.;2006
 * "A simple dynamic model of the primary circuit in VVERplants for controller design purposes"
 * URL: https://www.sciencedirect.com/science/article/pii/S0029549306006789
 *
 * A saída da planta tem a forma de um trem de impulsos Y(kT) = \sum_{k=0}^{\infty} \delta(t-kT)*Y(kT)
*/


	struct IDX {
		int id, uid;
		string name;
	};


/**
 * @brief Parâmetros do Sistema, devem ser constantes.
 * O construtor padrão carrega os valores obtidos na Unidade 3 do reator real
 */

struct Parameters{
        //Reactor Core
        double Lambda_R, S_R, c_psi_R;
        double p_R[3];
        //Primary Circuit
        double cp_PC, KT_SG, W_loss_PC, V0_PC;
        //Pressurizer
        double cp_PR,W_loss_PR,A_PR,M_PR;
        //Steam Generator
        double cp_L_SG, cp_V_SG, E_evap_SG,W_loss_SG;
        //Physical constants
        double pt[3];
        double cphi[3];

};

// ################################################## Reactor Class ######################

class Reactor
{


public:
    /* ************** IMPORTANTE: NÃO RECOMENDA-SE MUDAR A ORDEM DAS VARIÁVEIS NOS ENUMS
     * Adicione novas variáveis à direita, antes de _COUNT se necessário
     * Se mudar, atualize também contagens e os intervalos em IDX_MAP
    */
	
	//TODO (Talvez): Juntar as 3 informações(índice único, índice da matriz e nome) 
	//num único struct, incrementar o índ. único no construtor
	
	//Índices das colunas das matrizes, ordem alfabética
    enum INDEX{
		
		TIME=-98753, //Para podermos usar tempo como índice. É inválido no acesso às matrizes
		
		 /**
         * @brief Entradas do sistema
         * m_in: Vazão Mássica de Entrada no Circuito Primário [kg/s]
         * W_heat_PR: Potência de Aquecimento do Pressurizador [kW]
         * v_R: Posição das Barras de Controle [cm]
         * ones: Entrada sempre unitária, usada para completar as equações [1]
         */
        //IMPORTANTE: Como usamos um enum só, é preciso reiniciar a enumeração porque ela é sequencial
         m_in=0, v_R, W_heat_PR, //ones,
         IN_COUNT=3,
			
        /**
         * @brief Distúrbios (e outras entradas) do sistema
         * m_out: Vazão Mássica de Saída no Circuito Primário [kg/s]
         * m_SG: Vazão Mássica do Circuito Secundário no Gerador de Vapor[kg/s]
         * M_SG: Massa de água do Circuito Primário no Gerador de Vapor [kg]
         * T_SG_SW: Temperatura da Água na Entrada do Gerador de Vapor [°C]
         * T_PC_I: Temperatura na Entrada do Circuito Primário [°C]
         */
         m_out=0, m_SG, M_SG, T_PC_I, T_SG_SW, 
         DIST_COUNT=5,

        /**
         * @brief Saídas do Sistema
         * W_R: Potência do Reator [J/s]
         * p_SG: Pressão no Circuito Primário do Gerador de Calor [Pa]
         * l_PR: Nível de água no pressurizador [m]
         * p_PR: Pressão no Pressurizador [Pa]
         * W_SG: Potência transferida para cada 1 dos 6 Geradores de Vapor. É o que de fato vai gerar energia [W] (Eq 10)
         * p_SG, p_PR, l_PR requerem cálculos intermediários (Eqs, 14,15)
         */
         l_PR=0, p_PR, p_SG, W_R, W_SG,
         OUT_COUNT=5,
			
		/**
         * @brief Estados do sistema
         * N: Fluxo de Nêutrons no Reator [%]
         * M_PC: Massa no Circuito Primário [kg]
         * T_PC: Temperatura no Circuito Primário [°C]
         * m_PR: Vazão mássica no pressurizador[kg/s]
         * T_PR: Temperatura no Pressurizador [°C]
         * T_SG: Temperatura no Gerador de Vapor [°C]
         * m_PR é variável intermedíaria para o cálculo de T_PR  e depende de T_PC. Eq. 20)
         */

         N=0, m_PR, M_PC, T_PC, T_PR, T_SG,
         STATE_COUNT=6,


        NO_INDEX=-11729 // Usado como valor padrão. É inválido no acesso às matrizes

    };
	
	const static int IDX_COUNT = 24;
	/*Mapeia índice único->índice_da_matriz
	*Input : (1,5)	Dist: (5,11)	Out:(11,17)		Sta:(17,24), Time:[1], Outros: NO_INDEX
	*Dentro desses intervalos, os nomes seguem ordem alfabética
	*/
	const static INDEX IDX_MAP[IDX_COUNT];
	
	/*Mapeia índice único->nome do índice_da_matriz
	* Útil para consultas e para converter texto->índice da matriz usando o mapa acima.
	* Usa-se esses 2 vetores em vez de um std::map porque assim temos 3 informações(índice único, índice da matriz e nome)
	* Um std::map só armazena (chave,valor)
	*/
	const static string IDX_NAME[IDX_COUNT];

    Reactor(double samplingTime=DEFAULT_SAMPLING_TIME);
    ~Reactor();
    //Reactor(Parameters,Inputs,Outputs,Disturbances,States,double samplingTime);

    //Auxiliar da entrada
    //Calcula reatividade do núcleo com base na posição dos rods
    // rho: Reatividade do Reator [1]. É variável intermediária (Eq. 5)
    double rho_of_v(double v);

    //Auxiliares da saída

    //Calcula pressão do PR ou SG com base na temperatura
    double p_of_T(double T);
    //Calcula densidade da água no PC com base na temperatura
    double phi_of_T(double T);


	bool consumeSample(){if(sampleReady) return true||(sampleReady=false); else return false;}
	bool isSampleReady(){return sampleReady;}

    MatrixXd getInputs();
    MatrixXd getOutputs();
    MatrixXd getStates();
    MatrixXd getDisturbances();
	
	const MatrixXd* getInputsMatrix();
	const MatrixXd* getOutputsMatrix();
	const MatrixXd* getStatesMatrix();
	const MatrixXd* getDisturbancesMatrix();
	

    void setInput(const double&,INDEX);
    void setDisturbance(const double&,INDEX);
	//Checa que tipo de valor é pelo índice único
	//Intevalos Input : (1,5)	Dist: (5,11)	Out:(11,17)		Sta:(17,24), Time:[1]
	static inline bool isInput(int i){ return (i>1&&i<5);}
	static inline bool isDisturbance(int i){ return (i>5&&i<11);}
	static inline bool isOutput(int i){ return (i>11&&i<17);}
	static inline bool isState(int i){ return (i>17&&i<IDX_COUNT);}
	

    double getSamplingTime(){return Ts;}
	double getTimeAtBufferStart(){return Ts*BUF_SIZE*nFilledBuffers;};
    int getCurrentBuffer(){return buf;}
	int getK(){return k;}

    void killSamplingThread();
    void startSamplingThread();
    

protected:
	mutex muxInputs, muxDisturbances;
    thread samplingThread;
    bool samplingThreadStarted;
    bool samplingThreadKilled;
    bool samplingThreadShouldDie;
	bool sampleReady;

    int buf; //Indica qual o buffer atual

    const static int BUF_COUNT =2;
    double Ts, T0; // Tempo de amostragem e tempo inicial
    unsigned long k; //Amostra atual
    unsigned long nFilledBuffers;

    //Parametros do reator
    //Alternativa: marcar como const struct Parameters* e criar uma nova variável inicializada com as constantes
    const struct Parameters *par;

    //Usado para AZ-5
    bool removeExternalNeutronSource;

    //About 600K ram each for 2 buffers with 21621 samples (2h, Ts=.33s)
    //Tested by using stdlib double[2][5][21621]
    MatrixXd in[BUF_COUNT], out[BUF_COUNT], dist[BUF_COUNT], sta[BUF_COUNT];

	    //Auxiliar necessária para o primeiro cálculo
    void computeInitial_m_PR();

    //void createStateSpaceMatrix();

    void updateStates();
    void updateOutputs();
    void switchBuffers(int oldBuf, int newBuf);
    void onSample();
};

#endif // REACTOR_H
