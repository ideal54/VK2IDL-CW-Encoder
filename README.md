# VK2IDL-CW-Encoder
Morse Code encoder for the Arduino

GENERAL

This program reads text characters typed from the Arduino IDE monitor's keyboard and 
generates Morse Code for sending to a transceiver. It can also read text from a 
series of precoded buffers stored in the software. The buffers can be recalled by typing
special characters on the keyboard. 

It will also accept input from a standard Morse key as well as from a single-arm Paddle key.

The morse sending speed can be manually adjusted in preset steps between 5 WPM and 30 WPM. 
Users can alter the software to provide a larger range or more steps than those currently 
provided. Please refer to the instructions further below for a list of command characters 
and what they do.

All transmitted text is displayed on the Arduino IDE's monitor. Additional code has been 
included to display transmitted text on a 20 x 4 LCD display connected to the Arduino by 
an I2C bus. This code is presently under the control of an ifdef command preventing
compilation of the LCD code. Simply enable the #ifdef header then recompile the code 
to include the LCD code.

This Morse Encoder program was specifically designed to be used in conjunction with the 
"WB7FHC Simple Morse Code Decoder v2.4" to form a complete morse code Send/Receive station. 
The encoder includes a dedicated morse output pin with inverted logic that has been 
specifically included for connecting to the input pin of the WB7FHC Decoder. Your code 
will then conveniently appear on the WB7FHC Decoder's display. It works best when the morse
speed on both units are a close match.

NOTE: If connecting the morse output pin of the VK2IDL Morse Encoder directly to the 
WB7FHC morse Decoder as described above, you should not try to decode incoming morse 
from another source at the same time as you are sending it as it will just confuse the 
WB7FHC Decoder resulting in coding errors on the display.

MORSE ENCODING

Budd Churchward's Morse Encoding software method as used in the "WB7FHC's Simple Morse 
Code Decoder v. 2.4" program has been used in this code. It works perfectly so I saw
no need to re-invent another method

Budd's software methods works as follows:

All the standard morse code characters are stored in a 64 byte array called mySet[]. 
The position of each character in the array is determined by the binary value of its 
position within the array. 

It works like this:

The morse code is stored in 8 bit binary form using 1's and 0's. A '1' represents 
a Dit and a 0 represents a Dah. An extra '1' is used to the immediate-left of the 
actual morse code bits to flag the start of the morse sequence.

e.g. The morse code for the letter C is _ . _ . (DAH DIT DAH DIT). Using bits, with
Dah = 0 and Dit = 1, this would be represented as 0101. Add the preceeding flag bit 
and the binary value for the letter C is now 10101. Represent this as an 8 bit value 
and its 00010101. The decimal value of 00010101 is 21. If you count through the 
characters in the 'mySet[] array in the program, you will see that the character C 
is in position 21.
   
So, all the program needs to do is scan the array 'mySet[]' for the desired character, 
store the array position in the array counter 'morseNum'.
   
Once the program has the array-counter value in 'morseNum', it rotates the bits to the 
LEFT one bit at a time while reading the bit-value at bit 7. The first '1'that is found 
(the flag bit) is discarded but all subsequent '1's and '0's are processed 
as Dits and Dahs, until all 8 bits of 'morseNum' have been processed.

(NOTE: Binary values not associated with a valid morse character are represented
in the table by a '#' character). 
  
The table below shows a part of the 'mySet[]' array contents with 
NUM (the position of the character in the array), 
BINARY (the binary value of NUM)  
CHAR (the character associated with that location) and
MORSE (the code extracted using the binary bits). In the BINARY column, the code bits 
have been separated from the Start bit for visual clarity.

NUM	BINARY		CHAR	MORSE

 00	00000000 	# 
 
 01	00000001 	#
 
 02	0000001 0	T	_
 
 03	0000001 1	E	.
 
 04	000001 00	M	_ _
 
 05	000001 01	N	_ .
 
 06	000001 10	A	. _
 
 07	000001 11	I	. .
 
 08	00001 000	O	_ _ _
 
	. . . etc . . .
	
 62	001 11110	4	. . . . _
 
 63	001 11111	5	. . . . .
 
The second code section borrowed from "WB7FHC's Simple Morse Code Decoder is the 
'printPunctuation' function. This provies additional values for 'morseNum' that 
represent special punctuation characters. Due to the much larger decimal values 
required to represent these characters as morse code, they could not be included
in the 64 character 'mySet[] array so they are processed separately. The code
checks the keyboard input for these higher values and processes them through
the 'printPunctuation' function.

COMMAND CHARACTERS

Morse Speed

The following command characters are used to control the morse speed. Note that these 
two characters are on the same key (net to the '1' key) with the Morse-Up character 
'~' being the shifted version of the Morse Down character '`'.

`  Morse Speed DOWN

~  Morse Speed UP

After typing the character, press the <ENTER> key to activate the cange. For example, 
to increase the morse speed by 3 steps you must press ~ followed by <ENTER> three 
times. At each step the new morse speed will be displayed on the monitor.

Preset Buffers

The following command characters will send the text stored in a preset buffer. You may 
alter the content of the buffers if you wish. Make sure you press the <ENTER> key after 
typing the command character. 

You may queue several messages (e.g. press ']' <ENTER> followed by '\' <ENTER> to sent 
the NAME message followed immediately by the QTH message. Both messages will be 
transfered to the buffer in the order you selected them and the second message 
will be sent automatically once the first message has been completed.
NOTE: You should limit message queueing to no more than 2 preset buffers as the input 
buffer has a limited size. If you overload it any excess characters will be lost.

For the SIGNAL message simply type } <ENTER> then follow it with the RST:
e.g.  Type '}' <ENTER> 599 599 <ENTER> to send "YR RST 599 599"

[  Buffer #1 - CQ_Mess[] = "CQ CQ CQ DE VK2IDL VK2IDL K "

]  Buffer #2 - NAME_Mess[] = "NAME IS IAN IAN. "

\  Buffer #3 - QTH_Mess[] = "QTH IS PORT MACQUARIE. "

{  Buffer #4 - SIGNAL_Mess[]="YR RST "

}  Buffer #5 - ANT_Mess[] = "ANTENNA IS DIAMOND W8010 TRAP DIPOLE. "

|  Buffer #6 - CQTest_Mess[] = "CQTEST CQTEST CQTEST DE VK2IDL. "
