#ifndef __milo_H
#define __milo_H
#include "common.h"

typedef struct {
  // Send operation timeout.
  uint32_t send_timeout;
  // Receive operation timeout.
  uint32_t recv_timeout;
  // Delay before doing any operation.
  uint32_t pre_delay;
  // Delay after doing all operations.
  uint32_t post_delay;
  // The size of the response in bytes.
  //
  // This amount will be written to the provided buffer.
  uint16_t response_size;
  // The command ID that will be sent.
  uint8_t command_id;
} QTZ_Command;

typedef enum {
  QTZ_MILO_Snapshot = 'S',             // '[S]napshot',
  QTZ_MILO_EnableEarthlimbModel = 'E', // 'MODEL [E]ARTHLIMB ON',
  QTZ_MILO_EnableHypsoModel = 'H',     // 'MODEL [H]YPSO ON',
  QTZ_MILO_EnableSentinelModel = 'T',  // 'MODEL SEN[T]INEL ON',
  QTZ_MILO_DisableModel = 'O',         // 'Turn [o]ff Model',
  QTZ_MILO_MoreBrightness = 'B',       // '[B]rightness +',
  QTZ_MILO_LessBrightness = 'b',       // '[B]rightness -',
  QTZ_MILO_MoreContrast = 'C',         // '[C]ontrast +',
  QTZ_MILO_LessContrast = 'c',         // '[C]ontrast -',
  QTZ_MILO_Status = 's',               // '[S]tatus',
  QTZ_MILO_ResetCam = 'r',             // '[R]eset OpenMV Cam',
  QTZ_MILO_ImageStatistics = 'i',      // '[I]mage Result',
  QTZ_MILO_Ping = 'p',                 // '[P]ing',
  QTZ_MILO_SPI_Enable = 'R',           // 'Cambio a SPI',
} QTZ_MILO_COMMAND;

typedef enum {
  QTZ_MILO_CloudyHigh = 'H',   // The label CloudyHigh was identified
  QTZ_MILO_CloudyMedium = 'M', // The label CloudyMedium was identified
  QTZ_MILO_CloudyLow = 'L',    // The label CloudyLow was identified
  QTZ_MILO_NoLabel = 'N',      // An invalid label was identified
} QTZ_MILO_Labels;

typedef enum {
  QTZ_MILO_OK,
  QTZ_MILO_Timeout,
} QTZ_MILO_Result;

QTZ_MILO_Result QTZ_MILO_SendCommand(QTZ_Command cmd,
                                     QTZ_ByteArray *response_buffer);
#endif
