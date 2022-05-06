#include <stdio.h>
#include <time.h>
#include "SparkFun_DE2120_Arduino_Library.h"
#include "SoftwareSerial.h"
#include <SPI.h>
#include <Ethernet2.h>
#include <MFRC522.h>
#include <Arduino.h>
#define SIZE_BUFFER 18
#define MAX_SIZE_BLOCK 16
#define BUFFER_LEN 40
unsigned long tempoAnterior = 0;
void pause (float);
char scanBuffer[BUFFER_LEN];
DE2120 scanner;
SoftwareSerial softSerial(16, 17);
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 1, 177);
// esse objeto 'chave' é utilizado para autenticação
MFRC522::MIFARE_Key key;
// código de status de retorno da autenticação
MFRC522::StatusCode status;

#define RST_PIN 0
#define SS_PIN 15
// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance
unsigned long agora = 0;
void MRFC522_PROX_SETUP();
void MRFC522_PROX_LOOP();
void SETUP_WEB_SERVER();
void LOOP_WEB_SERVER();
void leituraDados();
void gravarDados();
int menu();
void flushRx();

void setup()
{

  Serial.begin(9600);
  SETUP_WEB_SERVER();
  MRFC522_PROX_SETUP();

  if (scanner.begin(softSerial) == false)
  {
    Serial.println("Scanner did not respond. Please check wiring. Did you scan the POR232 barcode? Freezing...");
    while (1)
      ;
  }
  Serial.println("Scanner online!");
}

void loop()
{
  flushRx();
  {
   
    if (scanner.readBarcode(scanBuffer, BUFFER_LEN))
    {
      Serial.print("Code found: ");
      for (int i = 0; i < strlen(scanBuffer); i++)
        Serial.print(scanBuffer[i]);
      Serial.println();
    }
  //if(millis() - currentTime)>= 2000)
    delay(200);
  }
 
  MRFC522_PROX_LOOP();
  LOOP_WEB_SERVER();
}

void SETUP_WEB_SERVER()
{

  while (!Serial)
  {
    ; // wait for serial port to connect.
  }
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}

