 /*
 * VK2IDL's CW Morse Decoder program for Arduino [March 2020]
 * (c) 2020, Ian Lindquist - VK2IDL
 * 
 * This software is free of charge and may be used, modified or freely distributed.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED.
 * 
 * This program reads keyboard text characters from the Arduino IDE monitor and generates 
 * Morse Code for sending to a transceiver. All transmitted text is displayed in the Arduino 
 * IDE's monitor. The program also has 6 preset buffers pre-loaded with commonly used statements
 * including your CQ call, Name, QTH, and Antenna details.  Additional code has been included 
 * to allow your text to be displayed on a separate 20 x 4 LCD connected to the Arduino.
 * 
 * ==== Please refer to "Operation.txt" for general instructions. ====
 * 
 * Serveral software methods used in this program have been borrowed from Budd Churchward's 
 * "WB7FHC's Simple Morse Code Decoder v. 2.4" program. The VK2IDL Morse Encoder program was 
 * designed to compliment the "WB7FHC Simple Morse Code Decoder" to form a complete morse 
 * code Send/Receive station.
 * 
 * This software uses a method borrowed from "WB7FHC's Simple Morse Code Decoder which 
 * derives morse code from an array. The morse code for each character is stored in a 
 * 63 byte array called mySet[]. The binary value of the array position for each character 
 * represents the morse code for that character in 1's and 0's with 1 being a DIT and 0 
 * being a Dah. Preceeding each morse sequence is an additional 1 which is used as a flag 
 * to identify the start of the morse code.    
 * 
 * I have also borrowed from WB7FHC's 'printPunctuation()' function as it also works 
 * equally well for this application.
 * 
*/

//#define LCDYes 1             // Remove '//' to enable a connected 20x4 LCD

#ifdef LCDYes
  // ================= Include this section if using a 20x4 LCD ==================
  // Include libraries
  #include <LiquidCrystal_I2C.h>  // Include LCD library
  #include <Wire.h>               // Include the Wire library for Serial access and I2C control

  // Define LCD2004A Configuration pinout and address
  const int  en = 2, rw = 1, rs = 0, d4 = 4, d5 = 5, d6 = 6, d7 = 7, bl = 3;  // Define LCD's variables
  const int i2c_addr = 0x27;                                                  // Define LCD's I2C Address

  // Initialize the LCD variables with the numbers of the interface pins
  LiquidCrystal_I2C lcd(i2c_addr, en, rw, rs, d4, d5, d6, d7, bl, POSITIVE);  // Define LCD object from library and 
#endif

// Fix variables
int morseSpeed;               // Holds the Morse speed in WPM
int WPMCounter = 2;           // Defines default morse WPM value held in 'speedValues' array
                              // {0=5WPM, 1=10WPM, 2=12WPM, 3=15WPM, 4=20WPM, 5=25WPM, 6=30WPM}
                                                    
// Text and buffer variables
const unsigned int bufferMax = 55;   // Maximum size of the input buffer 
const unsigned int bufferSize = 53;  // Useable size of the input buffer
            
char bufferIn[bufferMax];           // Input buffer to hold characters typed by the user
int bufferIn_Count = 0;             // Keeps track of the # of characters in the buffer
char inputChar;                     // Holds keyboard input character
char bufferChar;                    // Holds the character taken from the keyboard input buffer
char morseChar;                     // Holds character extracted from the mySet[] Array
int morseNum;                       // Holds the location of the morse character in mySet[] array
int txLCD_Count = -1;               // Start location for the TX output text print on the LCD
const int tx_LCD_Start = 0;         // Fixed start position for TX text 
char txBuffer[20];                  // Holds LCD TX display buffer for scrolling
char presetBuffer;                  // Hold the content of the preset buffer being used

// Morse Output pins
int morsePin_LED = 6;               // Defines the Output pin to drive the Morse Code flasher LED
int morsePin_CODE = 7;              // Defines the Output pin to drive the Morse code keyer signal
int morsePin_RXin = 8;              // Defines the output pin to drive the morse decoder input
                                    // NOTE: This pin is an inverted signal to the LED and CODE outputs
// Morse-Key Input Pins
int keyPin = 9;                     // Input for traditional morse key
int paddlePin_LEFT = 10;            // Input for morse paddle (Left side)
int paddlePin_RIGHT = 11;           // Input for morse paddle (Right side)

