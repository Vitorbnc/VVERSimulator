#ifndef NETSERVER_H
#define NETSERVER_H

//Adicione a flag -lws2_32 para compilar com g++ no mingw
#include "httplib.h"
#include <thread>
#include "reactor.h"
#include <iostream>


using namespace std;
using namespace httplib;

class NetServer{
	protected:
	char *url;
	int port;
	thread thr;
	Reactor *reactor;
	Server svr;
	
	//Codifica os dados da saída na forma de matriz: {1,1,5,1\n2,3,4,4},{k,Ts, T0}
	string encodeOutputData(double* data, int cols, int rows=1){
		float k = reactor->getK();
		double Ts = reactor->getSamplingTime();
		double T0 = reactor->getTimeAtBufferStart();
		string out="{";
		for(int i=0;i<rows;i++){
			for(int j=0;j<cols;j++) {
				out+=to_string(*data++);
				if(j<cols-1)out.push_back(',');
			}
		if(i<rows-1)out.push_back('\n'); else out.push_back('}');
		}
		out+=",{" + to_string(k) + ','+ to_string(Ts) + ',' + to_string(T0) + '}';
	return out;
	}
	
	struct ReactorValue{
		Reactor::INDEX idx; // índice da matriz
		double val; //valor
		int uidx; // índice único
	};
	//Recebe as strings idx, val e retorna uma estrutrua ReactorValue pronta para uso
	ReactorValue getReactorValue(string idx, string val){
		cout<<"idx:"<<idx<<"\t"<<"val:"<<val<<endl;
		ReactorValue rv={Reactor::NO_INDEX, 0.0, -1};
		int i=0;
		bool foundIdx = false; 		
		for(i=0; i<Reactor::IDX_COUNT;i++) 
			if(idx.compare(Reactor::IDX_NAME[i])==0){
				foundIdx=true;
				break; //sai do for se achar a string
			}
		if(!foundIdx) return rv;
		rv.idx = Reactor::IDX_MAP[i];
		cout<<"found idx! number:"<<i<<"\t name:"<<Reactor::IDX_NAME[i]<<endl;
				
		rv.val = stod(val); // string to double
		rv.uidx = i;
			
		cout<<"converted value:"<<rv.val<<endl;
		return rv;		
		}
	
	public:
	void killServer(){if(svr.is_running()){ thr.detach(); svr.stop(); }}
	
	NetServer(Reactor *r=nullptr, int svPort=1234, char* svUrl="localhost"):
		url(svUrl), port(svPort), reactor(r)
	{
				
		thr = thread([this]{		
			
			cout<<"Servidor iniciado em:"<<url<<" na porta "<<to_string(port)<<endl;
			string data="";
			
			//Usamos Get para receber um vetor com os valores das variáveis
			svr.Get("/", [&](const Request & /*req*/, Response &res) {
				res.set_content("Ola! Voce esta na interface de rede do Software de Controle do VVER-440.", "text/plain");
			});
			svr.Get("/inputs",[&](const Request & req, Response & res){
				int len = reactor->getInputs().size();
				double *start = reactor->getInputs().data();
				res.set_content(encodeOutputData(start,len), "text/plain");
			
			});
			svr.Get("/outputs",[&](const Request & req, Response & res){
				int len = reactor->getOutputs().size();
				double *start = reactor->getOutputs().data();
				res.set_content(encodeOutputData(start,len), "text/plain");
			});
			svr.Get("/disturbances",[&](const Request & req, Response & res){
				int len = reactor->getDisturbances().size();
				double *start = reactor->getDisturbances().data();
				res.set_content(encodeOutputData(start,len), "text/plain");
			});
			svr.Get("/states",[&](const Request & req, Response & res){
				int len = reactor->getStates().size();
				double *start = reactor->getStates().data();
				res.set_content(encodeOutputData(start,len), "text/plain");
			});
			
			// Usamos POST para setar as variáveis, com parâmetros como:{ "idx" : "v_R", "val":"12.3"}
			svr.Post("/input", [&](const auto& req, auto& res) {
				if(!(req.has_param("idx") && req.has_param("val"))) return;
				string idx = req.get_param_value("idx");
				string val = req.get_param_value("val");
				ReactorValue rv = getReactorValue(idx,val);
				if(Reactor::isInput(rv.uidx))
					reactor->setInput(rv.val,rv.idx);				
				});
				
			svr.Post("/disturbance", [&](const auto& req, auto& res) {
				if(!(req.has_param("idx") && req.has_param("val"))) return;
				string idx = req.get_param_value("idx");
				string val = req.get_param_value("val");
				ReactorValue rv = getReactorValue(idx,val);
				if(Reactor::isDisturbance(rv.uidx))
					reactor->setDisturbance(rv.val,rv.idx);				
				});
			
			svr.listen(url,port);
			
			cout<<"Servidor Finalizado"<<endl;

		});
	}
	
};

#endif