#define NFCONF   (*((volatile unsigned long *)0x4E000000))    //NandFlash 配置寄存器
#define NFCONT   (*((volatile unsigned long *)0x4E000004))    //NandFlash 控制寄存器
#define NFCMMD   (*((volatile unsigned char *)0x4E000008))    //NandFlash 命令寄存器
#define NFADDR   (*((volatile unsigned char *)0x4E00000C))    //NandFlash 地址寄存器
#define NFDATA   (*((volatile unsigned char *)0x4E000010))    //NandFlash 数据寄存器
#define NFSTAT   (*((volatile unsigned char *)0x4E000020))    //NandFlash 状态寄存器

#define NAND_SECT_SIZE  2048
#define NAND_SECT_MASK  (NAND_SECT_SIZE-1)



void nand_read(unsigned int addr, unsigned char *buf, unsigned int len);
/*从flash复制代码到SDRAM
 *
 *若是从NandFlash 0地址对应于片内内存，所以可读可写
 *若是从NorFlash  0地址对应于flash 0地址，所以只可以直接读，不能直接写
 */
int isBootFromNorFlash(viod)
{
   volatile int *p = (volatile int *)0;
   int val = *p;
  *p = 0x123456;
   if(*p == 0x123456)
   {                /*可读可写 所以为NandFlash启动*/
    *p = val;       /*恢复为原来的值*/
    return 0;
   } 
   return 1;
}
void copy_code_to_sdram(unsigned char *src, unsigned char *dest, unsigned int len)
{
    int i=0;
    if(isBootFromNorFlash())
    {
        while(i < len)
        {
            dest[i] = src[i];
            i++;
        }
    }
    else
    {
        nand_read((unsigned int)src, dest, len);
    }
}

void clean_bss(void)
{
    extern int __bss_start, __bss_end;
    int *p = &__bss_start;
    for(;p < &__bss_end; p++)
        *p = 0;   
}

void nand_init(void)
{
#define TACLS   0
#define TWRPH0  1
#define TWRPH1  0
    NFCONF = (TACLS<<12)|(TWRPH0<<8)|(TWRPH1<<4);
    NFCONT = (1<<4)|(1<<1)|(1<<0);                      /*详见数据手册*/
}
/*1.片选*/
void nand_select(void)
{
    NFCONT &= ~(1<<1);
}
void nand_deselect(void)
{
    NFCONT |= (1<<1);
}

/*2.发出读命令*/
void nand_cmd(unsigned char cmd)
{
    volatile int i;
    NFCMMD = cmd;
    for(i=0; i<10; i++);    /*延时操作*/
}
/*3.发出地址 分5次*/
void nand_addr(unsigned int addr)
{
    unsigned int page  = addr / NAND_SECT_SIZE;       /*页地址*/
    unsigned int col = addr & NAND_SECT_MASK;         /*页内地址  每页2k*/
    volatile int i;
    NFADDR = col & 0xff; 
    for(i=0; i<10; i++);    /*延时操作*/
    NFADDR = (col>>8) & 0xff; 
    for(i=0; i<10; i++);    /*延时操作*/
    NFADDR = page & 0xff;
    for(i=0; i<10; i++);    /*延时操作*/
    NFADDR = (page>>8)  & 0xff;
    for(i=0; i<10; i++);    /*延时操作*/
    NFADDR = (page>>16) & 0xff;
    for(i=0; i<10; i++);    /*延时操作*/
}
/*5.判读状态*/
void nand_waite_ready(void)
{
    while(!(NFSTAT & 0X01));
}
/*6.读数据*/
unsigned char nand_data(void)
{
    return NFDATA;
}
void nand_read(unsigned int addr, unsigned char *buf, unsigned int len)
{
    int i = 0;
    int col = addr & NAND_SECT_MASK;     /*页内起始地址偏移*/
    nand_select();                       /*1.片选*/
    while(i<len)
    {
        nand_cmd(0x00);                  /*2.发出读命令 0x00*/
        nand_addr(addr);                 /*3.发出地址 分5次*/
        nand_cmd(0x30);                  /*4.发出读命令 0x00h*/
        nand_waite_ready();              /*5.判读状态*/
                                         /*6.读数据*/
        for(; (col < NAND_SECT_SIZE)&&(i < len); col++)      
        {
            buf[i] = nand_data(); 
            i++;
            addr++;
        }
        col = 0;
    }
    /*7.取消片选*/
    nand_deselect();
}






