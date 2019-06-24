//
// Copyright (c) 2019, ClassicSoft, LLC
// All rights reserved.  Developed by Brien King
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
// TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
#include <LiquidCrystal.h>
#include <Bitswap.h>
#include <CBus.h>
#include <CFast8BitBus.h>
#include <CFastPin.h>
#include <CGame.h>
#include <CGameCallback.h>
#include <CGenericBaseGame.h>
#include <CIoCheck.h>
#include <CRamCheck.h>
#include <CRomCheck.h>
#include <Error.h>
#include <ICpu.h>
#include <IGame.h>
#include <main.h>
#include <PinMap.h>
#include <Types.h>

#include <6502PinDescription.h>
#include <C6502ClockMasterCpu.h>
#include <C6502Cpu.h>

enum ResultCodes
{
	Success = 0,
	Warning = 1,
	Error = 2
};

String const VERSION = "1.0 ALPHA";
String const COMMAND_SEPARATOR = ":";
String const TERMINATOR_CHAR = "~";
String const LCD_BLANK_LINE = "                ";
static LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
static ICpu* m_currentCpu = nullptr;

#pragma region Arduino Main Startup and Looping Code

void setup() {
	lcd.begin(16, 2);
	DisplayStatusInfo("ICT USB V" + VERSION);
	Serial.begin(115200);
}

void loop() {
	char inputByte = 0;
	String tempString = "";
	String command = "";
	String parameters = "";
	int separatorIndex = -1;
	static int loopCount = 0;
	static char spinner[] = { '.', 'o', 'O', 'o' };
	static int spinnerIndex = 0;

	//////////////////////////////////////////////////////////////////////
	// Check to see if we have any serial data coming in.
	// The data we are expecting is a single line terminated with a CRLF.
	//////////////////////////////////////////////////////////////////////
	if (Serial.available() > 0)
	{
		tempString = Serial.readStringUntil('\n');
	}

	if (tempString != "")
	{
		//////////////////////////////////////////////////////////////////////
		// Commands should be sent in the form of either
		// <command>
		// or
		// <command>:<parameters>
		//
		// Check to see if we havea a multi-part command.  If so
		// then we break it apart and call the HandleCommand function
		// to process the command further.
		//
		//////////////////////////////////////////////////////////////////////
		if (tempString.indexOf(COMMAND_SEPARATOR) > -1)
		{
			separatorIndex = tempString.indexOf(COMMAND_SEPARATOR);
			if (separatorIndex >= 0)
			{
				command = tempString.substring(0, separatorIndex);
				parameters = tempString.substring(separatorIndex + 1);

				HandleCommand(command, parameters);
			}
			else
			{
				SendResponse(Error, "Unknown Command" + tempString, "");
			}
		}
		else
		{
			HandleCommand(tempString, "");
		}

	}

	//////////////////////////////////////////////////////////////////////
	// This displays a Spinning Character in the cornor to indicate
	// that the Tester is working and not locked up.
	//////////////////////////////////////////////////////////////////////
	loopCount++;
	if (loopCount < 0)
	{
		loopCount = 0;
	}

	if (loopCount % 3500 == 0)
	{
		lcd.setCursor(15, 1);
		lcd.print(spinner[spinnerIndex]);

		spinnerIndex++;
		if (spinnerIndex >= sizeof(spinner))
		{
			spinnerIndex = 0;
		}
	}
}

#pragma endregion

#pragma region Utility Functions

String GetVersionString()
{
	return "In-Circuit Tester USB Interface V" + VERSION;
}

// Displays Information on Line 1
void DisplayStatusInfo(String p_text)
{
	lcd.setCursor(0, 0);
	lcd.print(LCD_BLANK_LINE);
	lcd.setCursor(0, 0);
	lcd.print(p_text);
}

// Displays Information on Line 2
void DisplayCommandDetails(String p_text)
{
	lcd.setCursor(0, 1);
	lcd.print(LCD_BLANK_LINE);
	lcd.setCursor(0, 1);
	lcd.print(p_text);
}

void SendResponse(ResultCodes p_result, String p_message)
{
	SendResponse(p_result, p_message, "");
}

