/**
 * reads the state of all 10 switches at once and returns them as one number
 *
 * @param   pBase  base address returned by mmap
 * @return  int    the value of all the switches together (0-1023)
 */
int ReadAllSwitches(char *pBase){
    return RegisterRead(pBase, SW_BASE) & 0x3FF; //ANDs with 0x3FF (1111111111) to only keep the 10 switch bits, giving 0-1023
}