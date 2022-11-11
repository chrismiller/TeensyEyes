// from the linker
//  extern unsigned long _stextload;
extern unsigned long _stext;
extern unsigned long _etext;
//  extern unsigned long _sdataload;
extern unsigned long _sdata;
extern unsigned long _edata;
extern unsigned long _sbss;
extern unsigned long _ebss;
//  extern unsigned long _flexram_bank_config;
extern unsigned long _estack;

void DumpMemoryInfo() {
#if defined(__IMXRT1062__) && defined(DEBUG_MEMORY)
  uint32_t flexram_config = IOMUXC_GPR_GPR17;
  Serial.printf("IOMUXC_GPR_GPR17:%x IOMUXC_GPR_GPR16:%x IOMUXC_GPR_GPR14:%x\n",
                flexram_config, IOMUXC_GPR_GPR16, IOMUXC_GPR_GPR14);
  Serial.printf("Initial Stack pointer: %x\n", &_estack);
  uint32_t dtcm_size = 0;
  uint32_t itcm_size = 0;
  for (; flexram_config; flexram_config >>= 2) {
    if ((flexram_config & 0x3) == 0x2) dtcm_size += 32768;
    else if ((flexram_config & 0x3) == 0x3) itcm_size += 32768;
  }
  Serial.printf("ITCM allocated: %u  DTCM allocated: %u\n", itcm_size, dtcm_size);
  Serial.printf("ITCM init range: %x - %x Count: %u\n", &_stext, &_etext, (uint32_t)&_etext - (uint32_t)&_stext);
  Serial.printf("DTCM init range: %x - %x Count: %u\n", &_sdata, &_edata, (uint32_t)&_edata - (uint32_t)&_sdata);
  Serial.printf("DTCM cleared range: %x - %x Count: %u\n", &_sbss, &_ebss, (uint32_t)&_ebss - (uint32_t)&_sbss);
  Serial.printf("Now fill rest of DTCM with known pattern(%x - %x\n", (&_ebss + 1), (&itcm_size - 10)); Serial.flush(); //
  // Guess of where it is safe to fill memory... Maybe address of last variable we have defined - some slop...
  for (uint32_t *pfill = (&_ebss + 32); pfill < (&itcm_size - 10); pfill++) {
    *pfill = 0x01020304;  // some random value
  }
#endif
}

void EstimateStackUsage() {
#if defined(__IMXRT1062__) && defined(DEBUG_MEMORY)
  uint32_t *pmem = (&_ebss + 32);
  while (*pmem == 0x01020304) pmem++;
  Serial.printf("Estimated max stack usage: %d\n", (uint32_t)&_estack - (uint32_t)pmem);
#endif
}