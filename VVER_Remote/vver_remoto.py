from PySide2 import QtCore
from PySide2.QtCore import Qt, Signal, Slot, QObject, QTimer
from PySide2.QtWidgets import QLabel,QWidget,QApplication,QHBoxLayout, QVBoxLayout, QSizePolicy, QCheckBox
from PySide2.QtWidgets import QGridLayout, QLineEdit, QPushButton, QGroupBox, QLCDNumber
from PySide2.QtGui import QFont
import sys
import numpy as np
import requests
import matplotlib.pyplot as plt

nInputs = 3; nDist = 5; nOut = 5; nStates = 6;
#extras
k=0; Ts = 0; T0 = 0;
nExtras = 3
inputs = np.zeros([nInputs+nExtras,1])
outputs = np.zeros([nOut+nExtras,1])
states = np.zeros([nStates+nExtras,1])
disturbances = np.zeros([nDist+nExtras,1])



host = "127.0.0.1"
port  = 80
url = ""
reqTimeout = 0.3 #s
timerInterval = 500 #ms


kT = np.zeros([1,1])

def decodeInputData(data):
    #Exemplo:{5.556352,11814670.291583,4223553.642286,1365072964.669739,224881304.736805},{179.000000,0.040000,0.000000}
    data = data.replace('{','').replace('}','')
    i = 1+data.count(',')
    x = np.zeros([i,1])
    #print(x.shape)
    parts = data.split(',')
    #print(len(parts))
    #print(parts)
    for n in range(i-1): x[n,0] = float(parts[n])
    #print(data)
    return x #np.fromstring(data,dtype=np.float)

class Communicate(QObject):
    inputValueChanged = Signal(float)


class InputWidget(QWidget):
    def __init__(self, linha1:str, linha2:str, inicial:float):
        self.c  = Communicate()
        super().__init__()
        vBox = QVBoxLayout(self)
        lb1 =QLabel(linha1)
        lb1.setAlignment(Qt.AlignHCenter)
        vBox.addWidget(lb1)
        self.inpTxt =QLineEdit(str(inicial))
        self.inpTxt.returnPressed.connect(self.filter)
        self.inpTxt.setSizePolicy(QSizePolicy.Ignored,QSizePolicy.Preferred)
        self.inpTxt.setMinimumWidth(50)
        if linha2!="":
            lb2 = QLabel(linha2)
            lb2.setAlignment(Qt.AlignHCenter)
            vBox.addWidget(lb2)
        vBox.addWidget(self.inpTxt)
        vBox.setAlignment(Qt.AlignLeft)

    def filter(self):
        x = float(self.inpTxt.text())
        self.c.inputValueChanged.emit(x)

class OutputWidget(QWidget):
    def __init__(self, line1:str, line2:str, valor:float):
        super().__init__()
        vBox = QVBoxLayout(self)
        lb1 = QLabel(line1)
        lb1.setAlignment(Qt.AlignCenter)
        self.out = QLCDNumber()
        self.out.setDigitCount(7)
        self.out.setSizePolicy(QSizePolicy.Preferred,QSizePolicy.Preferred)
        self.out.setMinimumSize(90,60)
        self.out.setSegmentStyle(QLCDNumber.SegmentStyle.Flat)
        self.out.display(valor)
        self.out.setStyleSheet("QLCDNumber { background-color: white;  color: red; }")
        vBox.addWidget(lb1)
        if line2!='':
            lb2 = QLabel(line2)
            lb2.setAlignment(Qt.AlignCenter)
            vBox.addWidget(lb2)
        vBox.addWidget(self.out)
        vBox.setAlignment(Qt.AlignTop|Qt.AlignLeft)




