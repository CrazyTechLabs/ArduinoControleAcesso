
#include <EEPROM.h>  
#include <SPI.h>      
#include <MFRC522.h>   
#include <LiquidCrystal.h>
    
    


//Algumas Definicoes... 
//com isso nao temos que lembrar dos pinos por exemplo

#define ledvermelho 15
#define ledverde 16
#define ledazul 17
#define rele 18
#define Ligado LOW
#define Desligado HIGH
#define SS_PIN 10
#define RST_PIN 9



//Variaveis

boolean encontrado = false; 
boolean modoadmin = false; 
int leituraok; 
byte eepromCartao[4];  
byte CartaoLido[4];           
byte tagAdmin[4]; 

//Leitor de RFID, Configurando
//Inicializa o Leitor de RFID 522
MFRC522 mfrc522(SS_PIN, RST_PIN);	
//Inicia o LED 16X2
LiquidCrystal lcd(8, 7, 3, 4, 5, 6);

///////////////////////////////////////// Setup ///////////////////////////////////
void setup() {
  
//Arduino Pin Configuration
  pinMode(ledvermelho, OUTPUT);
  pinMode(ledverde, OUTPUT);
  pinMode(ledazul, OUTPUT);
  pinMode(rele, OUTPUT);
  digitalWrite(rele, HIGH); 
  apagaLeds();
  
  
  //Protocol Configuration
  Serial.begin(9600);	 // Initialize serial communications with PC
  SPI.begin();           // MFRC522 Hardware uses SPI protocol
  mfrc522.PCD_Init();    // Initialize MFRC522 Hardware
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max); //Set Antenna Gain to Max- this will increase reading distance

//Inicializa o LCD
lcd.begin(16, 2);
//Mensagem Inicial.
lcd.print("Passe sua TAG.....");

  //Verifica se existe uma tag master cadastrada 
  if (EEPROM.read(1) != 1) {  // Olha na  EEPROM se o Master esta cadastrado, o endereco um e para isso. 
    Serial.println("Defina uma TAG Master");
    
    do {
      leituraok = getID(); // atribui valor de 0 ou 1 para a leitura, sendo ver ou falso
      piscaLed(ledazul,2,200);
    }
    while (!leituraok); //como n˜o tem master definido, fica varrendo indefinidamente ate ter um cartao lido.
    for ( int j = 0; j < 4; j++ ) { // loop de 4 vezes
      EEPROM.write( 2 +j, CartaoLido[j] ); // escreve o cartao lido na EEPROM comeca no endereco 3
    }
    EEPROM.write(1,1); //escreve na EEPROM o cartao definido
    Serial.println("Cartao Master definido.");
    
  }
  
  for ( int i = 0; i < 4; i++ ) {     // Le o master da EEPROM
    tagAdmin[i] = EEPROM.read(2+i); // Escreve na variavel
    Serial.print(tagAdmin[i], HEX);
  }
  Serial.println("Passe sua TAG....");
 adminModeOn();    //muda a cor do led.
}

//Funcao para mostrar no LCD 16X2
void MostrarLCD(int linha, char texto[], boolean apagar)
{
  
  if(apagar == true)
  {
    lcd.clear();
  }
  
  lcd.setCursor(0,linha); 
  lcd.print(texto);
  
  
}

