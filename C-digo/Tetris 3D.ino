#include <Wire.h>


//falat definir valor dos inputs

//*********************************************************************************define's************************************************************************************************************************************************************************************

#define Clock 2    //pins micro I2C comunicacao
#define dados 3

#define enderecoEMCP 0x24 //E->esquerda
#define enderecoMMCP 0x26 //M->meio
#define enderecoDMCP 0x27 //D->direita

#define IODIRA  0x00	// Port A direction register. Write a 0 to make a pin an output, a 1 to make it an input
#define IODIRB  0x01    // Port B direction register. Write a 0 to make a pin an output, a 1 to make it an input
#define GPIOA   0x12    // Register Address of Port A - read data from or write output data to this port
#define GPIOB   0x13    // Register Address of Port B - read data from or write output data to this port

#define xTorre 6
#define yTorre 6
#define zTorre 6
#define numPecas 10
#define numMCP 3

//**********************************************************************************variaveis globais***************************************************************************************************************************************************************************

char contador_pisos=0;//sinaliza em que piso esta [1, 6]
int contador_ciclos=0;//250 para descer peça 1 andar
char pontuacao_atual=0;//contar pontuacao obtida cada ronda
char blocoEscolhido=0; //para identificar o bloco sorteado a ser desenhado

boolean fundo_torre[xTorre][yTorre][zTorre]={0}; //vetor que guarda informação sobre que leds acessos/apagados fundo
boolean ativo_torre[xTorre][yTorre][zTorre]={0}; ////vetor que guarda informação sobre que leds acessos ativos(peca em queda)
boolean i2c_em_uso=false; //para indicar que alguma tarefa esta a usar recurso patilhado, comunicacoes i2c
boolean comecar=true;//caso fazer novo jogo
boolean flag=false;//para executar a tarefa dentro do loop
boolean in_game=false;//esta um jogo a decorrer
boolean saida=false;//sinaliza o sair do jogo atual
boolean flag_left=0;
char andar_bloco_ativo=5;// função que gera um bloco reinicia para 5 (ultimo andar)

byte EMCP_GPIOB_anterior=0xFF;
byte EMCP_GPIOA_anterior=0xFF;
byte MMCP_GPIOB_anterior=0xFF;
byte MMCP_GPIOA_anterior=0xFF;
byte DMCP_GPIOB_anterior=0xFF;
byte DMCP_GPIOA_anterior=0xFF;


//estrutura para pecas
typedef struct
        {
              //boolean pontoCentral[3][3]; // igual á posicao no vetor
              boolean local[3][3]; // 9 posicoes
              int rotacao_atual; //nao esquecer ao criar instancias inicializar isto a 0
        } bloco;

//vetor para N pecas diferentes
bloco vetorBlocos[numPecas]; //vetor com as 10 pecas diferentes

//**********************************************************************************************************************funcao escrever no MCP...*****************************************************************************************************************************
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

//***********************************************************************************funcao init*******************************************************************************************************************************************************************************
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
 

 fundo_torre[0][0][0]=true;
  fundo_torre[5][5][0]=true;
 ativo_torre[0][1][5]=true;
  ativo_torre[1][1][5]=true;
   ativo_torre[2][1][5]=true;
 
      flag_left=true;
 

}

//**********************************************************************************************************************funcao c/ interrupção para refresh POV ***********************************************************************************************************************************
ISR(TIMER1_COMPA_vect){          // interrupção por igualdade de comparação no TIMER1

    flag=true;
    digitalWrite(Clock, LOW);
    digitalWrite(dados, LOW);
    contador_pisos=contador_pisos+1;
    contador_ciclos++;
    if(contador_ciclos>251){
      contador_ciclos=0;
    }
    if(contador_pisos>11){ //caso saia dos shiftRegister inserir dado para ter clock
      contador_pisos=0;
      digitalWrite(dados, HIGH);
    }
    
}