int morseKeyState = 1;              // Stores the state of the Morse Key
int LEFTpaddleState = 1;            // Stores the state of the Left Paddle Key
int RIGHTpaddleState = 1;           // Stores the state of the Right Paddle Key

// Morse Code timing parameters
float ditLength;        // Length of a dit based on the defined Morse Speed
float dahLength;        // Length of a Dah referenced to the Dit length
float characterSpace;   // Space between characters referenced to the Dit length
float elementSpace;     // Space between elements referenced to the Dit length
float wordSpace;        // Space between words referenced to the Dit length

// ============== Preset Morse Messages Here ===============
char CQ_Mess[] = " CQ CQ CQ DE VK2IDL VK2IDL K ";
char NAME_Mess[] = " NAME IS IAN IAN. ";
char QTH_Mess[] = " QTH IS PORT MACQUARIE PORT MACQUARIE. ";
char SIGNAL_Mess[]=" YR RST ";
char ANT_Mess[] = " ANTENNA IS MULTIBAND TRAP DIPOLE. ";
char CQTest_Mess[] = " CQTEST CQTEST CQTEST DE VK2IDL. ";


// ============ Preset Morse Speeds ================
int speedValues[] = {5, 10, 12, 15, 20, 25, 30};

// Define morse characters in an array
char mySet[] ="##TEMNAIOGKDWRUS##QZYCXBJP#L#FVH09#8###7#####/=61#######2###3#45"; 

void setup() // === INITIALISATION CODE ====
{
  Serial.begin(9600);               // Setup serial communication for Monitor
  
  pinMode(morsePin_LED, OUTPUT);    // Define the Morse LED pin
  pinMode(morsePin_CODE, OUTPUT);   // Define the Morse Code-Sending Pin
  pinMode(morsePin_RXin, OUTPUT);   // Define the Morse-to-RXinput pin
  digitalWrite(morsePin_RXin,HIGH); // Set Morse-to-RXinput pin HIGH to match 
                                    // the morse decoder signal input logic
  // Define Morse Key Input Pins
  pinMode(keyPin, INPUT);          // Define the Manual Morse Key Input pin
  digitalWrite(keyPin,1);          // Turn on internal pull up resistor

  pinMode(paddlePin_LEFT, INPUT);  // Define the Morse Paddle-Left pin
  digitalWrite(paddlePin_LEFT,1);  // Turn on internal pull up resistor
  pinMode(paddlePin_RIGHT, INPUT); // Define the Morse Paddle-Right pin
  digitalWrite(paddlePin_RIGHT,1); // Turn on internal pull up resistor

  // Print Header
  Serial.println("=== VK2IDL Morse Code Generator ==="); // Print initialisation header
  Serial.println();

  // Print Morse Speed
  morseSpeed = speedValues[WPMCounter];       // Load default morse speed from array
  Serial.print("Default morse speed = ");     // Print default morse speed
  Serial.print(morseSpeed);
  Serial.println(" WPM.");
  
  // Initialised the Morse Code timing parameters using the 'morseSpeed' value calculated above
  morseTiming();

  // Print On-Screen Instructions
  Serial.println("To adjust the Morse Speed (WPM), use the key above the 'Tab' key.");
  Serial.println("* Press '~' then <Enter> to Increase WPM.");
  Serial.println("* Press '`' then <Enter> to Decrease WPM.");
  Serial.println();
  Serial.println("To select 'Recall' buffers, use the following keys.");
  Serial.println("[ = Buffer #1       ] = Buffer #2       \\ = Buffer #3");
  Serial.println("{ = Buffer #4       } = Buffer #5       | = Buffer #6");
  Serial.println();
    
  // Initialise the Input buffer with zeros
  memset(bufferIn, 0, bufferMax);   

#ifdef LCDYes
// ================= if using an LCD ==================
  lcd.begin(20, 4);                 // Define the LCD as a 20x4 display
  lcd.clear();                      // Clear the display on startup

  lcd.setCursor(0,0);               // Position the cursor
  lcd.print("TX>");                 // Identify the TX text line
#endif

}



