#ifndef __adcs_H
#define __adcs_H

typedef enum {
  QTZ_ADCS_Ping = 'p',  // "[P]ing" Ping the microcontroller to know
                        // if it's still alive.
  QTZ_ADCS_Awake = 'a', // "[A]wake" Wake up the microcontroller from
                        // a low power mode.
  QTZ_ADCS_Sleep = 's', // "[S]leep" Make the microcontroller enter a
                        // low power mode.
  QTZ_ADCS_GetData =
      'g', // "[G]et Data" Get sensor data from the microcontroller.
  QTZ_ADCS_Reset = 'r', // "[R]eset" Reset the microcontroller.
} QTZ_ADCS_COMMAND;

#endif
