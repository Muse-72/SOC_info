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


typedef enum 
{
	Criterion_70kWh = 1,
	Criterion_84kWh,
	Criterion_100kWh,
	Criterion_75kWh_A,
	Criterion_75kWh_B,
	Criterion_ALPS90Kwh,
	Criterion_ALPS60Kwh_A,
	Criterion_ALPS60Kwh_B
} Criterion_numb;

typedef struct
{
	float ACell_Voltage_Low,
		ACell_Voltage_High, Safety_Cell_Voltage_Low,
		Safety_Cell_Voltage_High, delta_Cell_Voltage_Max;
		
} Threshold;


#define X 20     //��ʼ���

#define Y 230    //������ʼ���                �ڼ��е�i�и߶�    �� h =  X1 + Y1 *  i	

#define X1 80    //��ʼ�߶�

#define Y1 40    //ÿ�м��

#define BMSInsulation_Value_min  8000

#define Temperature_Max   55

#define delta_Temperature_Max    10
CanRxMsg RxMessage1; //���ܽṹ��

char str[20];


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

void SOC_start(u8 i)   //SOC��Ϣ��ʾ����
{
	LCD_Clear(WHITE);
	LCD_DrawLine(10, 10, 10, 470);
	LCD_DrawLine(10, 10, 310, 10);
	LCD_DrawLine(10, 470, 310, 470);
	LCD_DrawLine(310, 10, 310, 470);
	sprintf(str, "SOC_info:%u", i); // BMSSOC: SOC��ַ
	LCD_ShowString(112, 25, 200, 20, 24, str);
	LCD_DrawLine(10, 65, 310, 65);

	LCD_DrawPicture(12, 350, 288, 120, gImage_nio);
	//		LCD_ShowString(X,80,200,16,24,(uint8_t *)"BMSSOC:");
	//		LCD_ShowString(X,120,200,16,24,(uint8_t *)"BMSVoltage:");
	LCD_ShowString(X, X1, 200, 16, 24, (uint8_t *)"CellVolAage:");
	LCD_ShowString(X, X1+Y1, 200, 16, 24, (uint8_t *)"deltaCellVol:");
	LCD_ShowString(X, X1+Y1*2, 200, 16, 24, (uint8_t *)"BMSInsulation:");
	LCD_ShowString(X, X1+Y1*3, 200, 16, 24, (uint8_t *)"CellVolMax:");
	LCD_ShowString(X, X1+Y1*4, 200, 16, 24, (uint8_t *)"CellVolMin:");
	LCD_ShowString(X, X1+Y1*5, 200, 16, 24, (uint8_t *)"TempAverage:");
	//		LCD_DrawPicture(24,120,33,25,gImage_warn);
	//  	BEEP = 1;
}

float can_sig_analy(uint8_t u8_sig_start, uint8_t u8_len, float factor, float offset, uint8_t data[8])  //can�����������
{
	uint8_t lsbbit = 0;	 // LSB����bitλ
	uint8_t lsbbyte = 0; //���ֽ�����λ��
	uint8_t msbbyte = 0; //���ֽ�����λ��
	uint8_t start = 0;	 //��ʼλ��
	uint8_t length = 0;	 //�źų���
	uint8_t index = 0;
	uint32_t data_merge = 0, data_value = 0;
	float Scaling_value = 0;

	start = u8_sig_start; //�ź���ʼλ
	length = u8_len;	  //�ź���Ч���ݳ���
	lsbbit = start & 7;	  //��ȡLSB����bitλ
	lsbbyte = start >> 3; //��ȡ���ֽ�����λ��

	msbbyte = lsbbyte - ((lsbbit + length - 1) >> 3); //��ȡ���ֽ�����λ��

	//�ϲ��ߵ��ֽ�����
	for (index = msbbyte; index < (lsbbyte + 1); index++)
	{
		data_merge += data[index] << ((lsbbyte - index) << 3);
	}
	data_value = data_merge >> lsbbit;			   //ȥβ
	data_value = data_value & ((1 << length) - 1); //��λ�루ֻ������Ч���ݣ�
	Scaling_value = data_value * factor + offset;
	printf("Data : %f \r\n", Scaling_value);
	return Scaling_value;
}