class MainWindow(QWidget):
    
    def initUI(self):
        self.setWindowTitle("Controle Remoto do Reator PWR")
        self.setGeometry(200,300,500,500)
        vBox = QVBoxLayout(self)
        hBox1 = QHBoxLayout()
        self.hostTxt = QLineEdit(host)
        self.portTxt = QLineEdit(str(port))
        self.btnConnect = QPushButton("Conectar")
        self.btnConnect.clicked.connect(self.on_btnConnect_clicked)
        hBox1.addWidget(QLabel("host:"))
        hBox1.addWidget(self.hostTxt)
        hBox1.addWidget(QLabel("Porta:"))
        hBox1.addWidget(self.portTxt)
        hBox1.addWidget(QLabel("Status:"))
        self.lbConnected = QLabel("Desconectado")
        hBox1.addWidget(self.lbConnected)
        hBox1.addWidget(self.btnConnect)
        #hBox1.setAlignment(Qt.AlignTop)
        vBox.addLayout(hBox1)
        hBox2 = QHBoxLayout()
        vBox.addLayout(hBox2)
        #hBox2.setAlignment(Qt.AlignTop)
 
        #Núcleo do Reator
        grpCore = QGroupBox("Núcleo (R)")
        #grpCore.setSizePolicy(QSizePolicy.Ignored,QSizePolicy.Preferred)
        grpCoreLayout = QVBoxLayout()
        grpCore.setLayout(grpCoreLayout)
        inBarras = InputWidget("Barras (v_R)","[cm]",0.0) 
        inBarras.c.inputValueChanged.connect(self.setRods)
        grpCoreLayout.addWidget(inBarras)
        self.outPotNucleo = OutputWidget("Pot Núcleo (W_R)","[MW]",1365.5)
        grpCoreLayout.addWidget(self.outPotNucleo)
        hBox2.addWidget(grpCore)

        grpPressurizer = QGroupBox("Pressurizador(PR)")
        grpPressurizerLayout = QVBoxLayout()
        grpPressurizer.setLayout(grpPressurizerLayout)
        inAquecPR = InputWidget("Pot Ent PR ","(W_heat_PR)[kW]",168.0)
        inAquecPR.c.inputValueChanged.connect(self.setHeater)
        grpPressurizerLayout.addWidget(inAquecPR)
        self.outPresPR = OutputWidget("Pres (p_PR)","[bar]",118.145)
        grpPressurizerLayout.addWidget(self.outPresPR)
        hBox2.addWidget(grpPressurizer)

        grpSteam = QGroupBox("Gerador de Vapor (SG)")
        grpSteamLayout = QVBoxLayout()
        grpSteam.setLayout(grpSteamLayout)
        inVazSG = InputWidget("Vaz Liq SG (m_SG)","[kg/s]",119.3)
        inVazSG.c.inputValueChanged.connect(self.setSGFlow)
        grpSteamLayout.addWidget(inVazSG)
        self.outPotSG = OutputWidget("Pot Req (W_SG)","[MW]",1349)
        grpSteamLayout.addWidget(self.outPotSG)
        hBox2.addWidget(grpSteam)

        lbGraf = QLabel("Gráficos")
        lbGraf.setAlignment(Qt.AlignHCenter)
        vBox.addWidget(lbGraf)
        vBox.addStretch(1)

        self.cbW_SG = QCheckBox("Potência no Gerador de Vapor (W_SG)")
        self.cbW_SG.setChecked(False)
        self.cbW_SG.stateChanged.connect(self.chart_W_SG)


        self.cbp_PR = QCheckBox("Pressão no Pressurizador(p_PR)")
        self.cbp_PR.stateChanged.connect(self.chart_p_PR)
        self.cbp_PR.setChecked(False)
        self.cbm_SG = QCheckBox("Vazão Líquida no Gerador de vapor(m_SG)")
        self.cbm_SG.stateChanged.connect(self.chart_m_SG)
        self.cbm_SG.setChecked(False)
        
        vBox.addWidget(self.cbW_SG)
        vBox.addWidget(self.cbp_PR)
        vBox.addWidget(self.cbm_SG)
        vBox.addStretch(1)


    def chart_p_PR(self,state):
        if(state==Qt.Checked):
            global outputs
            t = outputs[-3,:]*outputs[-2,:]
            y = outputs[1,:]*1e-5
            plt.plot(t,y)
            plt.show()
            #self.cbp_PR.setChecked(False)

    def chart_m_SG(self,state):
        if(state==Qt.Checked):
            global disturbances
            t = disturbances[-3,:]*disturbances[-2,:]
            y = disturbances[1,:]
            plt.plot(t,y)
            plt.show()
            #self.cbm_SG.setChecked(False)

    def chart_W_SG(self,state):
        if(state==Qt.Checked):
            global outputs
            t = outputs[-3,:]*outputs[-2,:]
            y = outputs[4,:]*1e-6
            plt.plot(t,y)
            plt.show()
            #self.cbW_SG.setChecked(False)
        


    

    def onTimer(self):
        try:
            global url
            #print(url)
            ri = requests.get(url+'inputs', timeout=reqTimeout)
            ro = requests.get(url+'outputs', timeout=reqTimeout)
            rd = requests.get(url+'disturbances', timeout=reqTimeout)
            #print(ri.text)
            di = decodeInputData(ri.text)
            global inputs, outputs, disturbances
            inputs=np.append(inputs,di,axis=1) #aumenta uma coluna na matriz de entradas
            do = decodeInputData(ro.text)
            outputs = np.append(outputs,do,axis=1)
            dd = decodeInputData(rd.text)
            disturbances = np.append(disturbances,dd,axis=1)
            
            self.outPotNucleo.out.display(do[3,0]/1e6)
            self.outPresPR.out.display(do[1,0]/1e5)
            self.outPotSG.out.display(do[4,0]*6e-6)
            
            print(inputs.shape)
            
            
        except(ConnectionError, requests.exceptions.RequestException):
            print("Não é possível conectar")
            self.lbConnected.setText("Falha de Conexão")
            self.btnConnect.setText("Conectar")
            self.connected=False
            return

        if ri.ok:
            self.lbConnected.setText("Conectado")
            self.btnConnect.setText("Desconectar")
            self.connected=True
        

       
    def testSignal(self,val):
        print(val)
    #in
    #m_in=0, v_R, W_heat_PR, //ones,
    #dist
    # m_out=0, m_SG, M_SG, T_PC_I, T_SG_SW, 
    #out
    #l_PR=0, p_PR, p_SG, W_R, W_SG,
    #sta
    #N=0, m_PR, M_PC, T_PC, T_PR, T_SG,
    def setRods(self,val):
        if not self.connected: return
        print("set rods")
        #{ "idx" : "v_R", "val":"12.3"}
        try:
            global url
            print(url)
            r = requests.post(url+'input', data = {'idx':'v_R','val':str(val)})
        except (ConnectionError, requests.exceptions.RequestException):
            print("erro ao enviar v_R")

        
    def setHeater(self,val):
        try:
            global url
            r = requests.post(url+'input', data = {'idx':'W_heat_PR', 'val':str(val)})
        except (ConnectionError, requests.exceptions.RequestException):
            print("erro ao enviar W_heat_PR") 
        
    def setSGFlow(self,val):
        try:
            global url
            r = requests.post(url+'disturbance', data = {'idx':'m_SG', 'val':str(val)})
        except (ConnectionError, requests.exceptions.RequestException):
            print("erro ao enviar m_SG") 
        
        
        
    def __init__(self):
        super().__init__()
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.onTimer)
        self.initUI()
        self.connected = False



    def on_btnConnect_clicked(self):
        if(self.connected):
            self.timer.stop()
            self.btnConnect.setText("Conectar")
            self.connected = False
            return
        host = self.hostTxt.text()
        port = self.portTxt.text()
        global url
        url = 'http://'+str(host)+':'+str(port)+'/'
        if not self.timer.isActive():
            self.timer.start(timerInterval) # 500 milissegundos


if __name__=="__main__":
    app = QApplication(sys.argv)
    font = QFont("Arial",10,0)
    font.setStyleHint(QFont.Monospace)
    app.setFont(font)
    win = MainWindow()
    win.show()
    app.exec_()