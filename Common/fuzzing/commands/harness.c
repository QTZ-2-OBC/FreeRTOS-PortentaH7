#include "./obc_impl.c"
#include <stdio.h>

int main(int argc, char **argv) {
  FILE *f = fopen(argv[1], "r");
  // NOTE: Guaranteed by the fuzzer
  // if (f == NULL) {
  //   return 0;
  // }

  QTZ_ByteArray_Create(i2c_arr, i2c_buff, 12);
  QTZ_ByteArray_Create(uart_arr, uart_buff, 12);

  // TODO: Get MILO task state and OBC state from input buffer too.
  QTZ_OBC_Ctx ctx = {
      .i2c = i2c_arr,
      .uart_rs485 = uart_arr,
      .milo_task = {0},
      .state = 0,
      .watchdog_ticks = 0,
  };

  char tmp[12] = {0};

  // Read type of packet. If it didn't read anything then default to 0.
  int _ = fread(tmp, 1, 1, f);

  // Read packet data
  switch (tmp[0]) {
  case 'I':
    i2c_arr.length = fread(i2c_arr.data, 1, QTZ_OBC_PACKET_LEN, f);
    break;
  case 'U':
    uart_arr.length = fread(uart_arr.data, 1, QTZ_OBC_PACKET_LEN, f);
    break;
  default:
    i2c_arr.length = fread(i2c_arr.data, 1, QTZ_OBC_PACKET_LEN, f);
    uart_arr.length = fread(uart_arr.data, 1, QTZ_OBC_PACKET_LEN, f);
  }

  QTZ_OBC_Routine_Tick(&ctx);
}
