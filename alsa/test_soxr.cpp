#include <iostream>
#include <thread>
#include <unistd.h>
#include <future>
#include "soxr.h"
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[])
{

    // 1. 打开音频
    FILE *in = fopen("48k_2.pcm", "rb"); // 单声道
    // FILE *in = fopen("44k.pcm", "rb"); // 立体声
    if(in == NULL)
    {
        printf("error to open in file\n");
        exit(1);
    }

    FILE *out = fopen("44.1k_2.pcm", "wb");
    if(out == NULL)
    {
        printf("error to open out file\n");
        exit(1);
    }


    double const irate = 48000.;
    double const orate = 44100.;

/* Allocate resampling input and output buffers in proportion to the input
 * and output rates: */
#define buf_total_len 1024 /* In samples. */
    size_t const olen = (size_t)(orate * buf_total_len / (irate + orate) + .5);
    size_t const ilen = buf_total_len - olen;
    size_t const osize = sizeof(int16_t)*2, isize = osize;
    void *obuf = malloc(osize * olen);
    void *ibuf = malloc(isize * ilen);

    size_t odone, written, need_input = 1;
    soxr_error_t error;
    // AV_SAMPLE_FMT_S16  SOXR_INT16_I c1 c1 c1 c1 .... c2 c2 c2 c2 .    // 单声道使用这个
    // AV_SAMPLE_FMT_S16P SOXR_INT16_S c1 c2 c1 c2 c1 c2 c1 c2...
    soxr_datatype_t type = SOXR_INT16_I;
    soxr_io_spec_t io_spec = soxr_io_spec(type, type);

    /* Create a stream resampler: */
    soxr_t soxr = soxr_create(
            irate, orate, 2,   /* Input rate, output rate, # of channels. */
            &error,            /* To report any error during creation. */
            &io_spec,
            NULL, NULL); /* Use configuration defaults.*/

    if (!error)
    {
        printf("will input \n");

        do
        {
            size_t ilen1 = 0;

            if (need_input)
            {

                /* Read one block into the buffer, ready to be resampled: */
                ilen1 = fread(ibuf, isize, ilen, in);

                if (!ilen1)
                {                /* If the is no (more) input data available, */
                    free(ibuf);  /* set ibuf to NULL, to indicate end-of-input */
                    ibuf = NULL; /* to the resampler. */
                }
            }

            /* Copy data from the input buffer into the resampler, and resample
             * to produce as much output as is possible to the given output buffer: */
            error = soxr_process(soxr, ibuf, ilen1, NULL, obuf, olen, &odone);

            written = fwrite(obuf, osize, odone, out); /* Consume output.*/

            /* If the actual amount of data output is less than that requested, and
             * we have not already reached the end of the input data, then supply some
             * more input next time round the loop: */
            need_input = odone < olen && ibuf;

        } while (!error && (need_input || written));
    }

    printf("over\n");
    // 3. 保存
    fclose(out);
    fclose(in);

    /* Tidy up: */
    soxr_delete(soxr);
    free(obuf), free(ibuf);
    /* Diagnostics: */
    fprintf(stderr, "%-26s %s; I/O: %s\n", argv[0], soxr_strerror(error),
            ferror(stdin) || ferror(stdout) ? strerror(errno) : "no error");
    return !!error;
}