void SendResponse(ResultCodes p_result, String p_message, String p_extraData)
{
	String responseText = "";
	String outputString = "";

	switch (p_result)
	{
	case Success:
		responseText = "Success:";
		break;
	case Warning:
		responseText = "Warning:";
		break;
	case Error:
		responseText = "Error:";
		break;
	}

	responseText += p_message;
	if (p_extraData != "")
	{
		responseText += "|" + p_extraData;
	}

	responseText += TERMINATOR_CHAR;

	Serial.print(responseText);
	Serial.flush();
}

void SendResponse(String p_message, bool p_terminate)
{
	String responseText = p_message;

	if (p_terminate)
	{
		responseText += TERMINATOR_CHAR;
	}

	Serial.println(responseText);
	Serial.flush();

}

void SendResponse(ResultCodes p_result)
{
	String responseText = "";
	String outputString = "";

	switch (p_result)
	{
	case Success:
		responseText = "Success:";
		break;
	case Warning:
		responseText = "Warning:";
		break;
	case Error:
		responseText = "Error:";
		break;
	}

	Serial.print(responseText);
}

//
// Converts a String Value to Unsigned int.  If it starts with a 0x it's assumed a Hex Value.
//
UINT32 StringToUint32(String p_value)
{
	//////////////////////////////////////////////////////////////////////////////////
	// If the value starts with an 0x we assume it's a Hex Value and need to convert
	// it as necessary, otherwise we assume it's just a simple Decimal Value and
	// convert it that way.
	//////////////////////////////////////////////////////////////////////////////////
	if (p_value.startsWith("0x") || p_value.startsWith("0X"))
	{
		return strtoul(p_value.substring(2).c_str(), nullptr, 16);
	}

	return strtoul(p_value.c_str(), nullptr, 10);
}

UINT16 StringToUint16(String p_value)
{
	//////////////////////////////////////////////////////////////////////////////////
	// If the value starts with an 0x we assume it's a Hex Value and need to convert
	// it as necessary, otherwise we assume it's just a simple Decimal Value and
	// convert it that way.
	//////////////////////////////////////////////////////////////////////////////////
	if (p_value.startsWith("0x") || p_value.startsWith("0X"))
	{
		return strtoul(p_value.substring(2).c_str(), nullptr, 16);
	}

	return strtoul(p_value.c_str(), nullptr, 10);
}


#pragma endregion

#pragma region Main Command Handling Code

void HandleCommand(String p_command, String p_parameters)
{
	if (p_command == "")
	{
		return;
	}

	if (p_command.equalsIgnoreCase("cpu"))
	{
		SetCpu(p_parameters);
	}
	else if (p_command.equalsIgnoreCase("rm"))
	{
		ReadMemory(p_parameters);
	}
	else if (p_command.equalsIgnoreCase("wm"))
	{
		WriteMemory(p_parameters);
	}
	else if (p_command == "?" || p_command.equalsIgnoreCase("help"))
	{
		DisplayCommandDetails("REQ Help");
		DisplayHelp();
	}
	else if (p_command.equalsIgnoreCase("v") || p_command.equalsIgnoreCase("version"))
	{
		DisplayCommandDetails("REQ Version");
		SendResponse(Success, GetVersionString());
	}
	else
	{
		DisplayCommandDetails("Unknown: '" + p_command + "'");
		SendResponse(Error, "Unknown Command: " + p_command);
	}

}

#pragma endregion

#pragma region Command Handling Functions

void SetCpu(String p_parameters)
{
	PERROR cpuResult;

	if (m_currentCpu != nullptr)
	{
		delete m_currentCpu;
	}

	if (p_parameters == "6502")
	{
		m_currentCpu = new C6502Cpu(true);
		if (m_currentCpu == nullptr)
		{
			DisplayStatusInfo("Error! 6502 CPU Mode");
			SendResponse(Error, "Could not create the 6502 CPU object.");
			return;
		}

		cpuResult = m_currentCpu->idle();
		if (cpuResult->code != ERROR_SUCCESS)
		{
			DisplayCommandDetails("Error! " + cpuResult->description);
			SendResponse(Error, cpuResult->description);
			return;
		}

		cpuResult = m_currentCpu->check();
		if (cpuResult->code != ERROR_SUCCESS)
		{
			DisplayCommandDetails("Error! " + cpuResult->description);
			SendResponse(Error, cpuResult->description);
			return;
		}

		DisplayStatusInfo("6502 CPU Mode");
		SendResponse(Success, "CPU Set to 6502");
	}
	else if (p_parameters.equalsIgnoreCase("z80"))
	{
		//TODO: Create the Z80 Class
		DisplayStatusInfo("Z80 CPU Mode");
		SendResponse(Success, "CPU Set to Z80");
	}
	else if (p_parameters == "6809")
	{
		//TODO: Create the 6809 Class
		DisplayStatusInfo("6809 CPU Mode");
		SendResponse(Success, "CPU Set to 6809");
	}
	else if (p_parameters == "8088")
	{
		//TODO: Create the 8088 Class
		DisplayStatusInfo("8088 CPU Mode");
		SendResponse(Success, "CPU Set to 8088");
	}
	else if (p_parameters == "68000")
	{
		//TODO: Create the 68000 Class
		DisplayStatusInfo("68000 CPU Mode");
		SendResponse(Success, "CPU Set to 68000");
	}
	else
	{
		DisplayStatusInfo("Unknown CPU");
		SendResponse(Error, "Unsupported CPU Type" + p_parameters);
		return;
	}

	DisplayCommandDetails("Ready!");

}

