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

    if(contador_pisos>11){ //caso saia dos shiftRegister inserir dado para ter clock
      contador_pisos=0;
      digitalWrite(dados, HIGH);
    }
    
}
 
//**********************************************************************************************************************fazer reset aos vetores com valores led's acessos(para comecar novo jogo)*******************************************************************************************
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
//**********************************************************************************************************************prepara para comecar novo jogo***********************************************************************************************************************************

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

//**********************************************************************************************************************gera novo bloco e faz mapeamento no vetor ativo_torre[][]*************************************************************************************************************

void newBlock(){
  
  char i, j;
  
  //mapeado numa matrix central de 3x3(pois assim unico ponto medio) 
  blocoEscolhido=random(numPecas); //alterar depois para o numero de blocos que existirem
  
   for(i=0;i<3;i++){
    for(j=0;j<3;j++){
      ativo_torre[i+1][j+2][5]=vetorBlocos[blocoEscolhido].local[i][j];//quais leds de 3x3 pertencem a peca
    }
   }
   
  //dadosEnviar(); nao preciso, pois já chama no loop 
  
}  
 
//**********************************************************************************************************************em cada andar faz merge blocos fundo e blocos ativos e envia dados para acender led's*********************************************************************************

void refreshTorre(char andar){
  
  char i;
  char j;
  char k;
  char s=0;
  boolean flag=0;
  byte auxiliar[numMCP*2]; //byte a enviar para acender leds
  
   for(j=0;j<yTorre;j++){
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
 }  
   
 //envia dados e atualizar valor anteriormente escrito(pois so escreve se for diferente do anterior, poupar tempo
   if(auxiliar[0]!=EMCP_GPIOA_anterior){
        MCPwrite(enderecoEMCP, GPIOA, auxiliar[0]);
        EMCP_GPIOA_anterior=auxiliar[0];
      }
    if(auxiliar[1]!=EMCP_GPIOB_anterior){
        MCPwrite(enderecoEMCP, GPIOB, auxiliar[1]);
        EMCP_GPIOB_anterior=auxiliar[1];
      }  
    if(auxiliar[2]!=MMCP_GPIOA_anterior){
        MCPwrite(enderecoMMCP, GPIOA, auxiliar[2]);
        MMCP_GPIOA_anterior=auxiliar[2];
      } 
    if(auxiliar[3]!=MMCP_GPIOB_anterior){
        MCPwrite(enderecoMMCP, GPIOB, auxiliar[3]);
        MMCP_GPIOB_anterior=auxiliar[3];
      }
    if(auxiliar[4]!=DMCP_GPIOA_anterior){
        MCPwrite(enderecoDMCP, GPIOA, auxiliar[4]);
        DMCP_GPIOA_anterior=auxiliar[4];
      }
    if(auxiliar[5]!=DMCP_GPIOB_anterior){
        MCPwrite(enderecoDMCP, GPIOB, auxiliar[5]);
        DMCP_GPIOB_anterior=auxiliar[5];
      }    
}   

//**********************************************************************************************************************da-me vetor 3x3 para procurar peca ativa no jogo de acordo com orientacao atual************************************************************************************************************************************ 
void compensa_rotacao(boolean aux[][3], boolean sentidoHorario){//sentido horario = -90º
  
  //ponto central da peca, sempre este pq matriz 3x3
  int pontoCentralX=1;
  int pontoCentralY=1;
  
  int xDestino, yDestino;
  int i, j;
  
  if(sentidoHorario){
     for(j=0;j<3;j++){
      for(i=0;i<3;i++){
       if(vetorBlocos[blocoEscolhido].local[i][j]){
        aux[i][j]=false;
        xDestino=j+pontoCentralX-pontoCentralY;
        yDestino=pontoCentralX+pontoCentralY-i;
        aux[xDestino][yDestino]=true;     
       }  
      }
     }
  }
  else{
    for(j=0;j<3;j++){
      for(i=0;i<3;i++){
       if(vetorBlocos[blocoEscolhido].local[i][j]){
        aux[i][j]=false;
        xDestino=j+pontoCentralX-pontoCentralY;
        yDestino=-pontoCentralX+pontoCentralY+i;
        aux[xDestino][yDestino]=true;  
       }  
      }
     } 
  }
  
  return;  
}

