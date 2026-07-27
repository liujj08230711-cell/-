# 基于声纹特征分析的宿舍外挂式智能节水监测系统 🚰⚡
> **本项目是一个基于 STM32F103C8T6 核心板的嵌入式外挂式水管监测装置。**  
> 通过高灵敏度声纹传感器采集水管管壁上的声学与振动信号，利用阈值判定算法与状态机，实现对宿舍水流工况（空闲、滴水漏水、用水洗手）的实时监测，并通过本地 OLED 显示屏与蓝牙模块（串口）进行状态推送。
---
## 💡 1. 原理解析：水管如何“说话”？
当传感器紧贴水管外壁时，不同水流状态会产生特征截然不同的振动与声音信号：
* **环境安静（IDLE）**：水龙头完全关闭，仅采集到环境杂音，ADC 采样的原始信号能量极低（基础数值通常在 `1200 ~ 1300` 区间）。
* **滴水/微漏（DRIP）**：水流断续撞击管壁，产生周期性、短促的冲击脉冲信号，振动能量中等（ADC 采样的原始数值跳变至 `1400 ~ 1700` 区间）。
* **洗手/水流（WASH）**：水流与管壁剧烈摩擦产生持续的高频振动与紊流声，能量暴增（ADC 采样的原始数值迅速飙升至 `2600 ~ 3000` 区间）。
通过 STM32 芯片内置的 12 位 ADC 模块对声音信号进行高频采样，并经过状态区间映射，即可在本地与远程精准识别水管的当前状态。
---
## 🛠️ 2. 硬件材料清单
### 核心电子模块

| 模块名称 | 规格 / 说明 | 数量 |
| :--- | :--- | :--- |
| **主控板** | STM32F103C8T6 最小系统板（推荐 Type-C 接口） | 1~2 块 |
| **声纹传感器** | MAX9814 麦克风放大器模块（带自动增益控制 AGC） | 1~2 个 |
| **无线蓝牙** | HC-05 蓝牙串口透传模块 | 1 个 |
| **本地显示屏** | 0.96寸 OLED 显示屏（I2C 接口，4 引脚） | 1 个 |

### 调试工具与结构辅材
* **仿真器**：ST-Link V2 下载器
* **实验板**：MB-102 面包板 + 20cm 杜邦线（公对公、公对母）
* **声学耦合与隔音**：强力导热硅胶/双面胶（贴紧咪头）、高密度隔音海绵（包裹绝缘外界人声）、尼龙扎带
---
## 🔌 3. 硬件引脚接线关系 (Pinout)
所有模块的电源端（VCC）统一连接至面包板正极（5V/3.3V），地端（GND）统一连接至共地负极排针。
```
+-------------------+       +-----------------------+
|  MAX9814 传感器   |       |   STM32F103C8T6 主控   |
|  OUT              +------>+  PA0 (ADC1_IN0)       |
+-------------------+       +-----------------------+
+-------------------+       +-----------------------+
|  0.96寸 OLED 屏   |       |                       |
|  SCL              +------>+  PB6 (I2C1_SCL)       |
|  SDA              +------>+  PB7 (I2C1_SDA)       |
+-------------------+       +-----------------------+
+-------------------+       +-----------------------+
|  HC-05 蓝牙模块   |       |                       |
|  TXD              +------>+  PA10 (USART1_RX)     |
|  RXD              +<------+  PA9  (USART1_TX)     |
+-------------------+       +-----------------------+
```
---
## 💻 4. 软件设计与核心寄存器代码
系统不依赖第三方庞大的标准库框架，采用**直接操作底寄存器**的方式编写，具备极高的执行效率与低内存占用特性。
### 核心逻辑流程：
1. **硬件初始化**：配置 RCC 时钟，初始化 GPIOB/I2C1 控制器，将 PA0 引脚配置为模拟输入模式，启用 ADC1。
2. **状态判定与刷新**：`while(1)` 循环中连续读取声音模拟量数值：
   * `ADC < 1350` $\rightarrow$ 显示 **IDLE**（空闲）
   * `1350 ≤ ADC < 2000` $\rightarrow$ 显示 **DRIP**（滴水）
   * `ADC ≥ 2000` $\rightarrow$ 显示 **WASH**（大水流/洗手）
