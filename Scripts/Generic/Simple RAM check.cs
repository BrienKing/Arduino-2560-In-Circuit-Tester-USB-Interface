// Script Created on 8/4/2019 1:24 PM
bool CheckRamSimple(int p_startAddress, int p_length)
{
    byte[] buffer = null;
 
    Tester.WriteMemory(p_startAddress,p_length,00);
    buffer = Tester.ReadMemory(p_startAddress, p_length);
    if (VerifyMemoryRead(buffer, 0) == false)
    {
        DisplayMessage("RAM Test Failed!");
        return false;
    }

    Tester.WriteMemory(p_startAddress,p_length,255);
    buffer = null;
    buffer = Tester.ReadMemory(p_startAddress, p_length);
    if (VerifyMemoryRead(buffer, 255) == false)
    {
        DisplayMessage("RAM Test Failed!");
        return false;
    }

    DisplayMessage("RAM Test Passed!");
    return true;
}

private bool VerifyMemoryRead(byte[] p_buffer, byte p_value)
{
    int memIndex = 0;
    bool passed = true;

    if (p_buffer == null)
    {
        DisplayMessage("Could not read the RAM.");
        AbortTests();
        return false;
    }
    else
    {
        memIndex = 0;
        foreach(byte mem in p_buffer)
        {
            if (mem != p_value)
            {
                LogMessage(string.Format("Memory Read Error at {0:x}/{0}.  Expected {1:x} found {2:x}", memIndex, p_value, mem)); 
                passed = false;
            }
            
            memIndex++;
        }
    }

    return passed;
}