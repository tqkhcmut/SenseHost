#ifndef _flash_h_
#define _flash_h_

struct flash_data
{
  unsigned char id;
};

int flash_write_buffer(char * buff, int size);
int flash_read_buffer(char * buff, int size);

#endif