void loop() // ==== MAIN PROGRAM LOOP ====
{
/*
 * The program first checks to see if the morse Paddle or Manual morse Key are active. If the 
 * Paddle is active, it processes DITs or DAHs depending on whether the left or right paddle 
 * is pressed. If the manual morse key is active, it activates the morse tone for as long as 
 * the Manual key is held (morseKeyDN) and releases the tone it once the Manual key is released 
 * (morseKeyUP). 
 * If neither the paddle nor the morse key are active, the program processes morse from the 
 * keyboard and preset buffers.
 */
   //Check the state of the Morse Paddle and Manual Morse Key pins
   LEFTpaddleState = digitalRead(paddlePin_LEFT);   // What is the Left Morse paddle doing?
   RIGHTpaddleState = digitalRead(paddlePin_RIGHT); // What is the Right Morse paddle doing?
   morseKeyState = digitalRead(keyPin);             // What is the state of the standard Morse key
   
   // If the Paddle's LEFT or RIGHT keys are active
   if ((!LEFTpaddleState) |  (!RIGHTpaddleState))   // If either paddle pin is LOW (0), the paddle is active
   {
     if (!LEFTpaddleState)                          // If its the left paddle
     {
        leftPaddleDN();                             // Send DAHs
     }
     else if (!RIGHTpaddleState)                    // Otherwise if its the Right paddle
     {
        rightPaddleDN();                            // Send DITs
     }
   }
  
  // Otherwise if the Manual morse key is active
  else if (!morseKeyState)                          // If Manual Key pin is LOW (0), Morse key is active
  {
    do                                              // Enable the morse tone while the key is pressed 
    {
      morseKeyDN();                                 // Activate the Morse Tone
      morseKeyState = digitalRead(keyPin);          // Check the state of the Manual Morse key
    } while (!morseKeyState);                       // If still pressed, continue the tone
    
     morseKeyUP();                                  // Otherwise release the tone
  }

   // If none of the Morse keys have been pressed, process the keyboard characters
  else
  {
    getTX_Characters();       // Get characters from user keyboard and put them into a buffer
    processTX_Buffer();       // Process the characters in the buffer into Morse Code
  }   
}



// ============================== FUNCTIONS GO HERE ===============================

/* ========= Gets input from the keyboard via the Arduino IDE monitor ========= 
 * Special command characters i.e. those used to change the morse speed or recall 
 * preset buffers are extracted and processed separately. The remaining characters are 
 * converted to uppercase where necessary then inserted into an input buffer array. 
 */
void getTX_Characters()
{
  // ==== Get input characters from the serial keyboard ====
  while (Serial.available()!=0)                 // If a character is being read, do this
  { 
    inputChar = Serial.read();                  // Read user input into variable 'inputChar'
        
    // ==== Isolate special command characters from the raw keyboard input ====
    if ((int(inputChar) >= 91 & int(inputChar) <= 93)
      | (int(inputChar) == 96)
      | (int(inputChar) >= 123 & int(inputChar) <= 126))
      {
        processCommands();                     // Process special command sequences
        return;                                // Exit from here - command characters arent Morse Code
      }
    
    else  
    // ==== Put remaining valid characters into the Input Buffer ====
      {
        // Convert lowercase characters to uppercase
        if (int(inputChar) >= 97 &  int(inputChar) <= 122)  // Check for valid lowercase character
          {
            inputChar = int(inputChar) - 32;          // Convert character to uppercase
          }
    
        // Load the characters into the Input Buffer then increment the counter
        if (bufferIn_Count < bufferSize)              // If there is room in the buffer?
          {
            bufferIn[bufferIn_Count] = inputChar;     // Load the character into the buffer
            bufferIn_Count++;                         // Increment the buffer counter for the next character
          }
      }
  }
}


/* ====== Takes characters stored in the input buffer and sends them as morse code ======
 * Characters are taken from the first position in the buffer (bufferIn[0]). All characters
 * are then rotated left putting the next available character in bufferIn[0]. The process
 * is continued until characters have been processed and bufferIn[0] has a value of 0 
 * indicating the buffer is empty. The buffer is then deliberately cleared by filling it 
 * with 0's. 
 */
