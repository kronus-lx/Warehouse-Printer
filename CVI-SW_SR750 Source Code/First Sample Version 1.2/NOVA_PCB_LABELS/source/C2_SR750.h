#ifndef C2_SR750_H
#define C2_SR750_H

//int scanner_callback (unsigned handle, int xType, int errCode, void *callbackData);
  int start_scanner(void);
  int stop_scanner(void);
  int init_scanner(void);
  int check_read_count(int* scannedcount, int* badCount); 
#endif