///////////////////////////////////////// Main Loop ///////////////////////////////////
void loop () {
  do {
    leituraok = getID(); 
    if (modoadmin) { //Se o cart˜o for de Administrador, entre no modo ADMIN..
      adminModeOn(); 
    }
    else {
      normalModeOn(); // senao modo normal de leitura
    }
  }
  while (!leituraok); //o programa fica varrendo indefinidamente a espera de uma leitura
  if (modoadmin) {
    if ( isMaster(CartaoLido) ) {  //Passou o cart˜o de novo, sai do modo admin
      Serial.println("Saindo do modo de programacao");
      MostrarLCD(1,"Saiu da Prog.",false);
      modoadmin = false;
      return;
    }
    else { //se no modo admin voce passa um cartao que ja esta gravado ele e excluido, senao inserido	
      if ( findID(CartaoLido) ) { 
        deleteID(CartaoLido);
        Serial.println("Cartao removido"); 
        MostrarLCD(1,"Removido.",false);
      }
      else {                    
        writeID(CartaoLido);
        Serial.println("Tag Adicionada.");
        MostrarLCD(1,"Tag Adicionada.",false);
        
      }
    }
  }
  else {
    if ( isMaster(CartaoLido) ) {  //Se o cartao for master....
      modoadmin = true;
      Serial.println("Modo Administrador");
      int count = EEPROM.read(0); // conta o numero de cartoes lendo o primeiro byte da EEPROM, onde fica a informacao
      Serial.print("Existem ");    
      Serial.print(count);
      Serial.print(" cart˜es na EEPROM");
      Serial.println("");
      Serial.println("Passe um cartao para inserir ou remover");
      Serial.println("-----------------------------");
      MostrarLCD(1,"Adic ou Rem...",false);
    }
    else {
      if ( findID(CartaoLido) ) {   //  Caso o cart˜o lido seja valido, mens. de boas vindas 
        Serial.println("Bem Vindo!");
        MostrarLCD(1,"Bem Vindo!",false);
        EntradaLiberada(300);      // desativa o rele por 300 milis.
      }
      else {				// se o cartao nao esta na base... 
        Serial.println("Vaza Malandro!");
        MostrarLCD(1,"Vaza Malandro!",false);
        failed(); 
      }
    }
  }
}