void processTX_Buffer()
{
   // Get the character from the start of the buffer then rotate the rest of the character one space left
   if (int(bufferIn[0]) != 0)                   // If there are characters in the buffer
   {
      bufferChar = bufferIn[0];                 // Get the first character from the buffer
     
      // Rotate all the characters in the buffer 1 space to the left
      for (int K = 0; K <= bufferIn_Count; K++) // Loop for the number of characters in the buffer
      {
        bufferIn[K] = bufferIn[K+1];            // Copy buffer characters 1 position to the left
      }
        
      bufferIn_Count--;                         // Decrement the buffer counter for the next input read
      if (bufferIn_Count <= 0)                  // Make sure bufferIn_Count is never less than 0
      {
        bufferIn_Count = 0;                     // Buffer is empty
        memset(bufferIn, 0, bufferMax);         // For safety, clear empty buffer by filling with 0s
      }

      // ==================== Process Characters into Morse Code ============================
      // If the character is a SPACE, apply a 'word-space' delay
      if (bufferChar == 32)                        // If the character is a <SPACE> character
      {
        delay(wordSpace);                          // Insert a 'word-space' delay to the morse
        Serial.print(bufferChar);                  // Print a space on the Screen

#ifdef LCDYes
// ================= if using an attached LCD ==================
        LCDprint_TXChar();                      // Print the space on the LCD
#endif

      }
        
      // Otherwise check for illegal punctuation characters
      else if((bufferChar >= 34 && bufferChar <= 38) 
                | (bufferChar >= 42 && bufferChar <= 43)
                | (bufferChar >= 59 && bufferChar <= 60)
                | (bufferChar == 62))
      {
        Serial.print("#");                         // Print '#' if the character is not recognised
      }
        
      // Otherwise if the buffer is empty
      else if (bufferChar == 0 && bufferIn_Count == 0)   
      {
         memset(bufferIn, 0, bufferMax);           // Fill buffer with 0s to clear it
      }

      // Otherwise search the 'mySet[]' Array for a matching character using 'morseNum' values from 0 to 64
      // The resulting bit value of 'morseNum' holds the Morse Code
      else
      {
        morseNum = 0;                           // start the array counter at 0
        do  
        {
          morseChar = mySet[morseNum];          // Read a character from the array
          morseNum++;                           // Increment the counter
          if (morseNum > 63)                    // Check to see if the counter has reached the end 
          {                                     // of the array without finding a matching character
             break;                             // If so, exit from the loop
          }
        } while (morseChar != bufferChar);      // Otherwise repeat the loop until a character match is found

        // Check for Punctuation (standard characters that werent in the array)
        if (morseNum > 63)                      // If the counter exceeded the array length
        {
          printPunctuation();                   // Look for the character in the  Punctuation table
        }

        // Otherwise send the character found in the array
        else 
        {
          morseNum = morseNum - 1;              // Correct morseNum - needed value was the previous count
        }

        Serial.print(bufferChar);               // Display the character being sent
        if (bufferChar == 46)                   // If the character was a full-stop (.)
        {
           Serial.println();                    // Print Line Feed on the display when a (.) is sent
        }
/*  ================= Add this section if using an attached LCD ==================
          LCDprint_TXChar();                    // Print the character on the LCD
*/
        sendCharacter(morseNum);                // Convert character to morse and send
        delay(characterSpace);                  // Add the correct space between characters
      }
  }
}


/*============= Decodes the embedded morse in 'morseNum' and sends it.=============
 * The morse code is embedded in the value of 'morseNum'. The bits in 'morseNum' are 
 * rotated left and bit 7 of 'morseNum' is checked for the first logic 1 bit. 
 * This indicates the start of the morse character so this bit is discarded as the 
 * actual morse character is encoded in the bits that follow. 
 * The bits continue to be rotated left and the subsequent 1's or 0's at bit 7 are 
 * then used to call the morse_Dah or morse_Dit functions that send the morse code.
 */
void sendCharacter(int morseNum)
{
  int bitCount = 0;               // Keeps track of the number of bit rotations (total 8)

// =========== Rotate all bits to the left until the morse-flag bit is found at bit 7 ================
// ================== Subsequent bits contain the morse code  =======================  

  // Find the Morse-flag
  do                                                                          
    {
      morseNum = morseNum << 1;              // Rotate the left-most bit in 'morseNum' 
      bitCount++;                            // Increment the counter to keep track of the number of bits processed
    } while (bitRead(morseNum, 7) != 0x01);  // Loop until the 7th bit value = 1


  // Extract the morse code using the subsequent bits in morseNum
  for (int I= bitCount+1; I <= 7; I++)       // Loop for the remaining bits in morseNum
  {
    morseNum = morseNum << 1;                // Rotate the left-most bit in 'morseNum' 
    if (bitRead(morseNum, 7) != 0x01)        // if the 7th bit is not a 1
    {
      morse_Dah();                           // Send a DAH
    }
    else                                     // Otherwise
    {
      morse_Dit();                           // Send a DIT
    }
  }
}


