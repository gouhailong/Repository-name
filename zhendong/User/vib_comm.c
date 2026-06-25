#include "vib_comm.h"
#include "vib_detect.h"
#include <stdio.h>
#include <string.h>

void VibComm_SendJson(UART_HandleTypeDef *huart, const VibResult *result)
{
    char tx[192];
    int len;

    len = snprintf(tx, sizeof(tx),
                   "{\"seq\":%lu,\"rms\":%.4f,\"p2p\":%.4f,\"peak\":%.4f,"
                   "\"crest\":%.2f,\"score\":%u,\"state\":\"%s\",\"drop\":%lu}\r\n",
                   (unsigned long)result->seq,
                   result->feature.rms,
                   result->feature.p2p,
                   result->feature.peak,
                   result->feature.crest,
                   result->score,
                   VibDetect_StateText(result->state),
                   (unsigned long)result->dropped_windows);

    if (len > 0) {
        if (len > (int)sizeof(tx)) {
            len = (int)sizeof(tx);
        }
        HAL_UART_Transmit(huart, (uint8_t *)tx, (uint16_t)strlen(tx), 100U);
    }
}

