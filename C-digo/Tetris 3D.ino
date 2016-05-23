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
char contador_ciclos=0;//250 para descer peça 1 andar
char pontuacao_atual=0;//contar pontuacao obtida cada ronda
char blocoEscolhido=0; //para identificar o bloco sorteado a ser desenhado

boolean fundo_torre[xTorre][yTorre][zTorre]={0}; //vetor que guarda informação sobre que leds acessos/apagados fundo
boolean ativo_torre[xTorre][yTorre][zTorre]={0}; ////vetor que guarda informação sobre que leds acessos ativos(peca em queda)
boolean i2c_em_uso=false; //para indicar que alguma tarefa esta a usar recurso patilhado, comunicacoes i2c
boolean comecar=true;//caso fazer novo jogo
boolean flag=false;//para executar a tarefa dentro do loop
boolean in_game=false;//esta um jogo a decorrer
boolean saida=false;//sinaliza o sair do jogo atual

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

}

//**********************************************************************************************************************funcao c/ interrupção para refresh POV ***********************************************************************************************************************************
ISR(TIMER1_COMPA_vect){          // interrupção por igualdade de comparação no TIMER1

    flag=true;
    digitalWrite(Clock, LOW);
    digitalWrite(dados, LOW);
    contador_pisos=contador_pisos+1;
    contador_ciclos++;
    if(contador_ciclos>=250){
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
    for(k=5;k>=0;k--){
      for(j=5;j>=0;j--){
        for(i=5;i>=0;i--){
          if(fundo_torre[i][j][k]){//se background estiver ativo verifica se tem algum bloco ativo no nivel acima
            if(ativo_torre[i][j][k+1]){//se o nivel acima está ativo então nao pode descer
              for(a=0;a<6;a++){
                for(b=0;b<6;b++){
                  if(ativo_torre[a][b][k+1]){
                    fundo_torre[a][b][k+1]=ativo_torre[a][b][k+1];//bloco ativo passa a fundo
                    ativo_torre[a][b][k+1]=0;
                  }
                }
              } //depois de meter no fundo retorna
              //PEDIR NOVO BLOCO
              return;
            }
          }
          else if(k==0){//ativo chega ao ultimo piso
            for(a=0;a<6;a++){
              for(b=0;b<6;b++){
                if(ativo_torre[a][b][k+1]){
                  fundo_torre[a][b][k]=ativo_torre[a][b][k];//bloco ativo passa a fundo
                  ativo_torre[a][b][k]=0;
                }
              }
            } //depois de meter no fundo retorna
            //PEDIR NOVO BLOCO
            return;
          }
        }
      }
    }
    //se não passar a fundo desce um piso
    for(i=1;i<6;i++){
      for(j=1;i<6;i++){
        for(j=1;i<6;i++){
          ativo_torre[i][j][k-1]=ativo_torre[i][j][k];
        }
      }
    }
    return;
}

void refreshTorre(char andar){

  char i;
  char j;
  char k;
  char s=0;
  boolean flag=0;
  byte auxiliar[numMCP*2]; //byte a enviar para acender leds

  if(contador_ciclos>=250)// se passar 250 interrupçoes verifica se mudou alguma coisa nos vetores que representam a torre
  {
     MoveDown();
     for(j=0;j<yTorre;j++){//percorre vetores para ver se muda alguma coisa
      auxiliar[j]=0x00; //coluna toda apagada
      for(i=0;i<xTorre;i++){
          if(fundo_torre[i][j][andar] || ativo_torre[i][j][andar]){ //OR, pois basta apenas um em ambas ligado
            flag=true;
          }
        if(flag){
          flag=false;
          if(j%2==0){
            if(i==0)
             auxiliar[j] |=0x01;
            else if(i==1)
             auxiliar[j] |=0x02;
            else if(i==2)
             auxiliar[j] |=0x04;
            else if(i==3)
              auxiliar[j] |=0x08;
            else if(i==4)
              auxiliar[j] |=0x10;
            else if(i==5)
              auxiliar[j] |=0x20;
           }
          if(j%2!=0){
            if(i==0)
              auxiliar[j] |=0x04;
            else if(i==1)
              auxiliar[j] |=0x08;
            else if(i==2)
              auxiliar[j] |=0x10;
            else if(i==3)
              auxiliar[j] |=0x20;
            else if(i==4)
              auxiliar[j] |=0x40;
            else if(i==5)
              auxiliar[j] |=0x80;
           }
        }
       }
     }

     //para negar bits pois logica negada transistores
     while(s<6){
        auxiliar[s]=!auxiliar[s];
        s++;
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
   else //se não passarem escreve os valores anteriores
   {
     MCPwrite(enderecoEMCP, GPIOB, EMCP_GPIOB_anterior);
     MCPwrite(enderecoEMCP, GPIOA, EMCP_GPIOA_anterior);
     MCPwrite(enderecoMMCP, GPIOB, MMCP_GPIOB_anterior);
     MCPwrite(enderecoMMCP, GPIOA, MMCP_GPIOA_anterior);
     MCPwrite(enderecoDMCP, GPIOB, DMCP_GPIOB_anterior);
     MCPwrite(enderecoDMCP, GPIOA, DMCP_GPIOA_anterior);
   }
}

void loop() {

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

}

//http://tronixstuff.com/2011/08/26/tutorial-maximising-your-arduinos-io-ports/


//**********************************************************************************************************************apaga uma linha cheia*********************************************************************************
/*
boolean deleteRow(){

  char i, j, k;
  char contador=0;

  for(i=0; i<xTorre;i++){
    for(j=0; j<yTorre;j++){
     if(fundo_torre[i][j][i]){
      contador++;
     }
     if(contador==6 and j==5){
       for(k=0;k<xTorre;k++){
         fundo_torre[i][k][i]=0;
       }
       dadosEnviar();
       pontuacao_atual+=10;//bonus pontos encher uma linha
       contador=0;
     }
   }
 }
}

void lerBotoes(){ //falta acabar <--------------------------------------------------------------------------------------------------------------------------------------------------------------

 char dadosLidos[numMCP*2]={1}; //adimitindo logica negada nos botões, para dizer que todos não precionados inicialmente
 char auxiliar=0x00; //para ficar de acordo com a negacao
 char i;

 dadosLidos[0]=MCPread(enderecoEMCP, GPIOA);
 dadosLidos[1]=MCPread(enderecoEMCP, GPIOB);
 dadosLidos[2]=MCPread(enderecoMMCP, GPIOA);
 dadosLidos[3]=MCPread(enderecoMMCP, GPIOB);
 dadosLidos[4]=MCPread(enderecoDMCP, GPIOA);
 dadosLidos[5]=MCPread(enderecoDMCP, GPIOB);

 for(i=0; i<(numMCP*2); i++){
   //vai buscar apenas os 2 ultimos bits cada lado MCP, pois onde estao dados dos botoes
   //assumindo botoes em logica negada, nego dados lidos para ficar em logica "directa"
   auxiliar=( (~dadosLidos[i] & 0x01) << 0 ) | ( (~dadosLidos[i] & 0x02) << 1 ); //vai buscar apenas os 2 ultimos bits cada lado MCP, pois onde estao dados dos botoes

    switch(auxiliar){

      case '0x01' : //botao ligado lado A MCP esquerda porta 8
                      //fazer função associdada esse botao
      break;

      case '0x02':  //botao ligado lado A MCP esquerda porta 7
                      //fazer função associdada esse botao
      break;

      case '0x03':  //botao ligado lado A MCP esquerda porta 7 e porta 8
                      //fazer função associdada aos dois botoes
      break;

    //fazer pata todos os casoos porta A e B todos MCP
    }
  }

}

//falta  acelerarQueda, gameover, ler botoes
boolean acelerarQueda(){

}

boolean gameover(){

 }
*/