/* =================== Morse Timing ================================
 * Defines the timing ratios of each element of the morse code.
 * The specific timing used is based on the morse speed in variable 'morseSpeed'. 
 * The morse speed can be selected by the user using command characters ` and ~. 
 * After each morse speed change by the user, this function is called to reset the 
 * morse speed parameters for dits, dahs and character/word spacing.
 * === Ratios are as list below === 
 *   DIT Length Ratio = 1
 *   DAH Length Ratio = 3
 *   Element Space Ratio = 1
 *   Character Space Ratio = 3
 *   Word Space Ratio = 7
 *   DIT Length = 1200/WPM (based on the word 'PARIS')
 */
void morseTiming()
{
  // Morse Code timing parameters
  ditLength = 1200/morseSpeed;      // Length of a dit based on the defined Morse Speed
  dahLength = ditLength * 3;        // Length of a Dah referenced to the Dit length
  characterSpace = ditLength * 3;   // Space between characters referenced to the Dit length
  elementSpace = ditLength;         // Space between elements referenced to the Dit length
  wordSpace = ditLength * 7;        // Space between words referenced to the Dit length
}


/*  ====================== Send DITs and DAHs ==========================
 *  Switch arduino pins HIGH or LOW to send morse code.
 *  Three pins are used for various morse code outputs.
 *  
 *  The =LED= pin drives an LED to provide a visual indication that your 
 *  morse code is being sent. 
 *  
 *  The =CODE= pin drives the radio's morse-key input via a 2N2222 transistor
 *  to send the morse code over the radio. 
 *  
 *  The =RXin= pin connects directly to the digital input pin of a separate 
 *  WB7FHC Morse Decoder unit. This provides an identical signal to the output 
 *  of the WB7FHC morse decoder's LM567 tone decoder IC, allowing your 
 *  transmitted morse to be decoded and displayed on the decoder's LCD. 
 *  You should not attempt to receive and send morse code at the same time as 
 *  the two signals will confuse the decoder.
 *  
 *  NOTE: The signal logic on the =RXin= pin is inverted compared to the 
 *  =LCD= and =CODE= pins which is why it requires a separate pin.
 */
// Send a DIT
void morse_Dit()                          // Send a DIT
{
  digitalWrite(morsePin_LED, HIGH);       // Take LED Pin HIGH
  digitalWrite(morsePin_CODE, HIGH);      // Take CODE Pin HIGH
  digitalWrite(morsePin_RXin,LOW);        // Take RX Input LOW
  
  delay(ditLength);                       // Hold the Pins High for the length of a DIT
  
  digitalWrite(morsePin_LED, LOW);        // Take LED Pin LOW
  digitalWrite(morsePin_CODE, LOW);       // Take CODE Pin LOW
  digitalWrite(morsePin_RXin,HIGH);       // Take RX Input HIGH

  delay(elementSpace);                    // Add an ELEMENT space at the end of the DIT
}

// Send a DAH 
void morse_Dah()                          // Send a DAH
{
  digitalWrite(morsePin_LED, HIGH);       // Take LED Pin HIGH
  digitalWrite(morsePin_CODE, HIGH);      // Take CODE Pin HIGH
  digitalWrite(morsePin_RXin,LOW);        // Take RX Input LOW

  delay(dahLength);                       // Hold the Pins High for the length of a DAH
  
  digitalWrite(morsePin_LED, LOW);        // Take LED Pin LOW
  digitalWrite(morsePin_CODE, LOW);       // Take CODE Pin LOW
  digitalWrite(morsePin_RXin,HIGH);       // Take RX Input HIGH

  delay(elementSpace);                    // Add an ELEMENT space and the end of the DAH
}

#ifdef LCDYes
/* ============ This function can be include if using an LCD =============
 * This is a debugging function which can be used to display the number
 * of characters currently in the input buffer on an attached LCD. This is 
 * sometimes useful as the buffer has a limited size and will be overloaded 
 * if too many characters are entered.Unfortunately printing this value on 
 * the serial monitor isnt very useful as it cannot be displayed separately 
 * from the rest of the text. This function is really only useful when using 
 * an LCD as the value can then be displayed in a specific location on the 
 * LCD away from other content.
 */
 
void LCD_Buffer_Count()
{
  lcd.setCursor(0,0);                     // Position the cursor

  if (bufferIn_Count < 10)                // If the buffer counter is less than 10
  {
   lcd.print('0');                        // Print a preceeding 0
  }
     
  lcd.print(bufferIn_Count);              // Then print the number
}