void ReadMemory(String p_parameters)
{
	PERROR result;
	UINT32 address;
	UINT32 length = 0;
	UINT16 value;
	String addressPart;
	String lengthPart;
	int charIndex = 0;
	String memoryDump = "";
	char hexValue[20];

	if (m_currentCpu == nullptr)
	{
		SendResponse(Error, "CPU Not Initialized");
		DisplayCommandDetails("CPU Not Initialized");
		return;
	}

	charIndex = p_parameters.indexOf(",");
	if (charIndex == -1)
	{
		SendResponse(Error, "Invalid Read Memory Format. <address>,<length>");
		return;
	}

	addressPart = p_parameters.substring(0, charIndex);
	lengthPart = p_parameters.substring(charIndex + 1);

	address = StringToUint32(addressPart);
	length = StringToUint32(lengthPart);

	DisplayCommandDetails("RM: " + addressPart + "," + lengthPart);

	//////////////////////////////////////////////////////////////////
	// This is an optimistic approach.  We are saying the operation
	// was successful, when in fact there could be a failure 
	// with the call the the memoryRead() method.
	//
	// If for some reason the read does fail, the error will be
	// returned as part of the result.  This means that you might
	// get some valid data and then an error message tacked on
	// the end.  It is therefore recommended that the client app
	// checks the result to see if it contains "Error:" in the result
	// before processing it.
	//////////////////////////////////////////////////////////////////
	SendResponse(Success);
	SendResponse(addressPart + "|", false);

	for (int i = 0; i < length; i++)
	{
		result = m_currentCpu->memoryRead(address, &value);
		if (result->code != ERROR_SUCCESS)
		{
			SendResponse(Error, result->description);
			DisplayCommandDetails(result->description);
			return;
		}

		sprintf(hexValue, "%02X", value);
		memoryDump += hexValue;

		/////////////////////////////////////////////////////////////////
		// We do this because memory is limited and we need to make
		// sure all the data gets sent back.
		/////////////////////////////////////////////////////////////////
		if (memoryDump.length() > 500)
		{
			SendResponse(memoryDump, false);
			memoryDump = "";
		}
	}

	if (memoryDump.length() > 0)
	{
		SendResponse(memoryDump, true);
		memoryDump = "";
	}
	else
	{
		SendResponse("", true);
	}
}

void WriteMemory(String p_parameters)
{
	PERROR result;
	UINT32 address;
	UINT32 length = 0;
	UINT16 value;
	String addressPart;
	String lengthPart;
	String valuePart;
	int firstCharIndex = 0;
	int secondCharIndex = 0;
	int bytesWritten = 0;

	if (m_currentCpu == nullptr)
	{
		SendResponse(Error, "CPU Not Initialized");
		DisplayCommandDetails("CPU Not Initialized");
		return;
	}

	firstCharIndex = p_parameters.indexOf(",");
	if (firstCharIndex == -1)
	{
		SendResponse(Error, "Invalid Write Memory Format. <address>,<length>,<value>");
		return;
	}

	secondCharIndex = p_parameters.indexOf(",", firstCharIndex + 1);
	if (secondCharIndex == -1)
	{
		SendResponse(Error, "Invalid Write Memory Format. <address>,<length>,<value>");
		return;
	}

	addressPart = p_parameters.substring(0, firstCharIndex);
	lengthPart = p_parameters.substring(firstCharIndex + 1, secondCharIndex - firstCharIndex + 1);
	valuePart = p_parameters.substring(secondCharIndex + 1);

	address = StringToUint32(addressPart);
	length = StringToUint32(lengthPart);
	value = StringToUint16(valuePart);

	//TODO: Add sanity checks here to make sure the values are within the address range of the CPU and that the length doesn't exceed the address space.

	DisplayCommandDetails("WM: " + addressPart + "-" + lengthPart + "-" + valuePart);

	for (int i = 0; i < length; i++)
	{
		result = m_currentCpu->memoryWrite(address, value);
		if (result->code != ERROR_SUCCESS)
		{
			SendResponse(Error, result->description);
			DisplayCommandDetails(result->description);
			return;
		}

		bytesWritten++;
		address++;
	}

	SendResponse(Success, "Data Written");
}

