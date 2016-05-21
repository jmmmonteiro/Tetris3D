#include <Wire.h>

#define Clock 2    //pins micro I2C comunicacao
#define dados 3

#define enderecoEMCP 0x24 //E->esquerda
#define enderecoMMCP 0x26 //M->meio
#define enderecoDMCP 0x27 //D->direita

#define IODIRA  0x00  // Port A direction register. Write a 0 to make a pin an output, a 1 to make it an input
#define IODIRB  0x01    // Port B direction register. Write a 0 to make a pin an output, a 1 to make it an input
#define GPIOA   0x12    // Register Address of Port A - read data from or write output data to this port
#define GPIOB   0x13    // Register Address of Port B - read data from or write output data to this port

#define xTorre 6
#define yTorre 6
#define zTorre 6
#define numPecas 10
#define numMCP 3

//int contador_pisos=0;
int c=0;
boolean flag=false;
boolean fundo_torre[xTorre][yTorre][zTorre]={0}; //vetor que guarda informação sobre que leds acessos/apagados fundo
boolean ativo_torre[xTorre][yTorre][zTorre]={0}; ////vetor que guarda informação sobre que leds acessos ativos(peca em queda)


void MCPwrite(byte i2cAddress, byte i2cRegister, byte i2cData){

  Wire.beginTransmission(i2cAddress);    //endereco dispositivo
  Wire.write(i2cRegister);              //need to set up the proper register before reading it
  Wire.write(i2cData);                  //escrever para as saidas do respectivo lado
  Wire.endTransmission();

}

//**********************************************************************************************************************funcao ler no MCP...***********************************************************************************************************************************
byte MCPread(byte i2cAddress, byte i2cRegister){

   byte receive_data=0x00;

   Wire.beginTransmission(i2cAddress);
   Wire.write(i2cRegister);
   Wire.endTransmission();

   Wire.requestFrom(i2cAddress,(byte)1);   //Now we start the actual read
   while(Wire.available()!=1);             //wait unit the data gets back
   receive_data=Wire.read();
   //receive_data = ~Wire.read()         // usar caso logica negado botoes (get the data, note bitwise inversion to make it easier to tell what switch is closed)

  return receive_data;
}

void setup() {

  pinMode(Clock, OUTPUT); //mudar para os define, mas confirmar se estão certos pode o 2 e 3 estar ao contrario
  pinMode(dados, OUTPUT);
  Serial.begin(9600);  //configure the serial port so we can use the Serial Monitor
  randomSeed(analogRead(1));           //semear semente para depois escolher as pecas

  //temporizador para POV(passados 250*4=1segundo, a peca desce um andar)
  // Configuração do TIMER1
 noInterrupts();           // disable all interrupts
  TCCR1A = 0;                //confira timer para operação normal
 TCCR1B = 0;                //limpa registrador
  TCNT1  = 0;                //zera temporizado

  OCR1A = 120;            // carrega registrador de comparação: 16MHz/1024/1Hz = 15625 = 0X3D09   62
  TCCR1B |= (1 << WGM12)|(1<<CS12)|(0 << CS10);   // modo CTC, prescaler de 1024: CS12 = 1 e CS10 = 1
  TIMSK1 |= (1 << OCIE1A);  // habilita interrupção por igualdade de comparação
  interrupts();           // ativate all interrupts();
 //transmissao I2C
 Wire.begin(); // wake up I2C bus

  // set I/O pins to inputs/outputs
 Wire.beginTransmission(enderecoEMCP); //indicar endereco dispositivo
 Wire.write(IODIRA); // select register to a write
 Wire.write(0x00); // set flanc A of MCP.... 0 = output, 1 = input                             <--------------ver onde ligados botoes e alterar 0x00 para .....
 Wire.endTransmission();

 Wire.beginTransmission(enderecoEMCP);
 Wire.write(IODIRB); // IODIRB register
 Wire.write(0x00); // set all of port B to outputs
 Wire.endTransmission();

 Wire.beginTransmission(enderecoMMCP);
 Wire.write(IODIRA); // IODIRA register
 Wire.write(0x00); // set all of port A to outputs
 Wire.endTransmission();

 Wire.beginTransmission(enderecoMMCP);
 Wire.write(IODIRB); // IODIRB register
 Wire.write(0x00); // set all of port B to outputs
 Wire.endTransmission();

 Wire.beginTransmission(enderecoDMCP);
 Wire.write(IODIRA); // IODIRA register
 Wire.write(0x00); // set all of port A to outputs
 Wire.endTransmission();

 Wire.beginTransmission(enderecoDMCP);
 Wire.write(IODIRB); // IODIRB register
 Wire.write(0x00); // set all of port B to outputs
 Wire.endTransmission();

 //digitalWrite(dados, HIGH);
}