//**********************************************************************************************************************diz posicao onde bloco ativo esta**********************************************************************************************************************
void pontosAtuais(int **x, int **y, int **z){
  
  char i, j, k;
  boolean aux[3][3]={false};//vetorBlocos[tipoBloco].local;
  
  
  if((vetorBlocos[blocoEscolhido].rotacao_atual>0)){
      //atualizar aux tendo em conta rotacao
      for(i=0;i<(vetorBlocos[blocoEscolhido].rotacao_atual/90);i++){
          compensa_rotacao(aux,false);
      }  
  }        
  else if((vetorBlocos[blocoEscolhido].rotacao_atual<0)){
        //atualizar aux tendo em conta rotacao
        for(i=0;i<(vetorBlocos[blocoEscolhido].rotacao_atual/(-90));i++){
          compensa_rotacao(aux,true);
      }  
  }
  
   //descobrir pontos origem
     for(k=0;k<zTorre;k++){
      for(j=0;k<yTorre;j++){
        for(i=0;i<3;i++){ //pois pesquisa logo nas tres linhas ao mesmo tempo
          if( (ativo_torre[i][j][k]==aux[i][j]) && (ativo_torre[i+1][j][k]==aux[i+1][j]) && (ativo_torre[i+2][j][k]==aux[i+2][j]) ){
             //retorna as coordenadas do ponto central da peca
             (*x)[0]= i+1;
             (*y)[0]= j+1;
             (*z)[0]= k; 
             return;
          }
        }
      }
     }
}
 
//**********************************************************************************************************************pode mover baixo***************************************************************************************************************************************

boolean canMoveDown(){ 
  
  char i, j; 
  int *pontoCentralX, *pontoCentralY, *pontoCentralZ;
  boolean aux[3][3]={0};
  
  //ponto central da peca
  pontosAtuais(&pontoCentralX, &pontoCentralY, &pontoCentralZ); 
  
  //ja no ultimo nivel, logo não pode descer mais
  if (*pontoCentralZ==5) 
    return false;      
    
  for(j=0;j<3;j++){
    for(i=0;i<3;i++){
     if(aux[i][j]){ // se led acesso nesta posicao
       if(fundo_torre[*pontoCentralX+i-1][*pontoCentralY+j-1][*pontoCentralZ-1]){ //basta que um não possa mover para dar erro
        return false; 
       }  
     }
    }
  }  
  
  //se sair do ciclo quer dizer que todos se podem mover
  return true; 
 
}  

//**********************************************************************************************************************descer um andar*******************************************************************************************************************************************************

void desceNivel(){             
  
  char i,j;
  int *pontoCentralX, *pontoCentralY, *pontoCentralZ;
  
  pontosAtuais(&pontoCentralX, &pontoCentralY, &pontoCentralZ); 
  
  //verifica se bloco pode descer(ou seja, se andar abaixo tem espaço)
  if(canMoveDown()){
    for(j=0;j<3;j++){
      for(i=0;i<3;i++){
       if(ativo_torre[*pontoCentralX+i-1][*pontoCentralY+j-1][*pontoCentralZ]){
          ativo_torre[*pontoCentralX+i-1][*pontoCentralY+j-1][*pontoCentralZ]=0;
          ativo_torre[*pontoCentralX+i-1][*pontoCentralY+j-1][*pontoCentralZ-1]=1;
        }
      }
    } 
      return;    
  }
  //se atinge maximo nivel pode descer, passa de ativo a pertencer ao fundo(background)
  else if( (fundo_torre[*pontoCentralX][*pontoCentralY][*pontoCentralZ-1]==1) || (*pontoCentralZ==5) ){    
              
     for(j=0;j<3;j++){
      for(i=0;i<3;i++){
       if(ativo_torre[*pontoCentralX+i-1][*pontoCentralY+j-1][*pontoCentralZ]){
          ativo_torre[*pontoCentralX+i-1][*pontoCentralY+j-1][*pontoCentralZ]=0;
          fundo_torre[*pontoCentralX+i-1][*pontoCentralY+j-1][*pontoCentralZ]=1;
        }
      }
     }
     return;
  }  
  
} 

