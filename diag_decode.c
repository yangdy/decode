/*
Copoyright (C) 2013, yangdy <yangdy AT outlook.com>.
All rights reserved.
*/

#include "eQxdm.h"
#include "memory_data_decode.h"

data_decode_funcs_t data_decode_funcs[] = {
    {   0x20,   &memory_data_decode,    "memory data"   },
    {     -1,   NULL,                   NULL            }
};
