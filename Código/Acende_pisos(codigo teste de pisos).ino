#include <Wire.h>
#include <TimerOne.h>

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

int contador_pisos=0;
int c=0;
boolean flag=false;

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
//  // Configuração do TIMER1
 noInterrupts();           // disable all interrupts
  TCCR1A = 0;                //confira timer para operação normal
 TCCR1B = 0;                //limpa registrador
  TCNT1  = 0;                //zera temporizado

  OCR1A = 120;            // carrega registrador de comparação: 16MHz/1024/1Hz = 15625 = 0X3D09   62
  TCCR1B |= (1 << WGM12)|(1<<CS12)|(0 << CS10);   // modo CTC, prescaler de 1024: CS12 = 1 e CS10 = 1  
  TIMSK1 |= (1 << OCIE1A);  // habilita interrupção por igualdade de comparação
  interrupts();           // ativate all interrupts();
  
//  Timer1.initialize(50);                  //inicializar timer 4 milisegundos(percorrer cada andar)
//  Timer1.attachInterrupt(refreshDisplay);      //salta para a funcao 'refreshDisplay'
 

 

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



ISR(TIMER1_COMPA_vect)          // interrupção por igualdade de comparação no TIMER1
{
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




void loop() {
   
  int MMCP_GPIOA_anterior=1;
  int MMCP_GPIOA_escrever=0;
  
  if(flag==true){
    
        if(c==0)//piso 0
  {
//    if(MMCP_GPIOA_escrever!=MMCP_GPIOA_anterior){
//      MCPwrite(enderecoMMCP, GPIOA, 0x00);  
//      MMCP_GPIOA_anterior=MMCP_GPIOA_escrever;
//    }
    MCPwrite(enderecoEMCP, GPIOA, 0xFF); 
    MCPwrite(enderecoEMCP, GPIOB, 0x00); 
    MCPwrite(enderecoMMCP, GPIOA, 0xFF);
    MCPwrite(enderecoMMCP, GPIOB, 0xFF); 
    MCPwrite(enderecoDMCP, GPIOA, 0xFF);
    MCPwrite(enderecoDMCP, GPIOB, 0xFF); 
    }
    else if(c==2){//piso 1
//    if(MMCP_GPIOA_escrever!=MMCP_GPIOA_anterior){
//      MCPwrite(enderecoMMCP, GPIOA, 0x00);  
//      MMCP_GPIOA_anterior=MMCP_GPIOA_escrever;
//    }
      MCPwrite(enderecoEMCP, GPIOA, 0x00);  
MCPwrite(enderecoEMCP, GPIOB, 0xFF); 
    MCPwrite(enderecoMMCP, GPIOA, 0xFF);
    MCPwrite(enderecoMMCP, GPIOB, 0xFF);
    MCPwrite(enderecoDMCP, GPIOA, 0xFF);
    MCPwrite(enderecoDMCP, GPIOB, 0xFF);
    }
    else  if(c==4){//piso 2
//    if(MMCP_GPIOA_escrever!=MMCP_GPIOA_anterior){
//      MCPwrite(enderecoMMCP, GPIOA, 0x00);  
//      MMCP_GPIOA_anterior=MMCP_GPIOA_escrever;
//    }
    MCPwrite(enderecoEMCP, GPIOA, 0xFF);  
    MCPwrite(enderecoEMCP, GPIOB, 0xFF); 
    MCPwrite(enderecoMMCP, GPIOA, 0xFF);
    MCPwrite(enderecoMMCP, GPIOB, 0x00); 
    MCPwrite(enderecoDMCP, GPIOA, 0xFF);
    MCPwrite(enderecoDMCP, GPIOB, 0xFF); 
    
    }
    else  if(c==6){//piso 3
//    if(MMCP_GPIOA_escrever!=MMCP_GPIOA_anterior){
//      MCPwrite(enderecoMMCP, GPIOA, 0x00);  
//      MMCP_GPIOA_anterior=MMCP_GPIOA_escrever;
//    }
MCPwrite(enderecoEMCP, GPIOA, 0xFF);  
    MCPwrite(enderecoEMCP, GPIOB, 0xFF); 
    MCPwrite(enderecoMMCP, GPIOA, 0x00);
    MCPwrite(enderecoMMCP, GPIOB, 0xFF); 
    MCPwrite(enderecoDMCP, GPIOA, 0xFF);
    MCPwrite(enderecoDMCP, GPIOB, 0xFF); 
    }
     else  if(c==8){//piso 4
//    if(MMCP_GPIOA_escrever!=MMCP_GPIOA_anterior){
//      MCPwrite(enderecoMMCP, GPIOA, 0x00);  
//      MMCP_GPIOA_anterior=MMCP_GPIOA_escrever;
//    }
    MCPwrite(enderecoEMCP, GPIOA, 0xFF);  
    MCPwrite(enderecoEMCP, GPIOB, 0xFF); 
    MCPwrite(enderecoMMCP, GPIOA, 0xFF);
    MCPwrite(enderecoMMCP, GPIOB, 0xFF); 
    MCPwrite(enderecoDMCP, GPIOA, 0xFF);
    MCPwrite(enderecoDMCP, GPIOB, 0x00); 
    }
    else if(c==10){//piso 5
//    if(MMCP_GPIOA_escrever!=MMCP_GPIOA_anterior){
//      MCPwrite(enderecoMMCP, GPIOA, 0x00);  
//      MMCP_GPIOA_anterior=MMCP_GPIOA_escrever;
//    }
    MCPwrite(enderecoEMCP, GPIOA, 0xFF);  
    MCPwrite(enderecoEMCP, GPIOB, 0xFF); 
    MCPwrite(enderecoMMCP, GPIOA, 0xFF);
    MCPwrite(enderecoMMCP, GPIOB, 0xFF); 
    MCPwrite(enderecoDMCP, GPIOA, 0x00);
    MCPwrite(enderecoDMCP, GPIOB, 0xFF);  
    }
    flag=false;
    
    digitalWrite(Clock, HIGH);     
  }
   
}




