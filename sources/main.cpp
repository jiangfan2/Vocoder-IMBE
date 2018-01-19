/*
 *  ======== main.c ========
 */

#ifdef __cplusplus
extern "C"
{
#endif
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <xdc/runtime/Types.h>
#include <ti/sysbios/hal/Seconds.h>
#include <ti/sysbios/interfaces/ISeconds.h>
#include <time.h>
#include <ti/sysbios/knl/Task.h>
#include "../Debug/configPkg/package/cfg/app_pe674.h"

#include "gpio.h"
#include "psc.h"

#include "soc_OMAPL138.h"
#include "lcdkOMAPL138.h"

#ifdef __cplusplus
}
#endif

#include <stdint.h>
#include <stdlib.h>
#include "imbe_vocoder/imbe_vocoder.h"
#include "wav/wavreader.h"
#include "wav/wavwriter.h"


#define PLL0_BASE                                0x01C11000                              /*SYSTEM PLL BASE ADDRESS*/
#define SYS_BASE                                 0x01C14000

#define KICK0R                                   *(unsigned int*)(SYS_BASE + 0x038)
#define KICK1R                                   *(unsigned int*)(SYS_BASE + 0x03c)

#define CFGCHIP0                                 *(unsigned int*)(SYS_BASE + 0x17C)
#define CFGCHIP2                                 *(unsigned int*)(SYS_BASE + 0x184)
#define CFGCHIP3                                 *(unsigned int*)(SYS_BASE + 0x188)

#define PLL0_CKEN                                *(unsigned int*) (PLL0_BASE + 0x148)    /*Clock Enable Reg*/
#define PLL0_OCSEL                               *(unsigned int*) (PLL0_BASE + 0x104)    /*OBSCLK Select Register*/
#define PLL0_OSCDIV1                             *(unsigned int*) (PLL0_BASE + 0x124)    /*Oscilator Divider*/

#define PINMUX13                                 *(unsigned int*)(SYS_BASE + 0x154)     /*PINMUX13*/
#define CSL_SYSCFG_PINMUX13_PINMUX13_7_4_OBSCLK0 (0x00000001u)
#define CSL_SYSCFG_PINMUX13_PINMUX13_7_4_GP       (0x00000008u)
#define CSL_SYSCFG_PINMUX13_PINMUX13_7_4_SHIFT   (0x00000004u)

void clkoutPinEnable()
{
    KICK0R = 0x83e70b13;
    KICK1R = 0x95a4f1e0;

    // ����������� ��������� PLL0
    CFGCHIP0 &= ~(1 << 4);
    // ����������� ��������� PLL1
    CFGCHIP3 &= ~(1 << 5);

    // ��������� ������ �� ����� OBSEL
    PLL0_CKEN |= 0x03;

    // ����� ��������� ���������
    PLL0_OCSEL = 0x17;    // SYSCLK1

    // �������� �� OUTPIN
    PLL0_OSCDIV1 = 0;

    // ��������� ����� GPIO (GPIO6_14)
    PINMUX13 |= ((CSL_SYSCFG_PINMUX13_PINMUX13_7_4_OBSCLK0)
            << (CSL_SYSCFG_PINMUX13_PINMUX13_7_4_SHIFT));
    KICK0R = 0x00000000;
    KICK1R = 0x00000000;
}

void gpioPinEnable()
{
    // ��������� ����� GPIO (GPIO6_14)
    PINMUX13 |= ((CSL_SYSCFG_PINMUX13_PINMUX13_7_4_GP)
            << (CSL_SYSCFG_PINMUX13_PINMUX13_7_4_SHIFT));
}

#define IN_ENCODER_FRAME_SAMPLE_SIZE (160)
#define IN_ENCODER_FRAME_BYTE_SIZE (IN_ENCODER_FRAME_SAMPLE_SIZE*2)

#define OUT_ENCODER_FRAME_BITES_SIZE (88)
#define OUT_ENCODER_FRAME_SAMPLE_SIZE (8)

const short voc_test[88] = { 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, //12 bits
                             0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, //12 bits
                             0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, //12 bits
                             0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, //12 bits
                             1, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0,    //11 bits
                             0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0,    //11 bits
                             0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,    //11 bits
                             0, 0, 0, 0, 0, 0, 0                 //7 bits
        };

