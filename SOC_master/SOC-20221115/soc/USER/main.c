#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "lcd.h"
#include "key.h"
#include "can.h"
#include "beep.h"
#include "lcd_img.h"
#include "touch.h"
#include "string.h"
// #include <time.h>

// #pragma module_name = "?time"

typedef enum
{
	Criterion_70kWh = 1,
	Criterion_84kWh,
	Criterion_100kWh,
	Criterion_75kWh_A_B,
	Criterion_ALPS90Kwh,
	Criterion_ALPS60Kwh_A_B
} Criterion_numb;     // 电池 id枚举

typedef struct
{
	int Tx_ID, Rx_ID, DID_High, DID_Low;
	int Vaild_byte_length;
	unsigned char data[100];

} UDS_Para;      //电池id读取结构体缓冲区

typedef struct            
{
	float ACell_Voltage_Low,
		ACell_Voltage_High, Safety_Cell_Voltage_Low,
		Safety_Cell_Voltage_High, delta_Cell_Voltage_Max;

} Threshold;     //电源各参数阈值结构体

#define SOC_ID_SIZE   7    //电池型号的数量

#define SOC_ID_LONG   39    //电池型号的长度

#define SOC_ID_LONG_true   9   //电池型号的真是长度

#define X 20 //起始宽度

#define Y 230 //数据起始宽度                第几行第i列高度    ： h =  X1 + Y1 *  i

#define X1 80 //起始高度

#define Y1 36 //每行间隔

#define BMSInsulation_Value_min 8000

#define Temperature_Max 55

#define delta_Temperature_Max 10
CanRxMsg RxMessage1; //接受结构体

char str[30];

int loopi = 0;
CanRxMsg RxMessage;
UDS_Para para1;
// extern const u8 gImage_nio[];
float BMSSOC;

float BMSPackVoltage,
	BMSCellVoltgMax, BMSCellVoltgMin,
	CellVolAage, deltaCellVol,
	BMSTempAverage;

u16 BMSInsulation_Value;

u16 BMSCellVoltgMax_H, BMSCellVoltgMax_L,
	BMSCellVoltgMin_H, BMSCellVoltgMin_L,

	BMSPackVoltage_H, BMSPackVoltage_L,
	BMSInsulation_Value_H, BMSInsulation_Value_L,
	BMSTempAverage_D,
	Ah_charged_H, Ah_charged_L, Ah_charged_value,
	Ah_Discharged_H, Ah_Discharged_L, Ah_Discharged_value,
	Ah_Fastcharge_H, Ah_Fastcharge_L, Ah_Fastcharge_value;

void SOC_start(u8 i, char *SOC_ID) // SOC信息显示界面
{
	LCD_Clear(WHITE);
	LCD_DrawLine(10, 10, 10, 470);
	LCD_DrawLine(10, 10, 310, 10);
	LCD_DrawLine(10, 470, 310, 470);
	LCD_DrawLine(310, 10, 310, 470);
	sprintf(str, "SOC_info:     %s", SOC_ID); // BMSSOC: SOC地址
	LCD_ShowString(25, 25, 300, 20, 24, str);
	LCD_DrawLine(10, 65, 310, 65);

	LCD_DrawPicture(12, 340, 288, 120, gImage_nio);
	//		LCD_ShowString(X,80,200,16,24,(uint8_t *)"BMSSOC:");
	//		LCD_ShowString(X,120,200,16,24,(uint8_t *)"BMSVoltage:");
	LCD_ShowString(X, X1, 200, 16, 24, (uint8_t *)"CellVolAage:");
	LCD_ShowString(X, X1 + Y1, 200, 16, 24, (uint8_t *)"deltaCellVol:");
	LCD_ShowString(X, X1 + Y1 * 2, 200, 16, 24, (uint8_t *)"BMSInsulation:");
	LCD_ShowString(X, X1 + Y1 * 3, 200, 16, 24, (uint8_t *)"CellVolMax:");
	LCD_ShowString(X, X1 + Y1 * 4, 200, 16, 24, (uint8_t *)"CellVolMin:");
	LCD_ShowString(X, X1 + Y1 * 5, 200, 16, 24, (uint8_t *)"TempAverage:");
	LCD_ShowString(X, X1 + Y1 * 6, 200, 16, 24, (uint8_t *)"BMSSOC:");
	LCD_ShowString(X, X1 + Y1 * 7, 200, 16, 24, (uint8_t *)"BMSVoltage:");
	//		LCD_DrawPicture(24,120,33,25,gImage_warn);
	//  	BEEP = 1;
}

