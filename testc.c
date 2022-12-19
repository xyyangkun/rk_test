/******************************************************************************
 *
 *       Filename:  testc.c
 *
 *    Description:  test
 *
 *        Version:  1.0
 *        Created:  2022年10月14日 15时24分53秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#define NTHREADS 5
void *thread_function(void *);
int main()
{
   pthread_t thread_id[NTHREADS];
   int i, j;
   for(i=0; i < NTHREADS; i++)
   {
      pthread_create( &thread_id[i], NULL, thread_function, NULL );
   }

   for(j=0; j < NTHREADS; j++)
   {
      pthread_join( thread_id[j], NULL);
   }
   return 0;
}

void *thread_function(void *dummyPtr)
{
   printf("Thread number %ld addr(errno):%p\n", pthread_self(), &errno);
}