int16_t encoded_sin_frame[8] = { 0x009B, 0x0088, 0x00CC, 0x0621, 0x07B2, 0x002A, 0x004C, 0x0000 };



void testFft(imbe_vocoder & executor) {

    short x_array[2*256] = {8127, 5827, 3209, 9471, -2910, 9567, -7940, 6079, -9995, 314, -8306, -5569, -3505, -9365, 2608, -9654, 7745, -6326, 9980, -628, 8477, 5305, 3798, 9251, -2304, 9731, -7543, 6566, -9956, 941, -8639, -5036, -4086, -9127, 1997, -9799, 7333, -6800, 9921, -1253, 8793, 4762, 4371, 8994, -1688, 9856, -7115, 7026, -9877, 1564, -8938, -4484, -4652, -8852, 1378, -9905, 6891, -7247, 9823, -1874, 9075, 4201, 4927, 8702, -1066, 9943, -6660, 7459, -9759, 2181, -9202, -3914, -5198, -8543, 753, -9972, 6423, -7665, 9686, -2487, 9321, 3623, 5464, 8375, -440, 9990, -6179, 7863, -9603, 2790, -9430, -3328, -5724, -8200, 126, -9999, 5929, -8053, 9511, -3090, 9530, 3030, 5979, 8016, 188, 9998, -5673, 8235, -9409, 3387, -9620, -2730, -6228, -7824, -502, -9987, 5411, -8409, 9298, -3681, 9701, 2426, 6471, 7624, 816, 9967, -5144, 8575, -9178, 3971, -9773, -2120, -6707, -7417, -1129, -9936, 4873, -8733, 9048, -4258, 9834, 1812, 6937, 7203, 1440, 9896, -4596, 8881, -8910, 4540, -9887, -1502, -7159, -6982, -1750, -9846, 4315, -9021, 8763, -4818, 9929, 1191, 7375, 6753, 2059, 9786, -4029, 9152, -8607, 5090, -9961, -879, -7584, -6518, -2365, -9716, 3740, -9274, 8443, -5358, 9984, 565, 7785, 6277, 2669, 9637, -3446, 9387, -8271, 5621, -9997, -251, -7978, -6029, -2970, -9549, 3150, -9491, 8090, -5878, 10000, -63, 8163, 5776, 3269, 9451, -2850, 9585, -7902, 6129, -9993, 377, -8341, -5516, -3564, -9343, 2548, -9670, 7705, -6374, 9976, -691, 8510, 5252, 3856, 9227, -2243, 9745, -7501, 6613, -9950, 1004, -8671, -4982, -4144, -9101, 1935, -9811, 7290, -6845, 9913, -1316, 8823, 4707, 4428, 8966, -1626, 9867, -7071, 7071, -9867, 1626, -8966, -4428, -4707, -8823, 1316, -9913, 6845, -7290, 9811, -1935, 9101, 4144, 4982, 8671, -1004, 9950, -6613, 7501, -9745, 2243, -9227, -3856, -5252, -8510, 691, -9976, 6374, -7705, 9670, -2548, 9343, 3564, 5516, 8341, -377, 9993, -6129, 7902, -9585, 2850, -9451, -3269, -5776, -8163, 63, -10000, 5878, -8090, 9491, -3150, 9549, 2970, 6029, 7978, 251, 9997, -5621, 8271, -9387, 3446, -9637, -2669, -6277, -7785, -565, -9984, 5358, -8443, 9274, -3740, 9716, 2365, 6518, 7584, 879, 9961, -5090, 8607, -9152, 4029, -9786, -2059, -6753, -7375, -1191, -9929, 4818, -8763, 9021, -4315, 9846, 1750, 6982, 7159, 1502, 9887, -4540, 8910, -8881, 4596, -9896, -1440, -7203, -6937, -1812, -9834, 4258, -9048, 8733, -4873, 9936, 1129, 7417, 6707, 2120, 9773, -3971, 9178, -8575, 5144, -9967, -816, -7624, -6471, -2426, -9701, 3681, -9298, 8409, -5411, 9987, 502, 7824, 6228, 2730, 9620, -3387, 9409, -8235, 5673, -9998, -188, -8016, -5979, -3030, -9530, 3090, -9511, 8053, -5929, 9999, -126, 8200, 5724, 3328, 9430, -2790, 9603, -7863, 6179, -9990, 440, -8375, -5464, -3623, -9321, 2487, -9686, 7665, -6423, 9972, -753, 8543, 5198, 3914, 9202, -2181, 9759, -7459, 6660, -9943, 1066, -8702, -4927, -4201, -9075, 1874, -9823, 7247, -6891, 9905, -1378, 8852, 4652, 4484, 8938, -1564, 9877, -7026, 7115, -9856, 1688, -8994, -4371, -4762, -8793, 1253, -9921, 6800, -7333, 9799, -1997, 9127, 4086, 5036, 8639, -941, 9956, -6566, 7543, -9731, 2304, -9251, -3798, -5305, -8477, 628, -9980, 6326, -7745, 9654, -2608, 9365, 3505, 5569, 8306, -314, 9995, -6079, 7940, -9567, 2910, -9471, -3209, -5827, -8127, 0, -10000, 5827, -8127, 9471, -3209, 9567, 2910, 6079, 7940, 314, 9995, -5569, 8306 };
    short * x = (short*)memalign(8, 2*256*sizeof(short));
    memcpy(x, x_array, 2*256*sizeof(short));

    //executor.fft(x, 256, 1);


}