void HexToAscii(uint8_t *src, char *dest, int len)    //十六进制转字符串
{
	char dh,dl;  //字符串的高位和低位
	int i;
	if(src == NULL || dest == NULL)
	{
			printf("src or dest is NULL\n");
			return;
	}
	if(len < 1)
	{
			printf("length is NULL\n");
			return;
	}
	for(i = 0; i < len; i++)
	{
			dh = '0' + src[i] / 16;
			dl = '0' + src[i] % 16;
			if(dh > '9')
			{
					dh = dh - '9' - 1 + 'A'; // 或者 dh= dh+ 7;
			}
			if(dl > '9')
			{
					dl = dl - '9' - 1 + 'A'; // 或者dl = dl + 7;
			}
			dest[2*i] = dh;
			dest[2*i+1] = dl;
	}
	dest[2*i] = '\0';
}

float can_sig_analy(uint8_t u8_sig_start, uint8_t u8_len, float factor, float offset, uint8_t data[8]) // can报文运算操作
{
	uint8_t lsbbit = 0;	 // LSB所在bit位
	uint8_t lsbbyte = 0; //低字节所在位置
	uint8_t msbbyte = 0; //高字节所在位置
	uint8_t start = 0;	 //起始位置
	uint8_t length = 0;	 //信号长度
	uint8_t index = 0;
	uint32_t data_merge = 0, data_value = 0;
	float Scaling_value = 0;

	start = u8_sig_start; //信号起始位
	length = u8_len;	  //信号有效数据长度
	lsbbit = start & 7;	  //获取LSB所在bit位
	lsbbyte = start >> 3; //获取低字节所在位置

	msbbyte = lsbbyte - ((lsbbit + length - 1) >> 3); //获取高字节所在位置

	//合并高低字节数据
	for (index = msbbyte; index < (lsbbyte + 1); index++)
	{
		data_merge += data[index] << ((lsbbyte - index) << 3);
	}
	data_value = data_merge >> lsbbit;			   //去尾
	data_value = data_value & ((1 << length) - 1); //按位与（只保留有效数据）
	Scaling_value = data_value * factor + offset;
	//printf("Data : %f \r\n", Scaling_value);
	return Scaling_value;
}