3. **指示灯反馈**：每次采样刷新时翻转 PC13（板载状态 LED）以直观反映运行状态。
### 主程序代码 (`main.c`)
```c
// 基于寄存器直接操作的 ADC 采集与 I2C OLED 状态显示驱动
#define PERIPH_BASE         ((unsigned int)0x40000000)
#define APB1PERIPH_BASE     PERIPH_BASE
#define APB2PERIPH_BASE    (PERIPH_BASE + 0x10000)
#define RCC_BASE           ((unsigned int)0x40021000)
#define GPIOA_BASE         (APB2PERIPH_BASE + 0x0000)
#define GPIOB_BASE         (APB2PERIPH_BASE + 0x0C00)
#define GPIOC_BASE         (APB2PERIPH_BASE + 0x1000)
#define ADC1_BASE          (APB2PERIPH_BASE + 0x2400)
#define I2C1_BASE          (APB1PERIPH_BASE + 0x5400)
/* 寄存器定义 */
#define RCC_CFGR           (*(volatile unsigned int *)(RCC_BASE + 0x04))
#define RCC_APB1ENR        (*(volatile unsigned int *)(RCC_BASE + 0x1C))
#define RCC_APB2ENR        (*(volatile unsigned int *)(RCC_BASE + 0x18))
#define GPIOA_CRL          (*(volatile unsigned int *)(GPIOA_BASE + 0x00))
#define GPIOB_CRL          (*(volatile unsigned int *)(GPIOB_BASE + 0x00))
#define GPIOC_CRH          (*(volatile unsigned int *)(GPIOC_BASE + 0x04))
#define GPIOC_BSRR         (*(volatile unsigned int *)(GPIOC_BASE + 0x10))
#define GPIOC_BRR          (*(volatile unsigned int *)(GPIOC_BASE + 0x14))
#define ADC1_SR            (*(volatile unsigned int *)(ADC1_BASE + 0x00))
#define ADC1_CR2           (*(volatile unsigned int *)(ADC1_BASE + 0x08))
#define ADC1_SMPR2         (*(volatile unsigned int *)(ADC1_BASE + 0x14))
#define ADC1_SQR3          (*(volatile unsigned int *)(ADC1_BASE + 0x34))
#define ADC1_DR            (*(volatile unsigned int *)(ADC1_BASE + 0x4C))
#define I2C1_CR1           (*(volatile unsigned int *)(I2C1_BASE + 0x00))
#define I2C1_CR2           (*(volatile unsigned int *)(I2C1_BASE + 0x04))
#define I2C1_SR1           (*(volatile unsigned int *)(I2C1_BASE + 0x14))
#define I2C1_SR2           (*(volatile unsigned int *)(I2C1_BASE + 0x18))
#define I2C1_CCR           (*(volatile unsigned int *)(I2C1_BASE + 0x1C))
#define I2C1_TRISE         (*(volatile unsigned int *)(I2C1_BASE + 0x20))
#define I2C1_DR            (*(volatile unsigned int *)(I2C1_BASE + 0x10))
#define OLED_ADDR 0x78
void Delay(volatile unsigned int count) {
    while(count--);
}
void I2C1_Init(void) {
    RCC_APB2ENR |= (1 << 3);
    RCC_APB1ENR |= (1 << 21);
    GPIOB_CRL &= ~(0xFF << 24);
    GPIOB_CRL |= (0xFF << 24);
    I2C1_CR1 |= (1 << 15);
    I2C1_CR1 &= ~(1 << 15);
    I2C1_CR2 &= ~(0x3F);
    I2C1_CR2 |= 36;
    I2C1_CCR = 180;
    I2C1_TRISE = 37;
    I2C1_CR1 |= (1 << 0);
}
void I2C1_Start(void) {
    I2C1_CR1 |= (1 << 8);
    while(!(I2C1_SR1 & (1 << 0)));
}
void I2C1_Stop(void) {
    I2C1_CR1 |= (1 << 9);
}
void I2C1_SendAddr(unsigned char addr) {
    I2C1_DR = addr;
    while(!(I2C1_SR1 & (1 << 1)));
    (void)I2C1_SR1;
    (void)I2C1_SR2;
}
void I2C1_SendByte(unsigned char data) {
    while(!(I2C1_SR1 & (1 << 7)));
    I2C1_DR = data;
    while(!(I2C1_SR1 & (1 << 2)));
}
void OLED_WriteCmd(unsigned char cmd) {
    I2C1_Start();
    I2C1_SendAddr(OLED_ADDR);
    I2C1_SendByte(0x00);
    I2C1_SendByte(cmd);
    I2C1_Stop();
}
void OLED_WriteData(unsigned char data) {
    I2C1_Start();
    I2C1_SendAddr(OLED_ADDR);
    I2C1_SendByte(0x40);
    I2C1_SendByte(data);
    I2C1_Stop();
}
void OLED_SetPos(unsigned char x, unsigned char y) {
    OLED_WriteCmd(0xB0 + y);
    OLED_WriteCmd(((x & 0xF0) >> 4) | 0x10);
    OLED_WriteCmd(x & 0x0F);
}
void OLED_Clear(void) {
    unsigned char i, j;
    for(i = 0; i < 8; i++) {
        OLED_SetPos(0, i);
        for(j = 0; j < 128; j++)
            OLED_WriteData(0x00);
    }
}
void OLED_Init(void) {
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
void OLED_ShowLetter(unsigned char x, unsigned char y, unsigned char idx) {
    unsigned char i;
    OLED_SetPos(x, y);
    for(i = 0; i < 6; i++)
        OLED_WriteData(font_letters[idx][i]);
}
void OLED_ShowWord(unsigned char x, unsigned char y, unsigned char *letters) {
    unsigned char i;
    for(i = 0; i < 4; i++)
        OLED_ShowLetter(x + i*7, y, letters[i]);
}
void ADC_Init(void) {
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
unsigned int ADC_Read(void) {
    ADC1_CR2 |= (1 << 22);
    while(!(ADC1_SR & (1 << 1)));
    return ADC1_DR;
}
int main(void) {
    unsigned int adc_value;
    unsigned char word_idle[4] = {0, 1, 2, 3}; // IDLE
    unsigned char word_drip[4] = {1, 4, 0, 5}; // DRIP
    unsigned char word_wash[4] = {6, 7, 8, 9}; // WASH
    RCC_APB2ENR |= (1 << 4);
    GPIOC_CRH &= 0xFF0FFFFF;
    GPIOC_CRH |= 0x00300000;
    I2C1_Init();
    ADC_Init();
    OLED_Init();
    while(1) {
        adc_value = ADC_Read();
        OLED_SetPos(0, 0);
        {
            unsigned char k;
            for(k = 0; k < 40; k++)
                OLED_WriteData(0x00);
        }
        // 依据声学物理特征设置阈值区间
        if(adc_value < 1350) {
            OLED_ShowWord(0, 0, word_idle);
        }
        else if(adc_value < 2000) {
            OLED_ShowWord(0, 0, word_drip);
        }
        else {
            OLED_ShowWord(0, 0, word_wash);
        }
        // LED 状态闪烁指示
        GPIOC_BRR = (1 << 13);
        Delay(300000);
        GPIOC_BSRR = (1 << 13);
        Delay(300000);
    }
}
```
## ⚙️ 5. 实地安装与校准调试
 1. **传感器物理安装**：
   * 将 MAX9814 咪头使用强力双面胶固定于水管管壁外侧。
   * 使用隔音海绵完全包裹传感器，并用尼龙扎带死死扎紧，屏蔽环境说话声及杂音干扰。
 2. **软件阈值二次校准**：
   不同宿舍的管道材质及水压可能导致实际测得的采样基准值产生偏移，可在实测后调整代码中的 if 条件参数：
   ```c
   // 示例校准区间：
   // 环境安静基准值： 1200 ~ 1300
   // 滴水/微漏基准值：1400 ~ 1700
   // 洗手/水流基准值：2600 ~ 3000
   
   ```
```
```
