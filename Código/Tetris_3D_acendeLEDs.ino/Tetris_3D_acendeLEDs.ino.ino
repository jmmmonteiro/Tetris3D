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
              boolean pontoCentral[3][3]; // igual á posicao no vetor
              boolean local[3][3]; // 9 posicoes
              char rotacao_atual; //nao esquecer ao criar instancias inicializar isto a 0x00
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

  randomSeed(analogRead(1));           //semear semente para depois escolher as pecas

  Serial.begin(9600);  //configure the serial port so we can use the Serial Monitor
  pinMode(Clock, OUTPUT); //mudar para os define, mas confirmar se estão certos pode o 2 e 3 estar ao contrario
  pinMode(dados, OUTPUT);

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
          flag=1; 
          break;
        }  
      if(flag){
        flag=0;
        auxiliar[i] |= (0x1) << 7-j; //admitindo led(0,0) e MSB ????????(confirmar)
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
 
//**********************************************************************************************************************diz posicao onde bloco ativo esta*********************************************************************************
void pontosAtuais(char **x, char **y, char **z){
  
  char i, j, k;
  char aux[3][3]={0};//vetorBlocos[tipoBloco].local;
  
  
  if((vetorBlocos[blocoEscolhido].rotacao_atual==0x01)){//rotacac=90º
      //atualizar aux tendo em conta rotacao
  }    
  else if((vetorBlocos[blocoEscolhido].rotacao_atual==0x02)){//rotacac=180º
       //atualizar aux tendo em conta rotacao
  }     
  else if((vetorBlocos[blocoEscolhido].rotacao_atual==0x03)){//rotacac=270º ou -90º
        //atualizar aux tendo em conta rotacao
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
 
//**********************************************************************************************************************pode mover baixo*********************************************************************************

boolean canMoveDown(){ 
  
  char i, j; 
  char *pontoOrigemX, *pontoOrigemY, *pontoOrigemZ;
  char aux[3][3]={0};
  
  //ponto central da peca
  pontosAtuais(&pontoOrigemX, &pontoOrigemY, &pontoOrigemZ); 
  
  //ja no ultimo nivel, logo não pode descer mais
  if (*pontoOrigemZ==5) 
    return false;      
    
  if((vetorBlocos[blocoEscolhido].rotacao_atual==0x01)){//rotacac=90º
      //atualizar aux tendo em conta rotacao
  }    
  else if((vetorBlocos[blocoEscolhido].rotacao_atual==0x02)){//rotacac=180º
       //atualizar aux tendo em conta rotacao
  }     
  else if((vetorBlocos[blocoEscolhido].rotacao_atual==0x03)){//rotacac=270º ou -90º
        //atualizar aux tendo em conta rotacao
  }
  
  for(j=0;j<3;j++){
    for(i=0;i<3;i++){
     if(aux[i][j]){ // se led acesso nesta posicao
       if(fundo_torre[*pontoOrigemX+i-1][*pontoOrigemY+j-1][*pontoOrigemZ-1]){ //basta que um não possa mover para dar erro
        return false; 
       }  
     }
    }
  }  
  
  //se sair do ciclo quer dizer que todos se podem mover
  return true; 
 
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


//**********************************************************************************************************************descer um andar*********************************************************************************

void desceNivel(){             // TEM ERROS CORRIGIR, VER FACE
  
  char *pontoAtualX, *pontoAtualY, *pontoAtualZ;
  
  pontosAtuais(&pontoAtualX, &pontoAtualY, &pontoAtualZ); 
  
  //verifica se bloco pode descer(ou seja, se andar abaixo tem espaço)
  if(canMoveDown()){
    ativo_torre[*pontoAtualX][*pontoAtualY][*pontoAtualZ]=0;
    ativo_torre[*pontoAtualX][*pontoAtualY][*pontoAtualZ-1]=1; 
    return;    
  }
  //se atinge maximo nivel pode descer, passa de ativo a pertencer ao fundo(background)
  else if( (fundo_torre[*pontoAtualX][*pontoAtualY][*pontoAtualZ-1]==1) || (*pontoAtualZ==5) ){          
              ativo_torre[*pontoAtualX][*pontoAtualY][*pontoAtualZ]=0;
              fundo_torre[*pontoAtualX][*pontoAtualY][*pontoAtualZ]=1;
              return;
            }  
}

//**********************************************************************************************************************calcular resultado direcao*********************************************************************************

void rotacao(boolean direcao, char id){
    
    char i, j, k ,w;
    char pontoCentralX, pontoCentralY;
    char  *pontoOrigemX, *pontoOrigemY, *pontoOrigemZ;
    char pontoDestinoX, pontoDestinoY; 
    
    //descobrir pontos centrais
    for(w=0;w<6;i++){
        if(vetorBlocos[id].local[w]){
          if(w<3)
            pontoCentralX=w+1;
          else if(w>3)
            pontoCentralY=w+2;  
        }
    } 
    
    //descobrir pontos origem
    pontosAtuais(&pontoOrigemX, &pontoOrigemY, &pontoOrigemZ); 
     
   //-90 em z
   if(!direcao){
      pontoDestinoX=(*pontoOrigemY)+pontoCentralX-pontoCentralY;
      pontoDestinoY=pontoCentralX+pontoCentralY-(*pontoOrigemX);
      if(!canRotateRight(pontoDestinoX, pontoDestinoY, *pontoOrigemZ)){
        gameover();
       }
   }
   
   //+90 em z
   else if(direcao){
     pontoDestinoX=(*pontoOrigemY)+pontoCentralX-pontoCentralY;
     pontoDestinoY=-pontoCentralX+pontoCentralY+(*pontoOrigemX);
     if(!canRotateLeft(pontoDestinoX, pontoDestinoY, *pontoOrigemZ)){
        gameover();
       }
   }    
  
    ativo_torre[pontoDestinoX][pontoDestinoY][k]=0;
    dadosEnviar();
}  


//**********************************************************************************************************************pode mover esquerda*********************************************************************************
boolean canMoveLeft(){ 
  
  char *pontoOrigemX, *pontoOrigemY, *pontoOrigemZ;
   
  pontosAtuais(&pontoOrigemX, &pontoOrigemY, &pontoOrigemZ); 
  if(!fundo_torre[*pontoOrigemX][*pontoOrigemY-1][*pontoOrigemZ])
     return true; 
  else 
     return false;
  
}  

//**********************************************************************************************************************pode mover direita*********************************************************************************
boolean canMoveRight(){ 
  
    
  char *pontoOrigemX, *pontoOrigemY, *pontoOrigemZ;
   
  pontosAtuais(&pontoOrigemX, &pontoOrigemY, &pontoOrigemZ); 
  if(!fundo_torre[*pontoOrigemX][*pontoOrigemY+1][*pontoOrigemZ])
     return true; 
  else 
     return false;
  
}  

//**********************************************************************************************************************pode rodar esquerda*********************************************************************************
boolean canRotateLeft(char xDestino, char yDestino, char pontoOrigemZ){ 
   
  if(!fundo_torre[xDestino][yDestino][pontoOrigemZ])
     return true; 
  else 
     return false;
  
}  

//**********************************************************************************************************************pode rodar direita*********************************************************************************
boolean canRotateRight(char xDestino, char yDestino, char pontoOrigemZ){ 
  
  if(!fundo_torre[xDestino][yDestino][pontoOrigemZ])
     return true; 
  else 
     return false;
     
}  


//**********************************************************************************************************************apaga uma linha cheia*********************************************************************************
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
