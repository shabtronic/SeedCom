#pragma once
#ifndef SEEDCOMSERVER_H
#define SEEDCOMSERVER_H
#ifdef __cplusplus

#include "daisysp.h"
#include "daisy_seed.h"

void RebootToBootloader();
void UartShouldReboot();
void UartInitSerial(daisy::DaisySeed *hw,int RebootButton);


#endif
#endif