void LOOP_WEB_SERVER()
{

  // listen for incoming clients
  EthernetClient client = server.available();
  if (client)
  {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank)
        {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close"); // the connection will be closed after completion of the response
          // client.println("Refresh: 5");        // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          for (int i = 0; i < strlen(scanBuffer); i++)
          client.print(scanBuffer[i]);
          client.println();
          client.println("<br />");
          for (byte i = 0; i < mfrc522.uid.size; i++)
          {
            if (mfrc522.uid.uidByte[i] < 0x10)
              client.print(F(" 0"));
            else
            client.print(F(" "));
            client.print(mfrc522.uid.uidByte[i], HEX);
          }
          client.println();

          client.println("<br />");

          client.println("</html>");
          break;
        }
        if (c == '\n')
        {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r')
        {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

void MRFC522_PROX_SETUP()
{

  SPI.begin(14, 12, 13, 15); // Inicia  SPI bus
  mfrc522.PCD_Init();        // Inicia MFRC522
  Serial.println("Aproxime o seu cartao do leitor...");
}
void MRFC522_PROX_LOOP()
{

  // Aguarda a aproximacao do cartao
  if (!mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  // Seleciona um dos cartoes
  if (!mfrc522.PICC_ReadCardSerial())
  {
    return;
  }
  int opcao = 0;
  if (opcao == 0)
    leituraDados();
  else if (opcao == 1)
    gravarDados();
  else
  {
    Serial.println(F("Opção Incorreta!"));
    return;
  }
  // instrui o PICC quando no estado ACTIVE a ir para um estado de "parada"
  mfrc522.PICC_HaltA();
  // "stop" a encriptação do PCD, deve ser chamado após a comunicação com autenticação, caso contrário novas comunicações não poderão ser iniciadas
  mfrc522.PCD_StopCrypto1();
}

// faz a leitura dos dados do cartão/tag
void leituraDados()
{
  //unsigned long startTimeRC522 = millis();
  // imprime os detalhes tecnicos do cartão/tag
  mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));

  // Prepara a chave - todas as chaves estão configuradas para FFFFFFFFFFFFh (Padrão de fábrica).
  for (byte i = 0; i < 6; i++)
    key.keyByte[i] = 0xFF;

  // buffer para colocar os dados ligos
  byte buffer[SIZE_BUFFER] = {0};

  // bloco que faremos a operação
  byte bloco = 1;
  byte tamanho = SIZE_BUFFER;

  // faz a leitura dos dados do bloco
  status = mfrc522.MIFARE_Read(bloco, buffer, &tamanho);
  if (status != MFRC522::STATUS_OK )
  {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));

    //delay(3000);
    
    //while((millis() - startTimeRC522) <= 1000)
    //{
      pause(1); //precisa fazer uma maneira de ler uma vez por segundo sem travar o código
      
   // }

    return;
  }
  else
  {
    //delay(1000);
    pause(1);
  }
  Serial.print(F("\nDados bloco ["));
  Serial.print(bloco);
  Serial.print(F("]: "));

  // imprime os dados lidos
  for (uint8_t i = 0; i < MAX_SIZE_BLOCK; i++)
  {

    Serial.write(buffer[i]);
  }

  Serial.println(" ");
}
// faz a gravação dos dados no cartão/tag
void gravarDados()
{
  // imprime os detalhes tecnicos do cartão/tag
  mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));
  // aguarda 30 segundos para entrada de dados via Serial
  Serial.setTimeout(30000L);
  Serial.println(F("Insira os dados a serem gravados com o caractere '#' ao final\n[máximo de 16 caracteres]:"));

  // Prepara a chave - todas as chaves estão configuradas para FFFFFFFFFFFFh (Padrão de fábrica).
  for (byte i = 0; i < 6; i++)
    key.keyByte[i] = 0xFF;

  // buffer para armazenamento dos dados que iremos gravar
  byte buffer[MAX_SIZE_BLOCK] = "";
  byte bloco;        // bloco que desejamos realizar a operação
  byte tamanhoDados; // tamanho dos dados que vamos operar (em bytes)

  // recupera no buffer os dados que o usuário inserir pela serial
  // serão todos os dados anteriores ao caractere '#'
  tamanhoDados = Serial.readBytesUntil('#', (char *)buffer, MAX_SIZE_BLOCK);
  // espaços que sobrarem do buffer são preenchidos com espaço em branco
  for (byte i = tamanhoDados; i < MAX_SIZE_BLOCK; i++)
  {
    buffer[i] = ' ';
  }

  bloco = 1;                   // bloco definido para operação
  String str = (char *)buffer; // transforma os dados em string para imprimir
  Serial.println(str);

  // Authenticate é um comando para autenticação para habilitar uma comuinicação segura
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                    bloco, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    // digitalWrite(pinVermelho, HIGH);
    delay(1000);
    // digitalWrite(pinVermelho, LOW);
    return;
  }
  // else Serial.println(F("PCD_Authenticate() success: "));

  // Grava no bloco
  status = mfrc522.MIFARE_Write(bloco, buffer, MAX_SIZE_BLOCK);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));

    delay(1000);

    return;
  }
  else
  {
    Serial.println(F("MIFARE_Write() success: "));

    delay(1000);
  }
}
// menu para escolha da operação
/*int menu()
{
  Serial.println(F("\nEscolha uma opção:"));
  Serial.println(F("0 - Leitura de Dados"));
  Serial.println(F("1 - Gravação de Dados\n"));

  // fica aguardando enquanto o usuário nao enviar algum dado
  while (!Serial.available())
  {
  };

  // recupera a opção escolhida
  int op = (int)Serial.read();
  // remove os proximos dados (como o 'enter ou \n' por exemplo) que vão por acidente
  while (Serial.available())
  {
    if (Serial.read() == '\n')
      break;
    Serial.read();
  }
  return (op - 48); // do valor lido, subtraimos o 48 que é o ZERO da tabela ascii
}*/
void flushRx()
{
  while (Serial.available())
  {
    Serial.read();
    delay(1);
  }
}
void pause (float delay1) {//retorna o delay1

   if (delay1<0.001) return; // pode ser ajustado e/ou evita-se valores negativos.

   float inst1=0, inst2=0;

   inst1 = (float)clock()/(float)CLOCKS_PER_SEC;

   while (inst2-inst1<delay1) inst2 = (float)clock()/(float)CLOCKS_PER_SEC;

   return;

}