void SOC_ID_read(char *SOC_ID) //自动读取电池id
{
	char first_str[8];
	int nbytes;
	
	CanTxMsg Txmessage;
	Txmessage.StdId = para1.Tx_ID;
	Txmessage.DLC = 8;
	Txmessage.Data[0] = 0x03;
	Txmessage.Data[1] = 0x22;
	Txmessage.Data[2] = para1.DID_High;
	Txmessage.Data[3] = para1.DID_Low;
	Txmessage.Data[4] = 0x00;
	Txmessage.Data[5] = 0x00;
	Txmessage.Data[6] = 0x00;
	Txmessage.Data[7] = 0x00;
	CAN_Transmit(CAN1, &Txmessage);
	delay_ms(200);
	CAN_Receive(CAN1, CAN_FIFO1, &RxMessage);
	// nbytes = CAN_MessagePending(CAN1,CAN_FIFO0);
	// if (nbytes == 0)
	// {
	// 	LCD_ShowString(10, 25, 200, 20, 24, "Send Error frame[0]!");
	// }

	HexToAscii(RxMessage.Data, first_str, 8);
	LCD_ShowString(10, 10, 200, 20, 24, first_str);
	//delay_ms(2000);
	para1.Vaild_byte_length = RxMessage.Data[1];
	para1.data[0] = RxMessage.Data[5];
	para1.data[1] = RxMessage.Data[6];
	para1.data[2] = RxMessage.Data[7];
	
	delay_ms(200);
	
	int remainder = (para1.Vaild_byte_length - 3) % 7;
	int multmessagelength = (para1.Vaild_byte_length - 3) / 7;
	
	Txmessage.StdId = para1.Tx_ID;
	Txmessage.DLC = 8;
	Txmessage.Data[0] = 0x30;
	Txmessage.Data[1] = 0x00;
	Txmessage.Data[2] = 0xAA;
	Txmessage.Data[3] = 0xAA;
	Txmessage.Data[4] = 0xAA;
	Txmessage.Data[5] = 0xAA;
	Txmessage.Data[6] = 0xAA;
	Txmessage.Data[7] = 0xAA;
	CAN_Transmit(CAN1, &Txmessage);
	// delay_ms(2000);
	CAN_ITConfig(CAN1, CAN_IT_FMP1, ENABLE);

	// time_t start_t, end_t;
	// time(&start_t);
	
	while (loopi < multmessagelength)
	{
		// nbytes = read(s, &frame, sizeof(frame));
		// time(&end_t);
		// diff_T = difftime(end_t,start_t);
		if(CAN_MessagePending(CAN1,CAN_FIFO1) == 0)
			continue;

		CAN_Receive(CAN1, CAN_FIFO1, &RxMessage);

		// nbytes = CAN_MessagePending(CAN1,CAN_FIFO0);

		// if (nbytes > 0)
		// {
		for (int i = 1; i < 8; i++)
		{

			// CAN_Read_Message[loopi][i+1] = frame.data[i] ;
			para1.data[(i + (loopi * 7 + 2))] = RxMessage.Data[i];
			// printf("index: %d   ",(i+(loopi*7+2)));
			// printf("framedata: %x\r\n",frame.data[i]);
		}
		loopi++;
		// }
		// delay_ms(10);
	}
	
	para1.data[8]='\0';
	//return para1;
	//sprintf(SOC_ID, "%c%c%c%c%c%c%c%c%c%c",para1.data[0], para1.data[1], para1.data[2], para1.data[3], para1.data[4], para1.data[5], para1.data[6], para1.data[7], para1.data[8], para1.data[9]);
	sprintf(SOC_ID,"%s",para1.data);
	//HexToAscii(para1.data, SOC_ID, 30);
	// LCD_ShowString(X, 40, 200, 16, 24, (uint8_t *)para1.data);
	LCD_ShowString(X, X1, 200, 16, 24, (uint8_t *)SOC_ID);
	//delay_ms(5000);
	
}
#if CAN1_RX0_INT_ENABLE
void CAN1_RX1_IRQHandler(void)
{
	// CanRxMsg RxMessage;
	CAN_Receive(CAN1, CAN_FIFO1, &RxMessage);
	for (int i = 1; i < 8; i++)
	{

		// CAN_Read_Message[loopi][i+1] = frame.data[i] ;
		para1.data[(i + (loopi * 7 + 2))] = RxMessage.Data[i];
		// printf("index: %d   ",(i+(loopi*7+2)));
		// printf("framedata: %x\r\n",frame.data[i]);
	}
	loopi++;
	
}
#endif