void Threshold_init(Threshold *threshold)  //��ֵ�趨
{
	threshold[1].ACell_Voltage_High = 4.3;
	threshold[1].ACell_Voltage_Low = 3.4;
	threshold[1].Safety_Cell_Voltage_Low = 2.5;
	threshold[1].Safety_Cell_Voltage_High = 4.3;
	threshold[1].delta_Cell_Voltage_Max = 0.08;
	threshold[2].ACell_Voltage_High = 4.2;
	threshold[2].ACell_Voltage_Low = 3.2;
	threshold[2].Safety_Cell_Voltage_Low = 2.5;
	threshold[2].Safety_Cell_Voltage_High = 4.2;
}
void can_test(u8 i, Threshold *threshold)   //canͨ�����մ���
{
	CAN_Receive(CAN1, CAN_FIFO0, &RxMessage1); //����FIFO0�е�����
	if (RxMessage1.DLC > 0)					   //�յ����ݲ���ʾ
	{
		// SOC
		//			if(RxMessage1.StdId == 0xAC)
		//			{
		//					BMSSOC = RxMessage1.Data[4] * 0.5;
		//
		//					BMSPackVoltage_H = RxMessage1.Data[2] & 0x1F;//ȥ����3λ
		//					BMSPackVoltage_L = RxMessage1.Data[3];
		//					BMSPackVoltage_H = BMSPackVoltage_H << 8;		//����8λ
		//					BMSPackVoltage = (BMSPackVoltage_H | BMSPackVoltage_L)*0.1;
		//
		//					sprintf(str,"BMSSOC:%3.1f%%",BMSSOC);	//BMSSOC: SOC��ַ
		//					LCD_ShowString(X,80,200,16,24,(uint8_t *)str);
		////					if(BMSSOC >= || BMSSOC <= )
		////					{
		////						BEEP = 1;
		////						LCD_DrawPicture(24,80,33,25,gImage_warn);
		////					}
		//
		//					sprintf(str,"BMSVoltage:%4.1fV",BMSPackVoltage);		//BMSVoltage:��ѹ��ʾ
		//					LCD_ShowString(X,120,200,16,24,(uint8_t *)str);
		////					if(BMSPackVoltage >= || BMSPackVoltage <= )
		////					{
		////						BEEP = 1;
		////						LCD_DrawPicture(24,120,33,25,gImage_warn);
		////					}
		//			}

		// BMSInsulation_Value	��Ե��ֵ
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
				LCD_ShowString(Y, X1+Y1*2, 200, 16, 24, (uint8_t *)str);
				POINT_COLOR = BLACK;
			}else
			{
				LCD_ShowString(Y, X1+Y1*2, 200, 16, 24, (uint8_t *)str);
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
			}else
			{
				LCD_ShowString(Y, X1, 200, 16, 24, (uint8_t *)str);
			}

			deltaCellVol = BMSCellVoltgMax - BMSCellVoltgMin;
			sprintf(str, "%2.3fV", deltaCellVol); // deltaCellVol:MAX cell voltage - MIN cellvoltage
			LCD_ShowString(Y, X1+Y1, 200, 16, 24, (uint8_t *)str);
			if (deltaCellVol >= threshold[i].delta_Cell_Voltage_Max)
			{
				BEEP = 1;
				POINT_COLOR = RED;
				LCD_ShowString(Y, X1+Y1, 200, 16, 24, (uint8_t *)str);
				POINT_COLOR = BLACK;
			}else
			{
				LCD_ShowString(Y, X1+Y1, 200, 16, 24, (uint8_t *)str);
			}


			sprintf(str, "%2.3fV", BMSCellVoltgMax); // CellVolMax:
			
			if (BMSCellVoltgMax >= threshold[i].ACell_Voltage_High || BMSCellVoltgMax <= threshold[i].ACell_Voltage_Low)
			{
				BEEP = 1;
				POINT_COLOR = RED;
				LCD_ShowString(Y, X1+Y1*3, 200, 16, 24, (uint8_t *)str);
				POINT_COLOR = BLACK;
			}else
			{
				LCD_ShowString(Y, X1+Y1*3, 200, 16, 24, (uint8_t *)str);
			}



			sprintf(str, "%2.3fV", BMSCellVoltgMin); // CellVolMin:
	
			if (BMSCellVoltgMin >=  threshold[i].ACell_Voltage_High || BMSCellVoltgMin <= threshold[i].ACell_Voltage_Low)
			{
				BEEP = 1;
				POINT_COLOR = RED;
				LCD_ShowString(Y, X1+Y1*4, 200, 16, 24, (uint8_t *)str);
				POINT_COLOR = BLACK;
			}else
			{
				LCD_ShowString(Y, X1+Y1*4, 200, 16, 24, (uint8_t *)str);
			}
		}

		// BMSTempAverage	BMSTempAverage_D	ƽ���¶�
		if (RxMessage1.StdId == 0x373)
		{
			BMSTempAverage_D = RxMessage1.Data[2];

			BMSTempAverage = BMSTempAverage_D * 0.5 - 40;

			sprintf(str, "%3.1f", BMSTempAverage); //	BMSTempAverage:
			
			if (BMSTempAverage >= Temperature_Max)
			{
				BEEP = 1;
				POINT_COLOR = RED;
				LCD_ShowString(Y, X1+Y1*5, 200, 16, 24, (uint8_t *)str);
				POINT_COLOR = BLACK;
			}else
			{
				LCD_ShowString(Y, X1+Y1*5, 200, 16, 24, (uint8_t *)str);
			}
		}
	}
}
u8 check_SOC(void)   //����ͺ�ѡ��
{
	u8 i;
	if (tp_dev.y[0] < 55)
	{
		i = Criterion_70kWh;
	}
	else if (tp_dev.y[0] < 115)
	{
		i = Criterion_84kWh;
	}
	else if (tp_dev.y[0] < 175)
	{
		i = Criterion_100kWh;
	}
	else if (tp_dev.y[0] < 235 && tp_dev.x[0] < 160)
	{
		i = Criterion_75kWh_A;
	}
	else if (tp_dev.y[0] < 235)
	{
		i = Criterion_75kWh_B;
	}
	else if (tp_dev.y[0] < 285)
	{
		i = Criterion_ALPS90Kwh;
	}
	else if (tp_dev.y[0] < 345 && tp_dev.x[0] < 160)
	{
		i = Criterion_ALPS60Kwh_A;
	}
	else if (tp_dev.y[0] < 345)
	{
		i = Criterion_ALPS60Kwh_B;
	}
	else
	{
		i = 0;
	}

	return i;
}
void SOC_go(void)    //����ͺ�ѡ�����
{
	LCD_DrawPicture(0, 0, 320, 478, gImage_soc_go);
}

