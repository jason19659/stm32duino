/*
 * Initial Author: ryand1011 (https://github.com/ryand1011)
 *
 * Reads data written by a program such as "rfid_write_personal_data.ino"
 *
 * See: https://github.com/miguelbalboa/rfid/tree/master/examples/rfid_write_personal_data
 *
 * Uses MIFARE RFID card using RFID-RC522 reader
 * Uses MFRC522 - Library
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 *
 * More pin layouts for other boards can be found here: https://github.com/miguelbalboa/rfid#pin-layout
 */

#include <SPI.h>
#include <MFRC522.h>
#define STM32F10X_LD STM32F10X_LD
#define RST_PIN A3 // Configurable, see typical pin layout above
#define SS_PIN A4  // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance

String inputString = "";
//*****************************************************************************************//
void setup()
{
  inputString.reserve(256);
  Serial.begin(9600);                 // Initialize serial communications with the PC
  SPI.begin();                        // Init SPI bus
  mfrc522.PCD_Init();                 // Init MFRC522 card
  Serial.println(F("INIT SUCCESS:")); // shows in serial that it is ready to read
}
uint32_t loopCnt = 0;
bool errorFlag = false;

bool flag = 1;

uint8_t cmd[257] = {0};
uint8_t cmdlen;
uint8_t recvbuf[512] = {0};
uint8_t recvlen;
char buf[64];
char print_buf[64];
//*****************************************************************************************//
void loop()
{


  if (inputString.length() > 0)
  {
    Serial.println(inputString[0], 16);
    Serial.println(inputString[0] == 0xFE);

    if (inputString[0] == 0XFF)
    {
      // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
      MFRC522::MIFARE_Key key;
      for (byte i = 0; i < 6; i++)
        key.keyByte[i] = 0xFF;

      // some variables we need
      byte block;
      byte len;
      MFRC522::StatusCode status;

      //-------------------------------------------

      // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
      if (!mfrc522.PICC_IsNewCardPresent())
      {
        return;
      }

      // Select one of the cards
      if (!mfrc522.PICC_ReadCardSerial())
      {
        return;
      }

      Serial.println(F("**Card Detected:**"));

      //-------------------------------------------

      mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid)); // dump some details about the card

      // mfrc522.PICC_DumpToSerial(&(mfrc522.uid));      //uncomment this to see all blocks in hex

      //-------------------------------------------

      
    }
    else if (inputString[0] == 0XFE)
    {
      cmdlen = 0;
      byte command = 0x40;

      

      Serial.println("0x40");

      memset(recvbuf, 0, 512);
      byte b8[1] = {8};
      byte b7[1] = {7};
      byte b255[1] = {255};
      mfrc522.PCD_CommunicateWithPICC(command, 0x30, {}, cmdlen, recvbuf, b255, b7,0, true);
      // recvlen = nfc.finitepiSendData(cmd, cmdlen, recvbuf);
      Serial.println(recvlen);
      Serial.print("<< ");
      for (int i = 0; i < recvlen; i++)
      {
        sprintf(print_buf, "%02X ", recvbuf[i]);
        Serial.print(print_buf);
      }
      Serial.println("");
    }
    else
    {
      Serial.println(">> other command <<" );
      cmdlen = inputString.length();
      byte command = inputString[0];
      if (cmdlen > 1)
      {
        for (int i = 1; i < cmdlen; i++)
        {
          cmd[i] = inputString[i];
        }
      }
      Serial.println(">> " + command);
      Serial.print(">> ");
      for (int i = 0; i < cmdlen; i++)
      {
        sprintf(print_buf, "%02X ", cmd[i]);
        Serial.print(print_buf);
      }
      Serial.println("");

      memset(recvbuf, 0, 512);
      byte b8[1] = {8};
      byte b7[1] = {7};
      byte b255[1] = {255};
      Serial.print("back length : " );
      Serial.print(*b255 );

      
      mfrc522.PCD_CommunicateWithPICC(command, 0x00, cmd, cmdlen, recvbuf, b255, b8,0, true);
      // recvlen = nfc.finitepiSendData(cmd, cmdlen, recvbuf);
      Serial.println(recvlen);
      Serial.print("<< ");
      for (int i = 0; i < sizeof(recvbuf); i++)
      {
        sprintf(print_buf, "%02X ", recvbuf[i]);
        Serial.print(print_buf);
      }
      Serial.println("");
    }
    // clear the string:
    inputString = "";
  }
}

void serialEvent()
{
  while (Serial.available())
  {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    delay(2);
  }
}