void SOC_Threshold_init(Threshold *threshold) //阈值设定
{
	threshold[0].ACell_Voltage_High = 4.3;
	threshold[0].ACell_Voltage_Low = 3.4;
	threshold[0].Safety_Cell_Voltage_Low = 2.5;
	threshold[0].Safety_Cell_Voltage_High = 4.3;
	threshold[0].delta_Cell_Voltage_Max = 0.08;
	
	threshold[Criterion_70kWh].ACell_Voltage_High = 4.3;
	threshold[Criterion_70kWh].ACell_Voltage_Low = 3.4;
	threshold[Criterion_70kWh].Safety_Cell_Voltage_Low = 2.5;
	threshold[Criterion_70kWh].Safety_Cell_Voltage_High = 4.3;
	threshold[Criterion_70kWh].delta_Cell_Voltage_Max = 0.08;

	threshold[Criterion_84kWh].ACell_Voltage_High = 4.2;
	threshold[Criterion_84kWh].ACell_Voltage_Low = 3.2;
	threshold[Criterion_84kWh].Safety_Cell_Voltage_Low = 2.5;
	threshold[Criterion_84kWh].Safety_Cell_Voltage_High = 4.2;
	threshold[Criterion_84kWh].delta_Cell_Voltage_Max = 0.08;

	threshold[Criterion_100kWh].ACell_Voltage_High = 4.35;
	threshold[Criterion_100kWh].ACell_Voltage_Low = 3.4;
	threshold[Criterion_100kWh].Safety_Cell_Voltage_High = 4.35;
	threshold[Criterion_100kWh].Safety_Cell_Voltage_Low = 2.5;
	threshold[Criterion_100kWh].delta_Cell_Voltage_Max = 0.06;

	threshold[Criterion_ALPS90Kwh].ACell_Voltage_High = 4.2;
	threshold[Criterion_ALPS90Kwh].ACell_Voltage_Low = 2.8;
	threshold[Criterion_ALPS90Kwh].Safety_Cell_Voltage_High = 4.35;
	threshold[Criterion_ALPS90Kwh].Safety_Cell_Voltage_Low = 2.0;
	threshold[Criterion_ALPS90Kwh].delta_Cell_Voltage_Max = 0.08;

}
void can_test(u8 i, Threshold *threshold) // can通道接收处理
{
	CAN_Receive(CAN1, CAN_FIFO0, &RxMessage1); //接收FIFO0中的数据
	if (RxMessage1.DLC > 0)					   //收到数据才显示
	{
		//SOC
		if(RxMessage1.StdId == 0xAC)
		{
			BMSSOC = RxMessage1.Data[4] * 0.5;

			BMSPackVoltage_H = RxMessage1.Data[2] & 0x1F;//去除高3位
			BMSPackVoltage_L = RxMessage1.Data[3];
			BMSPackVoltage_H = BMSPackVoltage_H << 8;		//左移8位
			BMSPackVoltage = (BMSPackVoltage_H | BMSPackVoltage_L)*0.1;

			sprintf(str,"%3.1f%%",BMSSOC);	//BMSSOC: SOC地址
			LCD_ShowString(Y, X1 + Y1 * 6, 200, 16, 24, (uint8_t *)str);
			// if(BMSSOC >= || BMSSOC <= )
			// {
			// 	BEEP = 1;
			// 	LCD_DrawPicture(24,80,33,25,gImage_warn);
			// }

			sprintf(str,"%4.1fV",BMSPackVoltage);		//BMSVoltage:电压显示
			LCD_ShowString(Y, X1 + Y1 * 7, 200, 16, 24, (uint8_t *)str);
			// if(BMSPackVoltage >= || BMSPackVoltage <= )
			// {
			// 	BEEP = 1;
			// 	LCD_DrawPicture(24,120,33,25,gImage_warn);
			// }
		}

		// BMSInsulation_Value	绝缘阻值
		if (RxMessage1.StdId == 0x268)
		{
			BMSInsulation_Value_H = RxMessage1.Data[6];
			BMSInsulation_Value_L = RxMessage1.Data[7];
			BMSInsulation_Value = ((BMSInsulation_Value_H << 8) | BMSInsulation_Value_L);

			sprintf(str, "%dK", BMSInsulation_Value);
			if (BMSInsulation_Value <= BMSInsulation_Value_min)
			{
				BEEP = 1;
				POINT_COLOR = RED;
				LCD_ShowString(Y, X1 + Y1 * 2, 200, 16, 24, (uint8_t *)str);
				POINT_COLOR = BLACK;
			}
			else
			{
				LCD_ShowString(Y, X1 + Y1 * 2, 200, 16, 24, (uint8_t *)str);
			}
		}
		// BMSCellVoltg
		if (RxMessage1.StdId == 0x269)
		{
			BMSCellVoltgMax_H = RxMessage1.Data[0];
			BMSCellVoltgMax_L = RxMessage1.Data[1];
			BMSCellVoltgMax = ((BMSCellVoltgMax_H << 8) | BMSCellVoltgMax_L) * 0.001;

			BMSCellVoltgMin_H = RxMessage1.Data[2];
			BMSCellVoltgMin_H = BMSCellVoltgMin_H << 8;
			BMSCellVoltgMin_L = RxMessage1.Data[3];
			BMSCellVoltgMin = (BMSCellVoltgMin_H | BMSCellVoltgMin_L) * 0.001;

			CellVolAage = (BMSCellVoltgMax + BMSCellVoltgMin) / 2.0;
			sprintf(str, "%2.3fV", CellVolAage); // CellVolAvergae:

			if (CellVolAage >= threshold[i].ACell_Voltage_High || CellVolAage <= threshold[i].ACell_Voltage_Low)
			{
				BEEP = 1;
				POINT_COLOR = RED;
				LCD_ShowString(Y, X1, 200, 16, 24, (uint8_t *)str);
				POINT_COLOR = BLACK;
			}
			else
			{
				LCD_ShowString(Y, X1, 200, 16, 24, (uint8_t *)str);
			}

			deltaCellVol = BMSCellVoltgMax - BMSCellVoltgMin;
			sprintf(str, "%2.3fV", deltaCellVol); // deltaCellVol:MAX cell voltage - MIN cellvoltage
			LCD_ShowString(Y, X1 + Y1, 200, 16, 24, (uint8_t *)str);
			if (deltaCellVol >= threshold[i].delta_Cell_Voltage_Max)
			{
				BEEP = 1;
				POINT_COLOR = RED;
				LCD_ShowString(Y, X1 + Y1, 200, 16, 24, (uint8_t *)str);
				POINT_COLOR = BLACK;
			}
			else
			{
				LCD_ShowString(Y, X1 + Y1, 200, 16, 24, (uint8_t *)str);
			}

			sprintf(str, "%2.3fV", BMSCellVoltgMax); // CellVolMax:

			if (BMSCellVoltgMax >= threshold[i].ACell_Voltage_High || BMSCellVoltgMax <= threshold[i].ACell_Voltage_Low)
			{
				BEEP = 1;
				POINT_COLOR = RED;
				LCD_ShowString(Y, X1 + Y1 * 3, 200, 16, 24, (uint8_t *)str);
				POINT_COLOR = BLACK;
			}
			else
			{
				LCD_ShowString(Y, X1 + Y1 * 3, 200, 16, 24, (uint8_t *)str);
			}

			sprintf(str, "%2.3fV", BMSCellVoltgMin); // CellVolMin:

			if (BMSCellVoltgMin >= threshold[i].ACell_Voltage_High || BMSCellVoltgMin <= threshold[i].ACell_Voltage_Low)
			{
				BEEP = 1;
				POINT_COLOR = RED;
				LCD_ShowString(Y, X1 + Y1 * 4, 200, 16, 24, (uint8_t *)str);
				POINT_COLOR = BLACK;
			}
			else
			{
				LCD_ShowString(Y, X1 + Y1 * 4, 200, 16, 24, (uint8_t *)str);
			}
		}

		// BMSTempAverage	BMSTempAverage_D	平均温度
		if (RxMessage1.StdId == 0x373)
		{
			BMSTempAverage_D = RxMessage1.Data[2];

			BMSTempAverage = BMSTempAverage_D * 0.5 - 40;

			sprintf(str, "%3.1f", BMSTempAverage); //	BMSTempAverage:

			if (BMSTempAverage >= Temperature_Max)
			{
				BEEP = 1;
				POINT_COLOR = RED;
				LCD_ShowString(Y, X1 + Y1 * 5, 200, 16, 24, (uint8_t *)str);
				POINT_COLOR = BLACK;
			}
			else
			{
				LCD_ShowString(Y, X1 + Y1 * 5, 200, 16, 24, (uint8_t *)str);
			}
		}
	}
}