//**********************************************************************************************************************calcular resultado direcao*****************************************************************************************************************************

boolean rotacao(boolean sentidoHorario){
    
    int i, j;
    int *pontoCentralX, *pontoCentralY, *pontoCentralZ;
    int xDestino, yDestino; 
    
      
    //descobrir pontos origem
    pontosAtuais(&pontoCentralX, &pontoCentralY, &pontoCentralZ); 
     
   //-90 em z
   if(sentidoHorario){
     for(j=0;j<3;j++){
      for(i=0;i<3;i++){
       if(vetorBlocos[blocoEscolhido].local[i][j]){
         xDestino=j+*pontoCentralX-*pontoCentralY;
         yDestino=*pontoCentralX+*pontoCentralY-i;
         if(canRotateRight(xDestino, yDestino, *pontoCentralZ)){
            ativo_torre[*pontoCentralX+i-1][*pontoCentralY+j-1][*pontoCentralZ]=false;
            ativo_torre[xDestino][yDestino][*pontoCentralZ]=true;
         }
         else 
            return false;   
       }  
      }  
     }
    vetorBlocos[blocoEscolhido].rotacao_atual=vetorBlocos[blocoEscolhido].rotacao_atual+90;
    if((vetorBlocos[blocoEscolhido].rotacao_atual)==360)
       vetorBlocos[blocoEscolhido].rotacao_atual=0;       
   }  
   
    //90 em z
  else if(!sentidoHorario){
     for(j=0;j<3;j++){
      for(i=0;i<3;i++){
       if(vetorBlocos[blocoEscolhido].local[i][j]){
          xDestino=j+pontoCentralX-pontoCentralY;
          yDestino=+pontoCentralY+i-pontoCentralX;
          if(canRotateLeft(xDestino, yDestino, *pontoCentralZ)){
            ativo_torre[*pontoCentralX+i-1][*pontoCentralY+j-1][*pontoCentralZ]=false;
            ativo_torre[xDestino][yDestino][*pontoCentralZ]=true;
         }
         else 
            return false;   
       }  
      }  
     }
    vetorBlocos[blocoEscolhido].rotacao_atual=vetorBlocos[blocoEscolhido].rotacao_atual-90;
    if((vetorBlocos[blocoEscolhido].rotacao_atual)==-360)
       vetorBlocos[blocoEscolhido].rotacao_atual=0;       
   }       
  
   
}  

//**********************************************************************************************************************pode mover esquerda********************************************************************************************************************************
boolean canMoveLeft(int pontoCentralX, int pontoCentralY, int pontoCentralZ){ 
  
   char i, j;
   
   if(pontoCentralY==0)
     return false;
   else  
     pontoCentralY--;
     
   for(j=0;j<3;j++){
     for(i=0;i<3;i++){
        if(fundo_torre[pontoCentralX+i-1][pontoCentralY+j-1][pontoCentralZ])
           return false; 
     }
   }   
  
     return false;
  
}  

//**********************************************************************************************************************pode mover direita**********************************************************************************************************************************
boolean canMoveRight(int pontoCentralX, int pontoCentralY, int pontoCentralZ){ 
  
   
  char i, j;
   
   if(pontoCentralY==5)
     return false;
   else  
     pontoCentralY++;
     
   for(j=0;j<3;j++){
     for(i=0;i<3;i++){
        if(fundo_torre[pontoCentralX+i-1][pontoCentralY+j-1][pontoCentralZ])
           return false; 
     }
   }   
  
     return false;
  
}  

//**********************************************************************************************************************pode rodar esquerda***********************************************************************************************************************************
boolean canRotateLeft(char xDestino, char yDestino, char pontoOrigemZ){ 
   
  if(!fundo_torre[xDestino][yDestino][pontoOrigemZ])
     return true; 
  else 
     return false;
  
}  

//**********************************************************************************************************************pode rodar direita***********************************************************************************************************************************
boolean canRotateRight(char xDestino, char yDestino, char pontoOrigemZ){ 
  
  if(!fundo_torre[xDestino][yDestino][pontoOrigemZ])
     return true; 
  else 
     return false;
     
}  


//**********************************************************************************************************************loop principal**************************************************************************************************************************************  
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