/*  ============ Include this function include if using an attached LCD =============
 *  This function displays your transmitted characters on a connected LCD. 
 *  If you prefer to connect an LCD, please enable this code.
 *  Versions of this program that display the transmitted text using the 
 *  arduino IDE monitor are not required to enable this function
 */
 
void LCDprint_TXChar()
{
  
  if (txLCD_Count <= 18)                          // While the cursor position is less that 19
  {
    txLCD_Count++;                                // Increment the cursor position
    lcd.setCursor(txLCD_Count,0);                 // Set the cursor position
    lcd.print(bufferChar);                        // Print the latest character
    txBuffer[txLCD_Count] = bufferChar;           // Load the latest character into the buffer
  }
  else                                            // otherwise once the cursor position has reached 19
  {
    // Rotate all characters in the TX storage buffer one position to the left
    for (int LL = tx_LCD_Start; LL <= 19; LL++)   // For the length of the TX buffer
    {
      txBuffer[LL] = txBuffer[LL+1];              // Move the characters left in the buffer
    }
   // Print the characters in their new positions
    for (int LL = tx_LCD_Start; LL <= 19; LL++)   // For the length of the TX buffer
    {
      lcd.setCursor(LL,0);                // Position the cursor
      lcd.print(txBuffer[LL]);            // Display the characters in their new positions
    }
    txBuffer[19] = bufferChar;            // Load the latest character into the array
    lcd.setCursor(19,0);                  // Position the cursor in the last column
    lcd.print(bufferChar);                // Print the latest character
  }
}
#endif


/* ====== Keyboard Command Character Functions for recalling Preset Buffers ==================
 * These functions that are called by the processCommands() function to send messages 
 * stored in the preset buffers. There are 6 preset buffers driven by keyboard 
 * commands using special characters as listed below.
 * [ = buffer #1
 * ] = buffer #2
 * \ = buffer #3
 * { = buffer #4
 * } = buffer #5
 * | = buffer #6
 * 
 */
// Get characters from the CQ_Mess buffer and put them in the Input buffer 
void sendCQ_Mess()
{
  for (int PP=0; PP <= sizeof(CQ_Mess); PP++)     // For the size of the buffer
  {
    inputChar = CQ_Mess[PP];                      // Read character into variable 'inputChar'

    if (inputChar !=0)                            // As long as the next character isnt a 0
    {
      if (bufferIn_Count < bufferSize)            // If there is room in the buffer?
      {
       bufferIn[bufferIn_Count] = inputChar;      // Load the character into the buffer
       bufferIn_Count++;                          // Increment the buffer counter for the next character
      }
    }
    else                                          // Otherwise its the end of the buffer
    {
      return;                                     // Return. We're done here
    }
  } 
}


// ========== Get characters from the ANT_Mess buffer and put them in the Input buffer ==========
void sendANT_Mess()
{
  for (int PP=0; PP <= sizeof(ANT_Mess); PP++)    // For the size of the buffer
  {
    inputChar = ANT_Mess[PP];                     // Read character into variable 'inputChar'

    if (inputChar !=0)                            // As long as the next character isnt a 0
    {
      if (bufferIn_Count < bufferSize)            // If there is room in the buffer?
      {
       bufferIn[bufferIn_Count] = inputChar;      // Load the character into the buffer
       bufferIn_Count++;                          // Increment the buffer counter for the next character
      }
    }
    else                                          // Otherwise its the end of the buffer
    {
      return;                                     // Return. We're done here
    }
  } 
}



// ========== Get characters from the CQTest_Mess buffer and put them in the Input buffer ==========
void sendCQTest_Mess()
{
  for (int PP=0; PP <= sizeof(CQTest_Mess); PP++)    // For the size of the buffer
  {
    inputChar = CQTest_Mess[PP];                     // Read character into variable 'inputChar'
        
    if (inputChar !=0)                               // As long as the next character isnt a 0
    {
      if (bufferIn_Count < bufferSize)               // If there is room in the buffer?
      {
       bufferIn[bufferIn_Count] = inputChar;         // Load the character into the buffer
       bufferIn_Count++;                             // Increment the buffer counter for the next character
      }
    }
    else                                             // Otherwise its the end of the buffer
    {
      return;                                        // Return. We're done here
    }
  } 
}



