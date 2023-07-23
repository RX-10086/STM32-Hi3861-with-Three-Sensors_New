#ifndef __BH1750_H
#define __BH1750_H 

void Single_Write_BH1750(unsigned char REG_Address);
void Cmd_Write_BH1750(char cmd);
void Start_BH1750(void);
void Read_BH1750(void);
void Convert_BH1750(void);
int TaskBH1750(void);

#endif

