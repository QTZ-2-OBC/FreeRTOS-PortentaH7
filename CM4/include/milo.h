#ifndef __milo_H
#define __milo_H
#include "common.h"

typedef enum {
    QTZ_MILO_Snapshot							= 'S', // '[S]napshot',
    QTZ_MILO_EnableEarthlimbModel = 'E', // 'MODEL [E]ARTHLIMB ON',
    QTZ_MILO_EnableHypsoModel			= 'H', // 'MODEL [H]YPSO ON',
    QTZ_MILO_EnableSentinelModel	= 'T', // 'MODEL SEN[T]INEL ON',
    QTZ_MILO_DisableModel					= 'O', // 'Turn [o]ff Model',
    QTZ_MILO_MoreBrightness				= 'B', // '[B]rightness +',
    QTZ_MILO_LessBrightness				= 'b', // '[B]rightness -',
    QTZ_MILO_MoreContrast					= 'C', // '[C]ontrast +',
    QTZ_MILO_LessContrast					= 'c', // '[C]ontrast -',
    QTZ_MILO_Status								= 's', // '[S]tatus',
    QTZ_MILO_ResetCam							= 'r', // '[R]eset OpenMV Cam',
    QTZ_MILO_ImageStatistics			= 'i', // '[I]mage Result',
    QTZ_MILO_Ping									= 'p', // '[P]ing',
    QTZ_MILO_SPI_Enable						= 'R', // 'Cambio a SPI',
}QTZ_MILO_COMMAND ;

typedef enum {
	QTZ_MILO_OK,
}QTZ_MILO_RESULT;

QTZ_MILO_RESULT QTZ_MILO_SendCommand(QTZ_MILO_COMMAND cmd, QTZ_ByteArray *response_buffer, uint32_t timeout);
#endif