void SOC_LIST_Init(char (*SOC_ID_list)[SOC_ID_LONG])   //电池id列表的初始化
{
	strcpy(SOC_ID_list[Criterion_70kWh], "P0079340,P0210575,P0000084");
	
	strcpy(SOC_ID_list[Criterion_84kWh], "P0201877,P0085553");

	strcpy(SOC_ID_list[Criterion_100kWh], "P0205908,P0212542,P0237537,P0296681");

	strcpy(SOC_ID_list[Criterion_75kWh_A_B], "P0223981");

	strcpy(SOC_ID_list[Criterion_ALPS90Kwh], "P0273827");
}



u8 check_SOC(char *SOC_ID, char (*SOC_ID_list)[SOC_ID_LONG]) //电池型号选择
{
	for(int i = 1; i < SOC_ID_SIZE; i++)
	{
		if(strstr(SOC_ID_list[i], SOC_ID) > 0)
		return i;
	}
	
	//LCD_ShowString(400, 25, 200, 20, 24, "check no id");
	return 0;
}
//void SOC_go(void) //电池型号选择界面
//{
//	LCD_DrawPicture(0, 0, 320, 478, gImage_soc_go);
//}

int main(void) //	软件入口
{
	u16 White = 0xFFFF;
	u8 key;
	int i;
	Threshold Threshold[10];
	char SOC_ID_list[SOC_ID_SIZE][SOC_ID_LONG];
	SOC_Threshold_init(Threshold);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置系统中断优先级分组2
	delay_init(168);								//初始化延时函数
	uart_init(115200);								//初始化串口波特率为115200
	LED_Init();										//初始化LED
	LCD_Init();										// LCD初始化
	tp_dev.init();									//触摸屏初始化
	KEY_Init();										//按键初始化
	SOC_LIST_Init(SOC_ID_list);                     //电池id列表的初始化
	// BEEP_Init();
	CAN1_Mode_Init(CAN_SJW_1tq, CAN_BS2_6tq, CAN_BS1_7tq, 6, CAN_Mode_Normal); // CAN初始通用模式,波特率500Kbps    42/（（1+6+7）*6）=0.5M
	POINT_COLOR = BLACK;													   //
	char SOC_ID[SOC_ID_LONG_true];
	while (1)
	{
		para1.DID_High = 0x01;
		para1.DID_Low = 0x67;
		para1.Rx_ID = 0x68C;
		para1.Tx_ID =0x60C;
		SOC_ID_read(SOC_ID);
		i = check_SOC(SOC_ID,SOC_ID_list);
		delay_ms(500);
		SOC_start(i,SOC_ID);
		while (1)
		{
			key = KEY_Scan(0);
			if (key == KEY0_PRES)
			{
				BEEP = 0;
			}
			if (key == WKUP_PRES)
			{
				break;
			}
			can_test(i, Threshold);
		}

	}
}