// ========== Get characters from the QTH_Mess buffer and put them in the Input buffer ==========
void sendQTH_Mess()
{
  for (int PP=0; PP <= sizeof(QTH_Mess); PP++)     // For the size of the buffer
  {
    inputChar = QTH_Mess[PP];                      // Read character into variable 'inputChar'
        
    if (inputChar !=0)                             // As long as the next character isnt a 0
    {
      if (bufferIn_Count < bufferSize)             // If there is room in the buffer?
      {
       bufferIn[bufferIn_Count] = inputChar;       // Load the character into the buffer
       bufferIn_Count++;                           // Increment the buffer counter for the next character
      }
    }
    else                                           // Otherwise its the end of the buffer
    {
      return;                                      // Return. We're done here
    }
  } 
}


// ========== Get characters from the NAME_Mess buffer and put them in the Input buffer ==========
void sendNAME_Mess()
{
  for (int PP=0; PP <= sizeof(NAME_Mess); PP++)     // For the size of the buffer
  {
    inputChar = NAME_Mess[PP];                      // Read character into variable 'inputChar'
        
    if (inputChar !=0)                              // As long as the next character isnt a 0
    {
      if (bufferIn_Count < bufferSize)              // If there is room in the buffer?
      {
       bufferIn[bufferIn_Count] = inputChar;        // Load the character into the buffer
       bufferIn_Count++;                            // Increment the buffer counter for the next character
      }
    }
    else                                            // Otherwise its the end of the buffer
    {
      return;                                       // Return. We're done here
    }
  } 
}

// ========== Get characters from the SIGNAL_Mess buffer and put them in the Input buffer ==========
void sendSIGNAL_Mess()
{
  for (int PP=0; PP <= sizeof(SIGNAL_Mess); PP++)    // For the size of the buffer
  {
    inputChar = SIGNAL_Mess[PP];                     // Read character into variable 'inputChar'
        
    if (inputChar !=0)                               // As long as the next character isnt a 0
    {
      if (bufferIn_Count < bufferSize)               // If there is room in the buffer?
      {
       bufferIn[bufferIn_Count] = inputChar;         // Load the character into the buffer
       bufferIn_Count++;                             // Increment the buffer counter for the next character
      }
    }
    else                                             // Otherwise its the end of the buffer
    {
      return;                                        // Return. We're done here
    }
  } 
}  

// =================== Identify punctuation characters ====================
void printPunctuation() 
{
  // Punctuation marks are made up of more dits and dahs than
  // letters and numbers. Rather than extend the character array
  // out to reach these higher numbers we will simply check for
  // them here. This funtion only gets called when myNum is greater than 63
  
  // Thanks to Jack Purdum for the changes in this function
  // The original uses if then statements and only had 3 punctuation
  // marks. Then as I was copying code off of web sites I added
  // characters we don't normally see on the air and the list got
  // a little long. Using 'switch' to handle them is much better.

  switch (bufferChar) 
  {
    case ':':                 // ':'  [- - - . . .]
      morseNum = 71;          // Set Morsecode number
      break;

    case ',':                 // ','  [- - . . - -] 
      morseNum = 76;          // Set Morsecode number
      break;

    case '!':                 // '!'  [- . - . - -]
      morseNum = 84;          // Set Morsecode number
      break;

    case '-':                 // '-'  [- . . . . -]
      morseNum = 94;          // Set Morsecode number
      break;

    case 39:                  // Apostrophe  [. - - - - .]
      morseNum = 97;          // Set Morsecode number
      break;

    case '@':                 // 101, '@' [. - - . - .]
      morseNum = 101;         // Set Morsecode number
      break;

    case '.':                 // 106, '.' [. - . - . -]
      morseNum = 106;         // Set Morsecode number
      break;

    case '?':                 // 115, '?' [. . - - . .] 
      morseNum = 115;
      break;

    case '$':                 // 246, '$' [. . . - . . -]
      morseNum = 246;         // Set Morsecode number
      break;

    default:
      morseNum = '0';         // Should not get here
      break;
  }
}