///////////////////////////////////////// Pegar a ID do Cartao ///////////////////////////////////
int getID() {

  if ( ! mfrc522.PICC_IsNewCardPresent()) { // se um cartao estiver presente continua  lembre-se que ! nega a expressao :)  
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) { //se se comunicar continua
    return 0;
  }
  // Tem cartoes e TAGS com 4 ou 7 bytes... Aqui usamos somente 4.
  Serial.println("Scanned PICC's UID:");
  for (int i = 0; i < 4; i++) {  // 
    CartaoLido[i] = mfrc522.uid.uidByte[i];
    Serial.print(CartaoLido[i], HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); // para a leitura
  return 1;
}

/////////////////////////////////////////  (Admin Mode) ///////////////////////////////////
void adminModeOn() {
  apagaLeds();
  digitalWrite(ledazul, Ligado); // Make sure blue LED is off
  delay(200);
}

//////////////////////////////////////// Normal Mode Led  ///////////////////////////////////
void normalModeOn () { // no modo normal queremos luz amarela
  apagaLeds();
  digitalWrite(ledvermelho, Ligado); 
  digitalWrite(ledverde, Ligado); 
  digitalWrite(rele, LOW); 
}

//////////////////////////////////////// ler um cartao da EEPROM //////////////////////////////
void readID( int number ) {
  int start = (number * 4 ) + 2; 
  for ( int i = 0; i < 4; i++ ) { 
    eepromCartao[i] = EEPROM.read(start+i); 
  }
}

///////////////////////////////////////// Adicionar o cartao para a EEPROM   ///////////////////////////////////
void writeID( byte a[] ) {
  if ( !findID( a ) ) { //procura primeiro!
    int num = EEPROM.read(0); // ve a quantidade de registros, posicao 0 salva os registros
    int start = ( num * 4 ) + 6; // vamos ao espaco para gravar
    num++; // Incrementa o contador
    EEPROM.write( 0, num ); // agora colocamos o novo contador no 0
    for ( int j = 0; j < 4; j++ ) { // Loop 
      EEPROM.write( start+j, a[j] ); // escreve o array
    }
    successWrite();
  }
  else {
    failedWrite();
  }
}

///////////////////////////////////////// Removee Cartao da  EEPROM   ///////////////////////////////////
void deleteID( byte a[] ) {
  if ( !findID( a ) ) { // procura
    failedWrite(); // erro se nao achar
  }
  else {
    int num = EEPROM.read(0); // Get ve o numero de espacaos (cartoes)
    int slot;  
    int start;
    int looping;
    int j;
    int count = EEPROM.read(0); 
    slot = findIDSLOT( a ); 
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--; // excluir um do numero de registros
    EEPROM.write( 0, num ); // escreve o novo contador
    for ( j = 0; j < looping; j++ ) {  
      EEPROM.write( start+j, EEPROM.read(start+4+j)); //varre a EEPROM e desloca os seguintes... Evitando lacunas
    }
    for ( int k = 0; k < 4; k++ ) { 
      EEPROM.write( start+j+k, 0);
    }
    successDelete();
  }
}

///////////////////////////////////////// Check Bytes   ///////////////////////////////////
boolean checkTwo ( byte a[], byte b[] ) {
  if ( a[0] != NULL ) // verifica se existe algo no array
    encontrado = true; 
  for ( int k = 0; k < 4; k++ ) { 
    if ( a[k] != b[k] ) 
      encontrado = false;
  }
  if ( encontrado ) { 
    return true; 
  }
  else  {
    return false; 
  }
}

///////////////////////////////////////// Find Slot   ///////////////////////////////////
int findIDSLOT( byte find[] ) {
  int count = EEPROM.read(0); //contagem
  for ( int i = 1; i <= count; i++ ) { 
    readID(i); 
    if( checkTwo( find, eepromCartao ) ) { // Procura o cartao
      return i; // se achar retorna a posicao do Cartao
      break; // Para o for
    }
  }
}

///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
boolean findID( byte find[] ) {
  int count = EEPROM.read(0); // contagem
  for ( int i = 1; i <= count; i++ ) {  // loop na EEPROM
    readID(i); //Le o ID da EEPROM
    if( checkTwo( find, eepromCartao ) ) {  // Verifica o parametro com o EEpromcartao
      return true;
      break; // para a busca
    }
    else {  // se nao encontrar retorna falso   
    }
  }
  return false;
}

///////////////////////////////////////// Write Success to EEPROM   ///////////////////////////////////
//Pisca verde 3 vezes no sucesso de gravacao
void successWrite() {
piscaLed(ledverde, 3,200);
  Serial.println("Gravacao com sucesso.");
}

///////////////////////////////////////// Write Failed to EEPROM   ///////////////////////////////////
// Pisca vermelho 4 vezes para indicar problema ao gravar naEEPROM
void failedWrite() {
  piscaLed(ledvermelho, 4,150);
  Serial.println("Erro ao gravar na EEPROM");
}

///////////////////////////////////////// Success Remove UID From EEPROM  ///////////////////////////////////
// pisca vermelho 1 veze ao deletar um registro delete to EEPROM
void successDelete() {
  piscaLed(ledvermelho, 1,200);
  Serial.println("Succesfully removed ID record from EEPROM");
}


void piscaLed(int led, int vezes, int tempo)
{
  apagaLeds();
  
  for (int i =0; i<= vezes; i++)
 {
  
   if(i>0)
   {
    delay(tempo); 
   }
   digitalWrite(led,Ligado);
  delay(tempo);
 digitalWrite(led,Desligado); 
   
   
 } 
}
  
  void apagaLeds()
  {
   digitalWrite(ledvermelho, Desligado);
   digitalWrite(ledverde, Desligado);
   digitalWrite(ledazul, Desligado);
   
    
  }
  
  


////////////////////// Check CartaoLido IF is tagAdmin   ///////////////////////////////////
//Verifica se a Tag e Admin
boolean isMaster( byte test[] ) {
  if ( checkTwo( test, tagAdmin ) )
    return true;
  else
    return false;
}

///////////////////////////////////////// Entrada Liberada  ///////////////////////////////////
void EntradaLiberada( int setDelay ) {
  apagaLeds();
  digitalWrite(ledverde, Ligado); 
  digitalWrite(rele, LOW); // desarma o rele'
  delay(setDelay); // empo desarmada
  digitalWrite(rele, HIGH); // arma o rele
  delay(2000); // fica verde mais um pouco
}

///////////////////////////////////////// Failed Access  ///////////////////////////////////
void failed() {
  apagaLeds();
  piscaLed(ledvermelho, 5, 100);
 
}
