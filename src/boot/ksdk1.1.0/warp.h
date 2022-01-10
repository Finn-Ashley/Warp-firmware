#include "fsl_spi_master_driver.h"

#define	min(x,y)	((x) < (y) ? (x) : (y))
#define	max(x,y)	((x) > (y) ? (x) : (y))
#define	USED(x)		(void)(x)

/*
 *	On Glaux, we use PTA0/IRQ0/LLWU_P7 (SWD_CLK) as the interrupt line
 *	for the RV8803C7 RTC. The original version of this function for the
 *	FRDMKL03 was using PTB0.
 *
 *	The following taken from KSDK_1.1.0//boards/frdmkl03z/board.h. We
 *	don't include that whole file verbatim since we have a custom board.
 */
#define BOARD_SW_HAS_LLWU_PIN		1
#define BOARD_SW_LLWU_EXT_PIN		7
#define BOARD_SW_LLWU_PIN		0
#define BOARD_SW_LLWU_BASE		PORTA_BASE
#define BOARD_SW_LLWU_IRQ_HANDLER	PORTA_IRQHandler
#define BOARD_SW_LLWU_IRQ_NUM		PORTA_IRQn

typedef enum
{
	kWarpStatusOK			= 0,

	kWarpStatusDeviceNotInitialized,
	kWarpStatusDeviceCommunicationFailed,
	kWarpStatusBadDeviceCommand,

	/*
	 *	Generic comms error
	 */
	kWarpStatusCommsError,

	/*
	 *	Power mode routines
	 */
	kWarpStatusPowerTransitionErrorVlpr2Wait,
	kWarpStatusPowerTransitionErrorVlpr2Stop,
	kWarpStatusPowerTransitionErrorRun2Vlpw,
	kWarpStatusPowerTransitionErrorVlpr2Vlpr,
	kWarpStatusErrorPowerSysSetmode,
	kWarpStatusBadPowerModeSpecified,

	/*
	 *	Always keep this as the last item.
	 */
	kWarpStatusMax
} WarpStatus;

typedef enum
{
	/*
	 *	NOTE: This order is depended on by POWER_SYS_SetMode()
	 *
	 *	See KSDK13APIRM.pdf Section 55.5.3
	 */
	kWarpPowerModeWAIT,
	kWarpPowerModeSTOP,
	kWarpPowerModeVLPR,
	kWarpPowerModeVLPW,
	kWarpPowerModeVLPS,
	kWarpPowerModeVLLS0,
	kWarpPowerModeVLLS1,
	kWarpPowerModeVLLS3,
	kWarpPowerModeRUN,
} WarpPowerMode;

typedef enum
{
	kWarpModeDisableAdcOnSleep		= (1 << 0),
} WarpModeMask;

typedef enum
{
	kWarpMiscMarkerForAbsentByte					= 0xFF,
} WarpMisc;

typedef struct
{
	bool			isInitialized;

	/*
	 *	For holding the SPI CS I/O pin idnetifier to make
	 *	the driver independent of board config.
	 */
	int			chipSelectIoPinID;

	uint8_t *		spiSourceBuffer;
	uint8_t *		spiSinkBuffer;
	size_t			spiBufferLength;
	uint16_t		operatingVoltageMillivolts;
} WarpSPIDeviceState;

typedef struct
{
	bool			isInitialized;
	uint8_t			uartTXBuffer[kWarpSizesUartBufferBytes];
	uint8_t			uartRXBuffer[kWarpSizesUartBufferBytes];
	uint16_t		operatingVoltageMillivolts;
} WarpUARTDeviceState;

typedef struct
{
	uint8_t			errorCount;
} WarpPowerManagerCallbackStructure;

WarpStatus	warpSetLowPowerMode(WarpPowerMode powerMode, uint32_t sleepSeconds);
void		warpEnableSPIpins(void);
void		warpDeasserAllSPIchipSelects(void);
void		warpPrint(const char *fmt, ...);