ISR(TIMER1_COMPA_vect){          // interrupção por igualdade de comparação no TIMER1
  //if(flag==true){
    flag=true;
    digitalWrite(Clock, LOW);
    digitalWrite(dados, LOW);
    c=c+1;

    if(c>11){
      c=0;
      digitalWrite(dados, HIGH);
    }
    //digitalWrite(dados, LOW);
  //  flag=true;
  }
}

byte renderingEGPIOB(char andar){
  char i;
  byte EGPIOB=0x00;

  for(i=0;i<xTorre;i++){

        if(fundo_torre[i][0][andar]==1 || ativo_torre[i][0][andar]==1){
          if(i==0)
            EGPIOB |=0x01;
          else if(i==1)
            EGPIOB |=0x02;
          else if(i==2)
            EGPIOB |=0x04;
          else if(i==3)
            EGPIOB |=0x08;
          else if(i==4)
            EGPIOB |=0x10;
          else if(i==5)
            EGPIOB |=0x20;
        }
  }
  EGPIOB=!EGPIOB;
  return EGPIOB;
}

byte renderingEGPIOA(char andar){
  char i;
  byte EGPIOA=0x00;

  for(i=0;i<xTorre;i++){

        if(fundo_torre[i][1][andar]==1 || ativo_torre[i][1][andar]==1){
          if(i==0)
            EGPIOA |=0x04;
          else if(i==1)
            EGPIOA |=0x08;
          else if(i==2)
            EGPIOA |=0x10;
          else if(i==3)
            EGPIOA |=0x20;
          else if(i==4)
            EGPIOA |=0x40;
          else if(i==5)
            EGPIOA |=0x80;
        }

  }
  EGPIOA=!EGPIOA;
  return EGPIOA;
}

byte renderingMGPIOB(char andar){
  char i;
  byte MGPIOB=0x00;

  for(i=0;i<xTorre;i++){

        if(fundo_torre[i][2][andar]==1 || ativo_torre[i][2][andar]==1){
          if(i==0)
           MGPIOB |=0x01;
          else if(i==1)
           MGPIOB |=0x02;
          else if(i==2)
           MGPIOB |=0x04;
          else if(i==3)
           MGPIOB |=0x08;
          else if(i==4)
           MGPIOB |=0x10;
          else if(i==5)
           MGPIOB |=0x20;
        }

  }
  MGPIOB=!MGPIOB;
  return MGPIOB;
}

byte renderingMGPIOA(char andar){
  char i;
  byte MGPIOA=0x00;

  for(i=0;i<xTorre;i++){

        if(fundo_torre[i][3][andar]==1 || ativo_torre[i][3][andar]==1){
          if(i==0)
           MGPIOA |=0x04;
          else if(i==1)
           MGPIOA |=0x08;
          else if(i==2)
           MGPIOA |=0x10;
          else if(i==3)
           MGPIOA |=0x20;
          else if(i==4)
           MGPIOA |=0x40;
          else if(i==5)
           MGPIOA |=0x80;
        }

  }
  MGPIOA=!MGPIOA;
  return MGPIOA;
}

byte renderingDGPIOB(char andar){
  char i;
  byte DGPIOB=0x00;

  for(i=0;i<xTorre;i++){

        if(fundo_torre[i][4][andar]==1 || ativo_torre[i][4][andar]==1){
          if(i==0)
           DGPIOB |=0x01;
          else if(i==1)
           DGPIOB |=0x02;
          else if(i==2)
           DGPIOB |=0x04;
          else if(i==3)
           DGPIOB |=0x08;
          else if(i==4)
           DGPIOB |=0x10;
          else if(i==5)
           DGPIOB |=0x20;
        }

  }
  DGPIOB=!DGPIOB;
  return DGPIOB;
}

byte renderingDGPIOA(char andar){
  char i;
  byte DGPIOA=0x00;

  for(i=0;i<xTorre;i++){

        if(fundo_torre[i][5][andar]==1 || ativo_torre[i][5][andar]==1){
          if(i==0)
           DGPIOA |=0x04;
          else if(i==1)
           DGPIOA |=0x08;
          else if(i==2)
           DGPIOA |=0x10;
          else if(i==3)
           DGPIOA |=0x20;
          else if(i==4)
           DGPIOA |=0x40;
          else if(i==5)
           DGPIOA |=0x80;
        }

  }
  DGPIOA=!DGPIOA;
  return DGPIOA;
}

