#include "./obc_impl.c"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  char type;
  QTZ_OBC_Packet i2c;
  QTZ_OBC_Packet uart;
} FuzzSeed;

void panic_write(void *p, size_t size, size_t length, FILE *f) {
  int written = fwrite(p, size, length, f);
  if (written != length) {
    fprintf(stderr, "ERROR: Failed to write to file!");
    exit(1);
  }
}

void write_seed(FILE *f, FuzzSeed *seed) {
  QTZ_ByteArray_Create(tmp, tmp_buff, 32);

  panic_write(&seed->type, 1, 1, f);

  switch (seed->type) {
  case 'I': {
    QTZ_OBC_WritePacket(&tmp, seed->i2c);
    panic_write(tmp.data, 1, tmp.length, f);
  } break;
  case 'U': {
    QTZ_OBC_WritePacket(&tmp, seed->uart);
    panic_write(tmp.data, 1, tmp.length, f);
  } break;
  default:
    QTZ_OBC_WritePacket(&tmp, seed->i2c);
    panic_write(tmp.data, 1, tmp.length, f);

    QTZ_OBC_WritePacket(&tmp, seed->uart);
    panic_write(tmp.data, 1, tmp.length, f);
  }
}

int main() {
  const int SEED_COUNT = 2;
  FuzzSeed seeds[] = {
      {
          .type = 'I',
          .i2c =
              (QTZ_OBC_Packet){
                  .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
                  .status = 0,
                  .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
                  .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_PING,
                  .param0 = 0,
                  .param1 = 0,
              },
          .uart = {0},
      },
      {
          .type = 'I',
          .i2c =
              (QTZ_OBC_Packet){
                  .protocol_id = QTZ_OBC_PROTOCOL_HANDOVER,
                  .status = 0,
                  .subsys = QTZ_OBC_SUBSYSTEM_PORTENTA,
                  .cmd_id = QTZ_OBC_COMMAND_GOMSPACE_STATUS,
                  .param0 = 0,
                  .param1 = 0,
              },
          .uart = {0},
      },
  };

  char fname[10] = {0};
  for (int i = 0; i < SEED_COUNT; i++) {
    sprintf(fname, "%09d", i);
    FILE *f = fopen(fname, "w");
    write_seed(f, &seeds[i]);
    fclose(f);
  }
}
