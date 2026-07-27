 // ADC???? + ??I2C1??OLED??????

#define PERIPH_BASE                ((unsigned int)0x40000000)
#define APB1PERIPH_BASE              PERIPH_BASE
#define APB2PERIPH_BASE             (PERIPH_BASE + 0x10000)
#define RCC_BASE                    ((unsigned int)0x40021000)
#define GPIOA_BASE                  (APB2PERIPH_BASE + 0x0000)
#define GPIOB_BASE                  (APB2PERIPH_BASE + 0x0C00)
#define GPIOC_BASE                  (APB2PERIPH_BASE + 0x1000)
#define ADC1_BASE                   (APB2PERIPH_BASE + 0x2400)
#define I2C1_BASE                   (APB1PERIPH_BASE + 0x5400)

#define RCC_CFGR                    (*(volatile unsigned int *)(RCC_BASE + 0x04))
#define RCC_APB1ENR                 (*(volatile unsigned int *)(RCC_BASE + 0x1C))
#define RCC_APB2ENR                 (*(volatile unsigned int *)(RCC_BASE + 0x18))

#define GPIOA_CRL                   (*(volatile unsigned int *)(GPIOA_BASE + 0x00))
#define GPIOB_CRL                   (*(volatile unsigned int *)(GPIOB_BASE + 0x00))
#define GPIOC_CRH                   (*(volatile unsigned int *)(GPIOC_BASE + 0x04))
#define GPIOC_BSRR                  (*(volatile unsigned int *)(GPIOC_BASE + 0x10))
#define GPIOC_BRR                   (*(volatile unsigned int *)(GPIOC_BASE + 0x14))

#define ADC1_SR                     (*(volatile unsigned int *)(ADC1_BASE + 0x00))
#define ADC1_CR2                    (*(volatile unsigned int *)(ADC1_BASE + 0x08))
#define ADC1_SMPR2                  (*(volatile unsigned int *)(ADC1_BASE + 0x14))
#define ADC1_SQR3                   (*(volatile unsigned int *)(ADC1_BASE + 0x34))
#define ADC1_DR                     (*(volatile unsigned int *)(ADC1_BASE + 0x4C))

#define I2C1_CR1                    (*(volatile unsigned int *)(I2C1_BASE + 0x00))
#define I2C1_CR2                    (*(volatile unsigned int *)(I2C1_BASE + 0x04))
#define I2C1_SR1                    (*(volatile unsigned int *)(I2C1_BASE + 0x14))
#define I2C1_SR2                    (*(volatile unsigned int *)(I2C1_BASE + 0x18))
#define I2C1_CCR                    (*(volatile unsigned int *)(I2C1_BASE + 0x1C))
#define I2C1_TRISE                  (*(volatile unsigned int *)(I2C1_BASE + 0x20))
#define I2C1_DR                     (*(volatile unsigned int *)(I2C1_BASE + 0x10))

#define OLED_ADDR 0x78

void Delay(volatile unsigned int count)
{
    while(count--);
}

void I2C1_Init(void)
{
    // ?? GPIOB(?3?) ? AFIO????(?0?) ??
    RCC_APB2ENR |= (1 << 3) | (1 << 0);
    // ?? I2C1(?21?) ??
    RCC_APB1ENR |= (1 << 21);

    // ?? PB6(SCL) ? PB7(SDA)
    GPIOB_CRL &= ~(0xFF << 24);
    // 0xDD ??:??????,???? 50MHz (??I2C????????)
    GPIOB_CRL |= (0xDD << 24);

    // ??? I2C1 ??,??????
    I2C1_CR1 |= (1 << 15);
    I2C1_CR1 &= ~(1 << 15);

    // I2C ????
    I2C1_CR2 &= ~(0x3F);
    I2C1_CR2 |= 36; // 36MHz APB1??

    I2C1_CCR = 180;
    I2C1_TRISE = 37;

    // ?? I2C1
    I2C1_CR1 |= (1 << 0);
}

void I2C1_Start(void)
{
    I2C1_CR1 |= (1 << 8);
    while(!(I2C1_SR1 & (1 << 0)));
}

void I2C1_Stop(void)
{
    I2C1_CR1 |= (1 << 9);
}

void I2C1_SendAddr(unsigned char addr)
{
    I2C1_DR = addr;
    while(!(I2C1_SR1 & (1 << 1)));
    (void)I2C1_SR1;
    (void)I2C1_SR2;
}

void I2C1_SendByte(unsigned char data)
{
    while(!(I2C1_SR1 & (1 << 7)));
    I2C1_DR = data;
    while(!(I2C1_SR1 & (1 << 2)));
}

void OLED_WriteCmd(unsigned char cmd)
{
    I2C1_Start();
    I2C1_SendAddr(OLED_ADDR);
    I2C1_SendByte(0x00);
    I2C1_SendByte(cmd);
    I2C1_Stop();
}

void OLED_WriteData(unsigned char data)
{
    I2C1_Start();
    I2C1_SendAddr(OLED_ADDR);
    I2C1_SendByte(0x40);
    I2C1_SendByte(data);
    I2C1_Stop();
}

void OLED_SetPos(unsigned char x, unsigned char y)
{
    OLED_WriteCmd(0xB0 + y);
    OLED_WriteCmd(((x & 0xF0) >> 4) | 0x10);
    OLED_WriteCmd(x & 0x0F);
}