void resetArrays(){

  char i=0;
  char j=0;
  char k=0;

  for(i=0;i<xTorre;i++){
    for(j=0;j<yTorre;j++){
      for(k=0;k<zTorre;k++){
        fundo_torre[i][j][k]=0;
        ativo_torre[i][j][k]=0;
      }
    }
  }

  //dadosEnviar();
}

void newGame(){

  //apaga todos os led's
  i2c_em_uso=true;
  MCPwrite(enderecoEMCP, GPIOA, 0xFF);
  MCPwrite(enderecoEMCP, GPIOB, 0xFF);
  MCPwrite(enderecoMMCP, GPIOA, 0xFF);
  MCPwrite(enderecoMMCP, GPIOB, 0xFF);
  MCPwrite(enderecoDMCP, GPIOA, 0xFF);
  MCPwrite(enderecoDMCP, GPIOB, 0xFF);
  i2c_em_uso=false;

  //mete a pontuacao a zero
  pontuacao_atual=0;

  //limpa os vetores
  resetArrays();
}

void MoveDown(){//verfica se pode descer um piso (se nao puder passa ativo para o fundo)
    
    char i,j,k,a,b;
    
      if(andar_bloco_ativo==-1)
        return;
        
      if(andar_bloco_ativo==0){//ativo chega ao ultimo piso
         for(a=0;a<xTorre;a++){
           for(b=0;b<yTorre;b++){
               fundo_torre[a][b][andar_bloco_ativo] |=ativo_torre[a][b][andar_bloco_ativo];//bloco ativo passa a fundo
               ativo_torre[a][b][andar_bloco_ativo]=0;
               andar_bloco_ativo=-1;
                
              }
            } //depois de meter no fundo retorna
            //PEDIR NOVO BLOCO
            return;
          }
          
      for(j=0;j<6;j++){
        for(i=0;i<6;i++){
          if(fundo_torre[i][j][andar_bloco_ativo-1] && ativo_torre[i][j][andar_bloco_ativo]){//se background estiver ativo verifica se tem algum bloco ativo no nivel acima
              for(a=0;a<xTorre;a++){
                for(b=0;b<yTorre;b++){
                    fundo_torre[a][b][andar_bloco_ativo] |= ativo_torre[a][b][andar_bloco_ativo];
                    ativo_torre[a][b][andar_bloco_ativo]=0;
                    andar_bloco_ativo=-1; //Quando gerar novo bloco ativo reinicia a 5 
                }
              } //depois de meter no fundo retorna
              //PEDIR NOVO BLOCO
              return;
            }
          }
        }
    
    //se não passar a fundo desce um piso
      for(j=0;j<6;j++){
        for(i=0;i<6;i++){
          ativo_torre[i][j][andar_bloco_ativo-1]=ativo_torre[i][j][andar_bloco_ativo];
          ativo_torre[i][j][andar_bloco_ativo]=0;
        }
      }
    
    andar_bloco_ativo--;
    return;
}

void MoveLeft(){//verfica se pode descer um piso (se nao puder passa ativo para o fundo)
    
    char i,j,k,a,b;
    boolean teste;
      if(flag_left==false)
        return;
      flag_left=false;  
      //ativo chega ao ultimo piso
         for(b=0;b<yTorre;b++){
           for(a=0;a<xTorre;a++){
               if(ativo_torre[a][b][andar_bloco_ativo] && b==0)
                 return;
               teste=ativo_torre[a][b][andar_bloco_ativo]  && fundo_torre[a][b-1][andar_bloco_ativo]; //se tiver 2 bits lado a lado acesos não deixa mexer
               if(teste==1)
                 return;//nao deixa mexer no left
           }
         } //depois de meter no fundo retorna
         for(b=0;b<yTorre;b++){
           for(a=0;a<xTorre;a++){
             ativo_torre[a][b][andar_bloco_ativo]=ativo_torre[a][b-1][andar_bloco_ativo];
           }
         }
            //PEDIR NOVO BLOCO
         return;
     
}

