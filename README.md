# Arduino-2560-In-Circuit-Tester-USB-Interface
This is a .ino file that is to be used with the code from Paul Swan's Arduino Mega ICT repository.

Currently the ICT is designed to have software uploaded to it that is game specific.  My goal is to make the software "Generic" in that it only knows about the CPU's that you want to work with it.  This should make it a lot easier to add support for future games without having to change the software on the Arduino. 

The majority of the work will be done from a Client Application (I've written one for Windows and is available <To be added...>).
  
The ICT software will communicate to the client software via USB (Serial, Baud Rate is set to 115,200).  You will send it a command and it will return with a response.

For example:

You would send the command:

setCpu:6502

and the ICT will respond with "Success: CPU Set to 6502", or "Error: <error message>" if there is a problem setting up the CPU.
  
For maximum compatiblity and the fact you can use an Serial communication software such as the Arduino Serial Monitor, everything is sent via ASCII.  

Installation Instructions
... To be added

Windows Softare Download
... To be added