/*
 *  ======== taskFxn ========
 */
extern "C" void testImbeExecutionTime () {
	// testFft(imbeEncoder);
	int16_t inEncoderBuffer[3][IN_ENCODER_FRAME_SAMPLE_SIZE] = {
			{7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071,
			0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071,
			0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0,
			-7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0,
			7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0,
			-7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0,
			7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0,
			-7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0,
			7071, 10000, 7071, 0, -7071, -10000, -7071, 0},
			{7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0,
			-7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0,
			7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0,
			-7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0,
			7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0,
			-7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0,
			7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0,
			-7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0,
			7071, 10000, 7071, 0, -7071, -10000, -7071, 0},
			{7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0,
			-7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0,
			7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0,
			-7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0,
			7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0,
			-7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0,
			7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0,
			-7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0, 7071, 10000, 7071, 0, -7071, -10000, -7071, 0,
			7071, 10000, 7071, 0, -7071, -10000, -7071, 0}
	};
	int16_t outDecoderBuffer[3][IN_ENCODER_FRAME_SAMPLE_SIZE];
	int16_t outDecoderEtalonBuffer[3][IN_ENCODER_FRAME_SAMPLE_SIZE] = {
			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1,
			0, -1, 1, 1, 0, 0, 3, 1, -2, 2, -3, -8, 0, 0, 2, 9, 0, 0, 0, 7, 7, 0, 8, -9, -5, 13, 4, 4, 5, -6, 10, 11, 5, -6, -7, 6,
			-13, 13, 20, -7, 7, 0, -15, 7, 0, 0, -16, 0, 24, -8, 24, 8, 0, 0, -16, -8, 0, -8, -16, -32, -16, 0, -24, 8, -8, 0, 16,
			0, 24, 8, 0, -8, 0, 0, 8, 8, 0, -8, 0, 8, 8, 8, -8, 0, 0, 8, 0, 8, -8, -8, 8, 8, 8, 0, 0, -8, 0, 24, 8, 16, 0},
			{0, 8,
			8, 8, -8, 8, -8, -8, 24, 0, 0, 16, -24, -16, -8, -16, 0, -16, -16, 0, -8, 0, -8, -16, -8, -16, 0, 8, -8, 0, 0, 16, 8, 8,
			24, 8, 8, 0, 0, 0, -8, 8, -8, -8, 16, 0, 16, 0, 0, 0, -16, -8, 0, 0, -8, -16, -1, 0, -16, 0, -8, -16, 1, -14, 13, 1,
			-16, 0, -2, 9, -3, 0, -6, -4, 6, 3, 10, 18, 7, -12, -4, -21, 7, -8, 3, 19, 6, 5, -6, -6, -6, 0, 4, -21, -1, 9, -7, 17,
			0, -8, -8, 30, 7, -8, 0, 0, 0, -8, 8, -16, 8, 0, -8, 0, 8, 0, -16, 24, 16, -8, 8, 16, 0, -8, 8, 0, 0, 16, 0, -8, -8, 8,
			8, -8, 16, -8, 0, -8, -16, 8, 0, 0, -8, -8, -16, 16, -8, -16, 0, 8, 16, 0, 0, 32, 8, 8, 8, 0, 8, -16},
			{0, 0, -16, -16,
			0, 0, 8, -8, 24, 0, 8, -8, -8, 8, 0, 24, -16, -8, 8, -8, 8, -8, 0, 16, -24, 8, -8, 0, 8, 8, 8, 24, -16, 8, -8, 0, 16,
			-8, 8, 0, -8, 16, 16, -16, 0, 0, 8, 0, 0, -16, 0, -8, 0, 16, -8, -8, 20, 45, 72, 36, -118, -255, -161, 92, 243, -64,
			-674, -785, -344, 415, 1048, 1231, 805, 20, -646, -816, -428, 88, 471, 510, 308, 16, -4, -86, -492, -212, 189, 35, -278,
			-672, -453, -24, -44, -586, -317, 668, 913, 1047, 635, 62, -370, -383, -932, -2028, -1882, -1192, 768, 2368, 2496, 2128,
			616, -1416, -2320, -1168, 1208, 2656, 1664, -872, -3080, -3272, -608, 2152, 2784, 1960, -48, -1560, -1504, -824, -224,
			408, 848, 920, 512, 0, 0, 168, 384, -376, -920, -1256, -1288, -64, 1248, 1864, 928, -336, -1992, -1976, 632, 1808, 1000,
			-792, -2536, -2576, -1104, 1008, 2456, 2272, 1704, 904}
	};
	uint16_t outEncoderBuffer[3][OUT_ENCODER_FRAME_SAMPLE_SIZE];
	uint16_t outEncoderEtalonBuffer[3][OUT_ENCODER_FRAME_SAMPLE_SIZE] = { {1877, 4032, 3823, 2050, 0, 19, 1262, 124}, {18, 255,
			4031, 2553, 58, 544, 323, 121}, {178, 1946, 3231, 1329, 95, 1834, 52, 45}};
	memset(outEncoderBuffer, 0xff, OUT_ENCODER_FRAME_SAMPLE_SIZE * 2);
	ISeconds_Time ts1, ts2;
	ISeconds_Time resultTime1, resultTime2;
	bool doneSuccessfully1, doneSuccessfully2;
	imbe_vocoder imbeEncoder;
	imbe_vocoder imbeDecoder;
	for (int ii = 0; ii < 3; ii++) {
		{
			Seconds_getTime(&ts1);
			GPIOPinWrite(SOC_GPIO_0_REGS, 111, GPIO_PIN_HIGH);
			imbeEncoder.imbe_encode((int16_t*) (&outEncoderBuffer[ii][0]), &inEncoderBuffer[ii][0]);
			GPIOPinWrite(SOC_GPIO_0_REGS, 111, GPIO_PIN_LOW);
			Seconds_getTime(&ts2);
			resultTime1.secs = (ts2.secs - ts1.secs);
			resultTime1.nsecs = (ts2.nsecs - ts1.nsecs);
			int errorss = 0;
			for (int i = 0; i < OUT_ENCODER_FRAME_SAMPLE_SIZE; i++) {
				if (outEncoderBuffer[ii][i] != outEncoderEtalonBuffer[ii][i])
					errorss++;
			}
			doneSuccessfully1 = (errorss == 0) ? true : false;
		}
		Task_sleep(10);
		{
			Seconds_getTime(&ts1);
			GPIOPinWrite(SOC_GPIO_0_REGS, 111, GPIO_PIN_HIGH);
			imbeDecoder.imbe_decode((int16_t*) (&outEncoderBuffer[ii][0]), &outDecoderBuffer[ii][0]);
			GPIOPinWrite(SOC_GPIO_0_REGS, 111, GPIO_PIN_LOW);
			Seconds_getTime(&ts2);
			resultTime2.secs = (ts2.secs - ts1.secs);
			resultTime2.nsecs = (ts2.nsecs - ts1.nsecs);
			int errorss = 0;
			for (int i = 0; i < IN_ENCODER_FRAME_SAMPLE_SIZE; i++) {
				if (outDecoderBuffer[ii][i] != outDecoderEtalonBuffer[ii][i])
					errorss++;
			}
			doneSuccessfully2 = (errorss == 0) ? true : false;
		}

		asm(" nop");
	}
}