int main(void)   //	������
{
	u16 White = 0xFFFF;
	u8 key;
	Threshold Threshold[10];
	Threshold_init(Threshold);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //����ϵͳ�ж����ȼ�����2
	delay_init(168);								//��ʼ����ʱ����
	uart_init(115200);								//��ʼ�����ڲ�����Ϊ115200
	LED_Init();										//��ʼ��LED
	LCD_Init();										// LCD��ʼ��
	tp_dev.init();									//��������ʼ��
	KEY_Init();										//������ʼ��
	// BEEP_Init();
	CAN1_Mode_Init(CAN_SJW_1tq, CAN_BS2_6tq, CAN_BS1_7tq, 6, CAN_Mode_Normal); // CAN��ʼͨ��ģʽ,������500Kbps    42/����1+6+7��*6��=0.5M
	POINT_COLOR = BLACK;													   //

	while (1)
	{
		SOC_go(); //ѡ���ؽ���
		tp_dev.scan(0);
		if (tp_dev.sta & TP_PRES_DOWN) //������������
		{

			if (tp_dev.x[0] < lcddev.width && tp_dev.y[0] < lcddev.height)
			{

				u8 i;
				i = check_SOC(); //ѡ����
				if (i == 0)
					continue;

				SOC_start(i);
				while (1)
				{
					key = KEY_Scan(0);
					if (key == KEY0_PRES)
					{
						BEEP = 0;
						LCD_Fill(12, 66, 60, 310, White);
					}
					if (key == WKUP_PRES)
					{
						break;
					}
					can_test(i,Threshold);
				}
			}
		}
	}
}