void refreshTorre(char andar){

  char i;
  char j;
  char s=0;
  byte auxiliar[numMCP*2]; //byte a enviar para acender leds

//  if(contador_ciclos>=250)// se passar 250 interrupçoes verifica se mudou alguma coisa nos vetores que representam a torre
//  {
//     MoveDown();
//  }    
      andar=andar/2;
      
      for(j=0;j<yTorre;j++){//percorre vetores para ver se muda alguma coisa
      auxiliar[j]=0xFF; //coluna toda apagada
       for(i=0;i<xTorre;i++){
          if(fundo_torre[i][j][andar] || ativo_torre[i][j][andar]){ //OR, pois basta apenas um em ambas ligado
            if(j%2==0){
              if(i==0)
               auxiliar[j] &= 0xFE;
              else if(i==1)
               auxiliar[j] &= 0xFD;
              else if(i==2)
               auxiliar[j] &= 0xFB;
              else if(i==3)
                auxiliar[j] &= 0xF7;
              else if(i==4)
                auxiliar[j] &= 0xEF;
              else if(i==5)
                auxiliar[j] &= 0xDF;
             }
            if(j%2!=0){
              if(i==0)
                auxiliar[j] &= 0xFB;
              else if(i==1)
                auxiliar[j] &= 0xF7;
              else if(i==2)
                auxiliar[j] &= 0xEF;
              else if(i==3)
                auxiliar[j] &= 0xDF;
              else if(i==4)
                auxiliar[j] &= 0xBF;
              else if(i==5)
                auxiliar[j] &= 0x7F;
             }
          }
       }
    }
    
    
     //envia dados e atualizar valor anteriormente escrito(pois so escreve se for diferente do anterior, poupar tempo
     if(auxiliar[1]!=EMCP_GPIOA_anterior){
          MCPwrite(enderecoEMCP, GPIOA, auxiliar[1]);
          EMCP_GPIOA_anterior=auxiliar[1];
     }
     
     if(auxiliar[0]!=EMCP_GPIOB_anterior){
          MCPwrite(enderecoEMCP, GPIOB, auxiliar[0]);
          EMCP_GPIOB_anterior=auxiliar[0];
     }
          
     if(auxiliar[3]!=MMCP_GPIOA_anterior){
          MCPwrite(enderecoMMCP, GPIOA, auxiliar[3]);
          MMCP_GPIOA_anterior=auxiliar[3];
     }
     if(auxiliar[2]!=MMCP_GPIOB_anterior){
          MCPwrite(enderecoMMCP, GPIOB, auxiliar[2]);
          MMCP_GPIOB_anterior=auxiliar[2];
     }
     if(auxiliar[5]!=DMCP_GPIOA_anterior){
          MCPwrite(enderecoDMCP, GPIOA, auxiliar[5]);
          DMCP_GPIOA_anterior=auxiliar[5];
     }
     if(auxiliar[4]!=DMCP_GPIOB_anterior){
          MCPwrite(enderecoDMCP, GPIOB, auxiliar[4]);
          DMCP_GPIOB_anterior=auxiliar[4];
     } 
    
}  




/*
 if(comecar){
    newGame();
    comecar=false;
    in_game=true;
 }

 if(in_game){
   while(1){
        if(flag==true){
          refreshTorre(contador_pisos);
          flag=false;
          digitalWrite(Clock, HIGH);
        }
        if(saida)
          break;
   }
 }
*/
 void loop(){
      if(contador_ciclos==250)
      {
        MoveDown();
      }
      if(contador_ciclos==100)
      {
        //MoveLeft();
      }
      if(flag==true){
       if(contador_pisos%2==0)
         refreshTorre(contador_pisos);
         
       flag=false;
       digitalWrite(Clock, HIGH);  
       }  
    
}