void WriteMemoryContinuously(String p_parameters)
{

}

void DisplayHelp()
{
	Serial.println("");
	Serial.println("--------------------------------------------------");
	Serial.println("In Circuit Tester USB Interface " + VERSION + " Help");
	Serial.println("--------------------------------------------------");
	Serial.println("This wouldn't be possible without the excellent work from Paul Swan");
	Serial.println("https://github.com/prswan/arduino-mega-ict");
	Serial.println("and from the efforts of many people on the UkVAC Forums here");
	Serial.println("http://ukvac.com/forum/topic349525.html");
	Serial.println("");
	Serial.println("");
	Serial.println("All commands are separated by a " + COMMAND_SEPARATOR + ".");
	Serial.println("Parameters follow the command.");
	Serial.println("");
	Serial.println("For Example:");
	Serial.println("cpu:z80");
	Serial.println("");
	Serial.println("This would set the tester in Z80 Mode");
	Serial.println("");
	Serial.println("When you send a command you will get one of the following ");
	Serial.println("");
	Serial.println("Success: <Message>|<data>");
	Serial.println("Warning: <Message>");
	Serial.println("Error: <Message>");
	Serial.println("");
	Serial.println("Parameter values are assumed Decimal unless prefixed with an x.");
	Serial.println("");
	Serial.println("--------------------------------------------------");
	Serial.println("Commands Available");
	Serial.println("--------------------------------------------------");
	Serial.println("");
	Serial.println("Setting the CPU Mode");
	Serial.println("  cpu:<z80,6502,8088,68000>");
	Serial.println("");
	Serial.println("  Example: cpu:z80");
	Serial.println("");
	Serial.println("Reading Memory");
	Serial.println("  rm:<start>,<length>");
	Serial.println("");
	Serial.println("    Example: rm:1000,10");
	Serial.println("    Reads 10 bytes from Address 1000 (decimal)");
	Serial.println("");
	Serial.println("    Example: rm:0x38E,0x0A");
	Serial.println("    Reads 0x0A (10 decimal) bytes from Address 0x38E (1000 decimal)");
	Serial.println("");
	Serial.println("  Returns what it read as Hex Values.  Each 2 characters is one BYTE in Hex");
	Serial.println("  Example: Success: 010FAB00000107FFFE");
	Serial.println("");
	Serial.println("Write Memory");
	Serial.println("  wm:<address>,<length>,<values>");
	Serial.println("");
	Serial.println("  Example: wm:1000,3,0x10");
	Serial.println("");
	Serial.println("  Writes 0x10 @ Address 1000, 1001, and 1002");
	Serial.println("");
	Serial.println("Write Memory Multiples");
	Serial.println("  wmm:<address>,<values>");
	Serial.println("");
	Serial.println("  Example: wmm:1000,0x10,0x20,0xFF");
	Serial.println("");
	Serial.println("  Writes 0x10 @ Address 1000, 0x20 @ Address 1001, and 0xFF at Address 1002");
	Serial.println("");
	Serial.println("Write Memory Continuously");
	Serial.println("  wmc:<address>,<value>,<time>");
	Serial.println("");
	Serial.println("  <address> is where to write");
	Serial.println("  <value> is the value to write");
	Serial.println("  <time> is how long to write that value");
	Serial.println("");
	Serial.println("  Example: wmc:0x38e,0xFF,1000");
	Serial.println("  Writes the value 0xFF to address 0x38e for the next 1000 Milliseconds.");
	Serial.println("");
	Serial.println("--------------------------------------------------");
	Serial.println("End of Help");
	Serial.println("--------------------------------------------------");
	Serial.print(TERMINATOR_CHAR);
}

#pragma endregion