static void mymix(int16_t * buffer, uint16_t size) {
	for(int i = 0; i<size; i++) {
		buffer[i] += 1;
	}
}

static void pack(uint16_t * vocoder_frame) {

	uint8_t O[11];

	O[0] = (((vocoder_frame[0] / 16) & 240) + (vocoder_frame[1] / 256));
	O[1] = (((vocoder_frame[2] / 16) & 240) + (vocoder_frame[3] / 256));
	O[2] = (((vocoder_frame[4] / 8) & 224) + ((vocoder_frame[5] / 64) & 28) + (vocoder_frame[6] / 512));
	O[3] = (((vocoder_frame[6] / 2) & 128) + vocoder_frame[7]);
	O[4] = (vocoder_frame[0] & 255);
	O[5] = (vocoder_frame[1] & 255);
	O[6] = (vocoder_frame[2] & 255);
	O[7] = (vocoder_frame[3] & 255);
	O[8] = (vocoder_frame[4] & 255);
	O[9] = (vocoder_frame[5] & 255);
	O[10] = (vocoder_frame[6] & 255);
}

void testCorrectnessOfWork() {

	WavReader * pWavReader = WavReader::make("d:\\Projects\\Vocoder_IMBE\\voice_8kHz.wav");
	WavWriter * pWavWriter = WavWriter::make("d:\\Projects\\Vocoder_IMBE\\imbe_voice_8kHz.wav");
	pWavWriter->addHeader(pWavReader->getHeader());

	int writeCounter = 0;

	int16_t inEncoderBuffer[IN_ENCODER_FRAME_SAMPLE_SIZE];
	int16_t outDecoderBuffer[IN_ENCODER_FRAME_SAMPLE_SIZE];
	uint8_t outEncoderBuffer[OUT_ENCODER_FRAME_SAMPLE_SIZE*2];
	memset(outEncoderBuffer, 0xff, OUT_ENCODER_FRAME_SAMPLE_SIZE*2);
	imbe_vocoder imbeEncoder;
	imbe_vocoder imbeDecoder;
	while(1) {
		uint16_t byteRead = pWavReader->readBytes((uint8_t*)inEncoderBuffer, IN_ENCODER_FRAME_BYTE_SIZE);
		if(byteRead == IN_ENCODER_FRAME_BYTE_SIZE) {
			imbeEncoder.imbe_encode((int16_t*)outEncoderBuffer, inEncoderBuffer);
			pack((uint16_t*)outEncoderBuffer);
			imbeDecoder.imbe_decode((int16_t*)outEncoderBuffer, outDecoderBuffer);
			mymix(outDecoderBuffer, IN_ENCODER_FRAME_BYTE_SIZE/2);
			pWavWriter->writeBytes((uint8_t*)outDecoderBuffer, IN_ENCODER_FRAME_BYTE_SIZE);
			writeCounter += IN_ENCODER_FRAME_BYTE_SIZE;
			printf("Write bytes %d \n", writeCounter);
		}
		else if(byteRead>0){
			pWavWriter->writeBytes((uint8_t*)inEncoderBuffer, byteRead);
			writeCounter += byteRead;
			printf("Write bytes 2 %d \n", writeCounter);
		}
		else {
			break;
		}
	}

	delete pWavWriter;
	delete pWavReader;
}

extern "C" Void taskFxn (UArg a0, UArg a1)
{

   // testFft(imbeEncoder);

	//testImbeExecutionTime();
	testCorrectnessOfWork();
}


/*
 *  ======== main ========
 */
Int main()
{
    /*
     * use ROV->SysMin to view the characters in the circular buffer
     */
    System_printf("enter main()\n");
    //clkoutPinEnable();

    /* The Local PSC number for GPIO is 3. GPIO belongs to PSC1 module.*/
    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_GPIO, PSC_POWERDOMAIN_ALWAYS_ON,
    PSC_MDCTL_NEXT_ENABLE);

    /* Pin Multiplexing of pin 12 of GPIO Bank 6.*/
    gpioPinEnable();
    //GPIOBank6Pin12PinMuxSetup();

    /* Sets the pin 109 (GP6[12]) as input.*/
    GPIODirModeSet(SOC_GPIO_0_REGS, 111, GPIO_DIR_OUTPUT);

    BIOS_start(); /* does not return */
    return (0);
}





