// ========= CRC32 ========================================

unsigned int reg32 = 0xffffffff;         // Schieberegister
 
unsigned int crc32_bytecalc(unsigned char byte)
{
int i;
unsigned int polynom = 0xEDB88320;        // Generatorpolynom

    for (i=0; i<8; ++i)
    {
        if ((reg32&1) != (byte&1))
             reg32 = (reg32>>1)^polynom; 
        else 
             reg32 >>= 1;
        byte >>= 1;
    }
    return reg32 ^ 0xffffffff;             // inverses Ergebnis, MSB zuerst
}

unsigned int crc32_messagecalc(unsigned char *data, int len)
{
int i;

    reg32 = 0xffffffff;
    for(i=0; i<len; i++) {
        crc32_bytecalc(data[i]);        // Berechne fuer jeweils 8 Bit der Nachricht
    }
    return reg32 ^ 0xffffffff;
}
