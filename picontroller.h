#ifndef PICONTROLLER_H
#define PICONTROLLER_H
/* Controlador PI com sincronização externa por semáforo binário (opcinal)
* Recebe os parâmetros tradicionais (Kp, Ti, input, setpoint)
* Recebe uma função que retorna bool para sincronizar, você pode passar esse parâmtro com uma lambda: []()->bool{return xxx}
* TODO: Alterar a sincronização para não bloquear dentro da thread, mas para substituir totalmente a amostragem
* Isso deve permitir simular a amostragem síncrona (como se a planta já fosse amostrada e discretizada pela mesma CPU onde roda o PI)
* TODO: Adicionar saturação e anti-windup do termo integral
*/

class PIController{
	protected:
	double errSum, out;
	bool (*syncSem) ();
	thread thr;
	bool shouldDie;
	bool sampleReady;
	const double *setpoint;
	int lastSetpoint;
		
	void onSample(){
		while(!shouldDie){
			if(!pause){
			//Espera a amostra do sincronizador externo ficar pronta
			if(syncSem!=nullptr) 
				while(!syncSem()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
			if(lastSetpoint!=*setpoint){
				lastSetpoint = *setpoint;
				errSum = 0;
				cout<<"set change"<<endl;
			}
			
			double err = (*setpoint)-(*input);
			//TODO: ajustar ganhos para deixar tempo de amostragem sempre 1seg
			//Atualmente ignorando o ajuste
			errSum+= 1.0/Ti*Ts*err;//err; // *Ts/1000.0 *err;
			out = Kp*err +errSum;
			sampleReady = true;
			}
		std::this_thread::sleep_for(std::chrono::milliseconds(Ts));
		//std::cout<<"PI Running"<<endl;
		}
	}
	
	public:
	int Ts;
	double Ti, Kp;
	const double *input;
	bool pause;

	double output(){return out;}
	bool consumeSample(){if(sampleReady)return true | (sampleReady=false); else return false;}
	bool isSampleReady(){return sampleReady;}
		
	PIController(int samplingTimeMs, double Kp,double Ti, const double *input, const double *setpoint, bool (*externalSampleReady)(void) = nullptr):
		Ts(samplingTimeMs),  input(input), Kp(Kp), Ti(Ti), setpoint(setpoint),
		syncSem(externalSampleReady),
		shouldDie(false), pause(true),
		out(0), lastSetpoint(-1.7523e-5)
	{
		thr = std::thread([this]{onSample();});
	}
	
	
	~PIController(){
		thr.detach();
		shouldDie = true;
	}
	
};

#endif