// ============ Process special COMMAN characters to access preset buffers ==============
// ======================= and change Morse Speed ================================
void processCommands()
{
   switch (inputChar)       // Check the value of the raw keyboard input
  {
    case 96:                                // ===== '`' Morse Speed DOWN Command =====
      WPMCounter--;                         // Decrement the WPM counter
      
      if (WPMCounter <=0)  // Ensure minimum WPMCounter value never gets below 0
      {
        WPMCounter = 0; 
      }

      // Update the new morse speed value
      morseSpeed = speedValues[WPMCounter]; // Load the new morse speed from array
      morseTiming();                        // Reset timing variables using the new value

      // Print the new morse speed value
      Serial.println();                     // Print a linefeed
      Serial.print(morseSpeed);             // Print the Morse Speed
      Serial.println(" WPM");               
      break;
      
    case 126:                               // ===== '~' Morse Speed UP =====
      WPMCounter++;                         // Decrement the WPM counter
      
      if (WPMCounter >=6)  // Ensure maximum WPMCounter value never gets above 6
      {
        WPMCounter = 6; 
      }
      
      // Update the new morse speed value
      morseSpeed = speedValues[WPMCounter]; // Load morse speed from array
      morseTiming();                        // Reset the timing variables

      // Print the new morse speed value
      Serial.println();                     // Print a linefeed
      Serial.print(morseSpeed);             // Print the Morse Speed
      Serial.println(" WPM");               
      break;
      
    case 91:                        // '[' pressed - Recall Buffer #1 =====
      sendCQ_Mess();                // Process Send CQ message 
      processTX_Buffer;             // Send as morse
      Serial.println();             // Print a linefeed 
      break;
      
    case 93:                        // ']' pressed - Recall Buffer #2 =====
      sendNAME_Mess();              // Process NAME buffer 
      processTX_Buffer;             // Send as morse
      Serial.println();             // Print a linefeed 
      break;
       
    case 92:                        // '\' pressed - Recall Buffer #3 =====
      sendQTH_Mess();               // Process QTH buffer 
      processTX_Buffer;             // Send as morse
      Serial.println();             // Print a linefeed 
      break;
      
    case 123:                       // '{' pressed - Recall Buffer #4 =====
      sendSIGNAL_Mess();            // Process the SIGNAL Strength buffer 
      processTX_Buffer;             // Send as morse
      Serial.println();             // Print a linefeed 
      break;
      
    case 125:                       // '}' pressed - Recall Buffer #5 =====
      sendANT_Mess();               // Process ANTENNA buffer 
      processTX_Buffer;             // Send as morse
      Serial.println();             // Print a linefeed 
      break;
     
    case 124:                       // '|' pressed - Recall Buffer #6 =====
      sendCQTest_Mess();            // Process buffer #1 
      processTX_Buffer;             // Send as morse
      Serial.println();             // Print a linefeed
      break;
 
    default:                        // Capture anything else here
      break;
  }
}

// Send a DAH while the left paddle is being held
void leftPaddleDN()
{
  digitalWrite(morsePin_LED, HIGH);       // Take LED Pin HIGH
  digitalWrite(morsePin_CODE, HIGH);      // Take CODE Pin HIGH
  digitalWrite(morsePin_RXin,LOW);        // Take RX Input LOW

  delay(dahLength);                       // Hold the Pins High for the length of a DAH
  
  digitalWrite(morsePin_LED, LOW);        // Take LED Pin LOW
  digitalWrite(morsePin_CODE, LOW);       // Take CODE Pin LOW
  digitalWrite(morsePin_RXin,HIGH);       // Take RX Input HIGH

  delay(ditLength);                       // Hold the Pins High for the length of a DIT
}


// Send a DIT while the right paddle is being held
void rightPaddleDN()
{
  digitalWrite(morsePin_LED, HIGH);       // Take LED Pin HIGH
  digitalWrite(morsePin_CODE, HIGH);      // Take CODE Pin HIGH
  digitalWrite(morsePin_RXin,LOW);        // Take RX Input LOW
  
  delay(ditLength);                       // Hold the Pins High for the length of a DIT
  
  digitalWrite(morsePin_LED, LOW);        // Take LED Pin LOW
  digitalWrite(morsePin_CODE, LOW);       // Take CODE Pin LOW
  digitalWrite(morsePin_RXin,HIGH);       // Take RX Input HIGH

delay(ditLength);                       // Hold the Pins High for the length of a DIT
}

// Send manual Morse Code Tone
void morseKeyDN()
{
    digitalWrite(morsePin_LED, HIGH);       // Take LED Pin HIGH
    digitalWrite(morsePin_CODE, HIGH);      // Take CODE Pin HIGH
    digitalWrite(morsePin_RXin,LOW);        // Take RX Input LOW
}

// Release manual Morse Code Tone
void morseKeyUP()
{
  digitalWrite(morsePin_LED, LOW);        // Take LED Pin LOW
  digitalWrite(morsePin_CODE, LOW);       // Take CODE Pin LOW
  digitalWrite(morsePin_RXin,HIGH);       // Take RX Input HIGH
}