void OLED_Clear(void)
{
    unsigned char i, j;
    for(i = 0; i < 8; i++)
    {
        OLED_SetPos(0, i);
        for(j = 0; j < 128; j++)
            OLED_WriteData(0x00);
    }
}

void OLED_Init(void)
{
    Delay(500000);
    OLED_WriteCmd(0xAE);
    OLED_WriteCmd(0xD5);
    OLED_WriteCmd(0x80);
    OLED_WriteCmd(0xA8);
    OLED_WriteCmd(0x3F);
    OLED_WriteCmd(0xD3);
    OLED_WriteCmd(0x00);
    OLED_WriteCmd(0x40);
    OLED_WriteCmd(0x8D);
    OLED_WriteCmd(0x14);
    OLED_WriteCmd(0x20);
    OLED_WriteCmd(0x00);
    OLED_WriteCmd(0xA1);
    OLED_WriteCmd(0xC8);
    OLED_WriteCmd(0xDA);
    OLED_WriteCmd(0x12);
    OLED_WriteCmd(0x81);
    OLED_WriteCmd(0xCF);
    OLED_WriteCmd(0xD9);
    OLED_WriteCmd(0xF1);
    OLED_WriteCmd(0xDB);
    OLED_WriteCmd(0x40);
    OLED_WriteCmd(0xA4);
    OLED_WriteCmd(0xA6);
    OLED_WriteCmd(0xAF);
    OLED_Clear();
}

const unsigned char font_letters[][6] = {
    {0x00,0x41,0x7F,0x41,0x00,0x00}, // 0=I
    {0x7F,0x41,0x41,0x41,0x3E,0x00}, // 1=D
    {0x7F,0x40,0x40,0x40,0x40,0x00}, // 2=L
    {0x7F,0x49,0x49,0x49,0x41,0x00}, // 3=E
    {0x7F,0x09,0x19,0x29,0x46,0x00}, // 4=R
    {0x7F,0x09,0x09,0x09,0x06,0x00}, // 5=P
    {0x3F,0x40,0x38,0x40,0x3F,0x00}, // 6=W
    {0x7C,0x12,0x11,0x12,0x7C,0x00}, // 7=A
    {0x46,0x49,0x49,0x49,0x31,0x00}, // 8=S
    {0x7F,0x08,0x08,0x08,0x7F,0x00}, // 9=H
};

void OLED_ShowLetter(unsigned char x, unsigned char y, unsigned char idx)
{
    unsigned char i;
    OLED_SetPos(x, y);
    for(i = 0; i < 6; i++)
        OLED_WriteData(font_letters[idx][i]);
}

void OLED_ShowWord(unsigned char x, unsigned char y, unsigned char *letters)
{
    unsigned char i;
    for(i = 0; i < 4; i++)
        OLED_ShowLetter(x + i*7, y, letters[i]);
}

void ADC_Init(void)
{
    RCC_CFGR &= ~(3 << 14);
    RCC_CFGR |= (2 << 14);

    RCC_APB2ENR |= (1 << 2);
    RCC_APB2ENR |= (1 << 9);

    GPIOA_CRL &= 0xFFFFFFF0;

    ADC1_SMPR2 |= (7 << 0);
    ADC1_SQR3 = 0;
    ADC1_CR2 |= (7 << 17);
    ADC1_CR2 |= (1 << 20);
    ADC1_CR2 |= (1 << 0);
}

unsigned int ADC_Read(void)
{
    ADC1_CR2 |= (1 << 22);
    while(!(ADC1_SR & (1 << 1)));
    return ADC1_DR;
}

int main(void)
{
    unsigned int adc_value;
    unsigned char current_state = 0;
    unsigned char last_state = 0xFF; // ????????????,????????

    unsigned char word_idle[4] = {0, 1, 2, 3}; // IDLE
    unsigned char word_drip[4] = {1, 4, 0, 5}; // DRIP
    unsigned char word_wash[4] = {6, 7, 8, 9}; // WASH

    // ??? PC13 ?? LED ?
    RCC_APB2ENR |= (1 << 4);
    GPIOC_CRH &= 0xFF0FFFFF;
    GPIOC_CRH |= 0x00300000;

    I2C1_Init();
    ADC_Init();
    OLED_Init();

    while(1)
    {
        // 1. ???? ADC ???
        adc_value = ADC_Read();

        // 2. ????????????
        if(adc_value < 1350)
        {
            current_state = 0; // ?? IDLE
        }
        else if(adc_value < 2000)
        {
            current_state = 1; // ?? DRIP
        }
        else
        {
            current_state = 2; // ?? WASH
        }

        // 3. ?????????? OLED ??(?????????????)
        if(current_state != last_state)
        {
            // ??? 0 ?? 40 ???????
            OLED_SetPos(0, 0);
            {
                unsigned char k;
                for(k = 0; k < 40; k++)
                    OLED_WriteData(0x00);
            }

            // ??????????
            if(current_state == 0)
            {
                OLED_ShowWord(0, 0, word_idle);
            }
            else if(current_state == 1)
            {
                OLED_ShowWord(0, 0, word_drip);
            }
            else
            {
                OLED_ShowWord(0, 0, word_wash);
            }

            // ??????
            last_state = current_state;
        }

        // 4. ?? LED ????????????
        GPIOC_BRR = (1 << 13);
        Delay(300000);
        GPIOC_BSRR = (1 << 13);
        Delay(300000);
    }
}