void loop() {

  //int MMCP_GPIOA_anterior;
  //int MMCP_GPIOA_escrever;
  byte EMCP_GPIOB_anterior;
  byte EMCP_GPIOB_escrever;
  byte EMCP_GPIOA_anterior;
  byte EMCP_GPIOA_escrever;
  byte MMCP_GPIOB_anterior;
  byte MMCP_GPIOB_escrever;
  byte MMCP_GPIOA_anterior;
  byte MMCP_GPIOA_escrever;
  byte DMCP_GPIOB_anterior;
  byte DMCP_GPIOB_escrever;
  byte DMCP_GPIOA_anterior;
  byte DMCP_GPIOA_escrever;
  if(flag==true){

    if(c==0)//piso 0
    {
      EMCP_GPIOB_escrever=renderingEGPIOB(0x00);//rendering da primeira linha
      if(EMCP_GPIOB_escrever!=EMCP_GPIOB_anterior){
        MCPwrite(enderecoEMCP, GPIOA, EMCP_GPIOB_escrever);
        EMCP_GPIOB_anterior=EMCP_GPIOB_escrever;
      }

      EMCP_GPIOA_escrever=renderingEGPIOA(0x00);//rendering da segunda linha
      if(EMCP_GPIOA_escrever!=EMCP_GPIOA_anterior){
        MCPwrite(enderecoEMCP, GPIOA, EMCP_GPIOA_escrever);
        EMCP_GPIOA_anterior=EMCP_GPIOA_escrever;
      }

      MMCP_GPIOB_escrever=renderingMGPIOB(0x00);//rendering da terceira linha
      if(MMCP_GPIOB_escrever!=MMCP_GPIOB_anterior){
        MCPwrite(enderecoMMCP, GPIOB, MMCP_GPIOB_escrever);
        MMCP_GPIOB_anterior=MMCP_GPIOB_escrever;
      }

      MMCP_GPIOA_escrever=renderingMGPIOA(0x00);//rendering da quarta linha
      if(MMCP_GPIOA_escrever!=MMCP_GPIOA_anterior){
        MCPwrite(enderecoMMCP, GPIOA, MMCP_GPIOA_escrever);
        MMCP_GPIOA_anterior=MMCP_GPIOA_escrever;
      }

      DMCP_GPIOB_escrever=renderingDGPIOB(0x00);//rendering da quinta linha
      if(DMCP_GPIOB_escrever!=DMCP_GPIOB_anterior){
        MCPwrite(enderecoDMCP, GPIOB, DMCP_GPIOB_escrever);
        DMCP_GPIOB_anterior=DMCP_GPIOB_escrever;
      }

      DMCP_GPIOA_escrever=renderingDGPIOA(0x00);//rendering da sexta linha
      if(DMCP_GPIOA_escrever!=DMCP_GPIOA_anterior){
        MCPwrite(enderecoDMCP, GPIOA, DMCP_GPIOA_escrever);
        DMCP_GPIOA_anterior=DMCP_GPIOA_escrever;
      }

    // MCPwrite(enderecoEMCP, GPIOA, 0xFF);
    // MCPwrite(enderecoEMCP, GPIOB, 0x00);
    // MCPwrite(enderecoMMCP, GPIOA, 0xFF);
    // MCPwrite(enderecoMMCP, GPIOB, 0xFF);
    // MCPwrite(enderecoDMCP, GPIOA, 0xFF);
    // MCPwrite(enderecoDMCP, GPIOB, 0xFF);
    }
    else if(c==2){//piso 1

      EMCP_GPIOB_escrever=renderingEGPIOB(0x01);//rendering da primeira linha
      if(EMCP_GPIOB_escrever!=EMCP_GPIOB_anterior){
        MCPwrite(enderecoEMCP, GPIOA, EMCP_GPIOB_escrever);
        EMCP_GPIOB_anterior=EMCP_GPIOB_escrever;
      }

      EMCP_GPIOA_escrever=renderingEGPIOA(0x01);//rendering da segunda linha
      if(EMCP_GPIOA_escrever!=EMCP_GPIOA_anterior){
        MCPwrite(enderecoEMCP, GPIOA, EMCP_GPIOA_escrever);
        EMCP_GPIOA_anterior=EMCP_GPIOA_escrever;
      }

      MMCP_GPIOB_escrever=renderingMGPIOB(0x01);//rendering da terceira linha
      if(MMCP_GPIOB_escrever!=MMCP_GPIOB_anterior){
        MCPwrite(enderecoMMCP, GPIOB, MMCP_GPIOB_escrever);
        MMCP_GPIOB_anterior=MMCP_GPIOB_escrever;
      }

      MMCP_GPIOA_escrever=renderingMGPIOA(0x01);//rendering da quarta linha
      if(MMCP_GPIOA_escrever!=MMCP_GPIOA_anterior){
        MCPwrite(enderecoMMCP, GPIOA, MMCP_GPIOA_escrever);
        MMCP_GPIOA_anterior=MMCP_GPIOA_escrever;
      }

      DMCP_GPIOB_escrever=renderingDGPIOB(0x01);//rendering da quinta linha
      if(DMCP_GPIOB_escrever!=DMCP_GPIOB_anterior){
        MCPwrite(enderecoDMCP, GPIOB, DMCP_GPIOB_escrever);
        DMCP_GPIOB_anterior=DMCP_GPIOB_escrever;
      }

      DMCP_GPIOA_escrever=renderingDGPIOA(0x01);//rendering da sexta linha
      if(DMCP_GPIOA_escrever!=DMCP_GPIOA_anterior){
        MCPwrite(enderecoDMCP, GPIOA, DMCP_GPIOA_escrever);
        DMCP_GPIOA_anterior=DMCP_GPIOA_escrever;
      }
    /*
    MCPwrite(enderecoEMCP, GPIOA, 0x00);
    MCPwrite(enderecoEMCP, GPIOB, 0xFF);
    MCPwrite(enderecoMMCP, GPIOA, 0xFF);
    MCPwrite(enderecoMMCP, GPIOB, 0xFF);
    MCPwrite(enderecoDMCP, GPIOA, 0xFF);
    MCPwrite(enderecoDMCP, GPIOB, 0xFF);
    */
    }
    else  if(c==4){//piso 2

     EMCP_GPIOB_escrever=renderingEGPIOB(0x02);//rendering da primeira linha
      if(EMCP_GPIOB_escrever!=EMCP_GPIOB_anterior){
        MCPwrite(enderecoEMCP, GPIOA, EMCP_GPIOB_escrever);
        EMCP_GPIOB_anterior=EMCP_GPIOB_escrever;
      }

      EMCP_GPIOA_escrever=renderingEGPIOA(0x02);//rendering da segunda linha
      if(EMCP_GPIOA_escrever!=EMCP_GPIOA_anterior){
        MCPwrite(enderecoEMCP, GPIOA, EMCP_GPIOA_escrever);
        EMCP_GPIOA_anterior=EMCP_GPIOA_escrever;
      }

      MMCP_GPIOB_escrever=renderingMGPIOB(0x02);//rendering da terceira linha
      if(MMCP_GPIOB_escrever!=MMCP_GPIOB_anterior){
        MCPwrite(enderecoMMCP, GPIOB, MMCP_GPIOB_escrever);
        MMCP_GPIOB_anterior=MMCP_GPIOB_escrever;
      }

      MMCP_GPIOA_escrever=renderingMGPIOA(0x02);//rendering da quarta linha
      if(MMCP_GPIOA_escrever!=MMCP_GPIOA_anterior){
        MCPwrite(enderecoMMCP, GPIOA, MMCP_GPIOA_escrever);
        MMCP_GPIOA_anterior=MMCP_GPIOA_escrever;
      }

      DMCP_GPIOB_escrever=renderingDGPIOB(0x02);//rendering da quinta linha
      if(DMCP_GPIOB_escrever!=DMCP_GPIOB_anterior){
        MCPwrite(enderecoDMCP, GPIOB, DMCP_GPIOB_escrever);
        DMCP_GPIOB_anterior=DMCP_GPIOB_escrever;
      }

      DMCP_GPIOA_escrever=renderingDGPIOA(0x02);//rendering da sexta linha
      if(DMCP_GPIOA_escrever!=DMCP_GPIOA_anterior){
        MCPwrite(enderecoDMCP, GPIOA, DMCP_GPIOA_escrever);
        DMCP_GPIOA_anterior=DMCP_GPIOA_escrever;
      }
    /*
    MCPwrite(enderecoEMCP, GPIOA, 0xFF);
    MCPwrite(enderecoEMCP, GPIOB, 0xFF);
    MCPwrite(enderecoMMCP, GPIOA, 0xFF);
    MCPwrite(enderecoMMCP, GPIOB, 0x00);
    MCPwrite(enderecoDMCP, GPIOA, 0xFF);
    MCPwrite(enderecoDMCP, GPIOB, 0xFF);
    */
    }
    else  if(c==6){//piso 3

      EMCP_GPIOB_escrever=renderingEGPIOB(0x03);//rendering da primeira linha
      if(EMCP_GPIOB_escrever!=EMCP_GPIOB_anterior){
        MCPwrite(enderecoEMCP, GPIOA, EMCP_GPIOB_escrever);
        EMCP_GPIOB_anterior=EMCP_GPIOB_escrever;
      }

      EMCP_GPIOA_escrever=renderingEGPIOA(0x03);//rendering da segunda linha
      if(EMCP_GPIOA_escrever!=EMCP_GPIOA_anterior){
        MCPwrite(enderecoEMCP, GPIOA, EMCP_GPIOA_escrever);
        EMCP_GPIOA_anterior=EMCP_GPIOA_escrever;
      }

      MMCP_GPIOB_escrever=renderingMGPIOB(0x03);//rendering da terceira linha
      if(MMCP_GPIOB_escrever!=MMCP_GPIOB_anterior){
        MCPwrite(enderecoMMCP, GPIOB, MMCP_GPIOB_escrever);
        MMCP_GPIOB_anterior=MMCP_GPIOB_escrever;
      }

      MMCP_GPIOA_escrever=renderingMGPIOA(0x03);//rendering da quarta linha
      if(MMCP_GPIOA_escrever!=MMCP_GPIOA_anterior){
        MCPwrite(enderecoMMCP, GPIOA, MMCP_GPIOA_escrever);
        MMCP_GPIOA_anterior=MMCP_GPIOA_escrever;
      }

      DMCP_GPIOB_escrever=renderingDGPIOB(0x03);//rendering da quinta linha
      if(DMCP_GPIOB_escrever!=DMCP_GPIOB_anterior){
        MCPwrite(enderecoDMCP, GPIOB, DMCP_GPIOB_escrever);
        DMCP_GPIOB_anterior=DMCP_GPIOB_escrever;
      }

      DMCP_GPIOA_escrever=renderingDGPIOA(0x03);//rendering da sexta linha
      if(DMCP_GPIOA_escrever!=DMCP_GPIOA_anterior){
        MCPwrite(enderecoDMCP, GPIOA, DMCP_GPIOA_escrever);
        DMCP_GPIOA_anterior=DMCP_GPIOA_escrever;
      }
     /*
    MCPwrite(enderecoEMCP, GPIOA, 0xFF);
    MCPwrite(enderecoEMCP, GPIOB, 0xFF);
    MCPwrite(enderecoMMCP, GPIOA, 0x00);
    MCPwrite(enderecoMMCP, GPIOB, 0xFF);
    MCPwrite(enderecoDMCP, GPIOA, 0xFF);
    MCPwrite(enderecoDMCP, GPIOB, 0xFF);
    */
    }
     else  if(c==8){//piso 4

      EMCP_GPIOB_escrever=renderingEGPIOB(0x04);//rendering da primeira linha
      if(EMCP_GPIOB_escrever!=EMCP_GPIOB_anterior){
        MCPwrite(enderecoEMCP, GPIOA, EMCP_GPIOB_escrever);
        EMCP_GPIOB_anterior=EMCP_GPIOB_escrever;
      }

      EMCP_GPIOA_escrever=renderingEGPIOA(0x04);//rendering da segunda linha
      if(EMCP_GPIOA_escrever!=EMCP_GPIOA_anterior){
        MCPwrite(enderecoEMCP, GPIOA, EMCP_GPIOA_escrever);
        EMCP_GPIOA_anterior=EMCP_GPIOA_escrever;
      }

      MMCP_GPIOB_escrever=renderingMGPIOB(0x4);//rendering da terceira linha
      if(MMCP_GPIOB_escrever!=MMCP_GPIOB_anterior){
        MCPwrite(enderecoMMCP, GPIOB, MMCP_GPIOB_escrever);
        MMCP_GPIOB_anterior=MMCP_GPIOB_escrever;
      }

      MMCP_GPIOA_escrever=renderingMGPIOA(0x04);//rendering da quarta linha
      if(MMCP_GPIOA_escrever!=MMCP_GPIOA_anterior){
        MCPwrite(enderecoMMCP, GPIOA, MMCP_GPIOA_escrever);
        MMCP_GPIOA_anterior=MMCP_GPIOA_escrever;
      }

      DMCP_GPIOB_escrever=renderingDGPIOB(0x04);//rendering da quinta linha
      if(DMCP_GPIOB_escrever!=DMCP_GPIOB_anterior){
        MCPwrite(enderecoDMCP, GPIOB, DMCP_GPIOB_escrever);
        DMCP_GPIOB_anterior=DMCP_GPIOB_escrever;
      }

      DMCP_GPIOA_escrever=renderingDGPIOA(0x04);//rendering da sexta linha
      if(DMCP_GPIOA_escrever!=DMCP_GPIOA_anterior){
        MCPwrite(enderecoDMCP, GPIOA, DMCP_GPIOA_escrever);
        DMCP_GPIOA_anterior=DMCP_GPIOA_escrever;
      }
    /*
    MCPwrite(enderecoEMCP, GPIOA, 0xFF);
    MCPwrite(enderecoEMCP, GPIOB, 0xFF);
    MCPwrite(enderecoMMCP, GPIOA, 0xFF);
    MCPwrite(enderecoMMCP, GPIOB, 0xFF);
    MCPwrite(enderecoDMCP, GPIOA, 0xFF);
    MCPwrite(enderecoDMCP, GPIOB, 0x00);
    */
    }
    else if(c==10){//piso 5

      EMCP_GPIOB_escrever=renderingEGPIOB(0x05);//rendering da primeira linha
      if(EMCP_GPIOB_escrever!=EMCP_GPIOB_anterior){
        MCPwrite(enderecoEMCP, GPIOA, EMCP_GPIOB_escrever);
        EMCP_GPIOB_anterior=EMCP_GPIOB_escrever;
      }

      EMCP_GPIOA_escrever=renderingEGPIOA(0x05);//rendering da segunda linha
      if(EMCP_GPIOA_escrever!=EMCP_GPIOA_anterior){
        MCPwrite(enderecoEMCP, GPIOA, EMCP_GPIOA_escrever);
        EMCP_GPIOA_anterior=EMCP_GPIOA_escrever;
      }

      MMCP_GPIOB_escrever=renderingMGPIOB(0x05);//rendering da terceira linha
      if(MMCP_GPIOB_escrever!=MMCP_GPIOB_anterior){
        MCPwrite(enderecoMMCP, GPIOB, MMCP_GPIOB_escrever);
        MMCP_GPIOB_anterior=MMCP_GPIOB_escrever;
      }

      MMCP_GPIOA_escrever=renderingMGPIOA(0x05);//rendering da quarta linha
      if(MMCP_GPIOA_escrever!=MMCP_GPIOA_anterior){
        MCPwrite(enderecoMMCP, GPIOA, MMCP_GPIOA_escrever);
        MMCP_GPIOA_anterior=MMCP_GPIOA_escrever;
      }

      DMCP_GPIOB_escrever=renderingDGPIOB(0x05);//rendering da quinta linha
      if(DMCP_GPIOB_escrever!=DMCP_GPIOB_anterior){
        MCPwrite(enderecoDMCP, GPIOB, DMCP_GPIOB_escrever);
        DMCP_GPIOB_anterior=DMCP_GPIOB_escrever;
      }

      DMCP_GPIOA_escrever=renderingDGPIOA(0x05);//rendering da sexta linha
      if(DMCP_GPIOA_escrever!=DMCP_GPIOA_anterior){
        MCPwrite(enderecoDMCP, GPIOA, DMCP_GPIOA_escrever);
        DMCP_GPIOA_anterior=DMCP_GPIOA_escrever;
      }
    /*
    MCPwrite(enderecoEMCP, GPIOA, 0xFF);
    MCPwrite(enderecoEMCP, GPIOB, 0xFF);
    MCPwrite(enderecoMMCP, GPIOA, 0xFF);
    MCPwrite(enderecoMMCP, GPIOB, 0xFF);
    MCPwrite(enderecoDMCP, GPIOA, 0x00);
    MCPwrite(enderecoDMCP, GPIOB, 0xFF);
    */
    }
    flag=false;

    digitalWrite(Clock, HIGH);
  }

}