// u8 check_SOC_1_0(char *SOC_ID, char (*SOC_ID_list)[SOC_ID_LONG]) //电池型号选择
// {
// 	u8 i;
// 	if (tp_dev.y[0] < 55)
// 	{
// 		i = Criterion_70kWh;
// 	}
// 	else if (tp_dev.y[0] < 115)
// 	{
// 		i = Criterion_84kWh;
// 	}
// 	else if (tp_dev.y[0] < 175)
// 	{
// 		i = Criterion_100kWh;
// 	}
// 	else if (tp_dev.y[0] < 235 && tp_dev.x[0] < 160)
// 	{
// 		i = Criterion_75kWh_A;
// 	}
// 	else if (tp_dev.y[0] < 235)
// 	{
// 		i = Criterion_75kWh_B;
// 	}
// 	else if (tp_dev.y[0] < 285)
// 	{
// 		i = Criterion_ALPS90Kwh;
// 	}
// 	else if (tp_dev.y[0] < 345 && tp_dev.x[0] < 160)
// 	{
// 		i = Criterion_ALPS60Kwh_A;
// 	}
// 	else if (tp_dev.y[0] < 345)
// 	{
// 		i = Criterion_ALPS60Kwh_B;
// 	}
// 	else
// 	{
// 		i = 0;
// 	}
// }