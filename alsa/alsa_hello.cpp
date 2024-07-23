//
// Created by win10 on 2024/7/15.
//

#include <stdio.h>
#include <cstdint>
#include <cstdlib>
#include "UnlockQueue.h"


#define LSX_USE_VAR(x)  ((void)(x)) /* Parameter or variable is intentionally unused. */
#define LSX_UNUSED  __attribute__ ((unused)) /* Parameter or local variable is intentionally unused. */
#define PCM_LOCALS int16_t sox_macro_temp_pcm LSX_UNUSED; \
  double sox_macro_temp_double LSX_UNUSED

#define INT16_TO_FLOAT(d) ((d)*(1.0 / (INT16_MAX + 1.0)))

#define FLOAT_TO_PCM(d) FLOAT_64BIT_TO_PCM(d)

#define FLOAT_64BIT_TO_PCM(d)                        \
  (int16_t)(                                                  \
    LSX_USE_VAR(sox_macro_temp_pcm),                            \
     sox_macro_temp_double = (d) * (INT16_MAX + 1.0),       \
    sox_macro_temp_double < 0 ?                                 \
      sox_macro_temp_double <= INT16_MIN - 0.5 ?           \
        /*++(clips),*/ INT16_MIN :                             \
        sox_macro_temp_double - 0.5 :                           \
      sox_macro_temp_double >= INT16_MAX + 0.5 ?           \
        sox_macro_temp_double > INT16_MAX + 1.0 ?          \
          /*++(clips),*/ INT16_MAX :                           \
          INT16_MAX :                                      \
        sox_macro_temp_double + 0.5                             \
  )


int main()
{
    UnlockQueue *queue = new UnlockQueue(5 * sizeof(void*));
    queue->Initialize();
    printf("======> empty %d\n", queue->IsEmpty());
    for(int i=0; i<5; i++)
    {
        char *p = (char *)malloc(10);
        int size = sizeof(void*);
        printf("malloc ------------>%p\n", p);
        queue->Put((const unsigned char *)&p, size);
    }
    printf("======> %d %d\n", queue->IsFull(), queue->GetDataLen());

    uint16_t t16 = 32768/2;
    float f=0.5;
    PCM_LOCALS;


    f = INT16_TO_FLOAT(t16);
    printf("t16=%d   %f\n", t16, f);
    t16 = FLOAT_TO_PCM(f);
    printf("t16=%d   %f\n", t16, f);

    while(queue->GetDataLen() > 0)
    {
        char *p = nullptr;
        int size = sizeof(void*);
        queue->Get((unsigned char *)&p, size);
        printf("free ------------>%p\n", p);
        free(p);
    }
    printf("======> empty %d\n", queue->IsEmpty());
    delete queue;
    return 0;
}