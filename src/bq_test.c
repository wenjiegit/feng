#include "bufferqueue.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    double foo;
    double bar;
    size_t data_size;
    guchar data[];
} Stuff;

int awake = 0;
int stop_fill = 0;

GMutex *mux;

static void fill_cb(gpointer prod_p, gpointer cons_p)
{
    g_mutex_lock(mux);
    BufferQueue_Consumer *consumer = prod_p;
    BufferQueue_Producer *producer = cons_p;
    Stuff *buffer = g_malloc0(sizeof(Stuff) + 2000);

    while (bq_consumer_unseen(consumer) < 16) {
        buffer->foo++;
        buffer->bar--;
        buffer->data_size = 2000;
        memset(buffer->data, 'a', 14);
        bq_producer_put(producer, g_memdup (buffer, sizeof(Stuff) + 2000));
        if(awake++ > 10) {
            sleep(1);
            fprintf(stderr, "Sleeping %p\n", consumer);
            awake = 0;
        }
        if (stop_fill) goto exit;
    }
    exit:
    g_free(buffer);
    g_mutex_unlock(mux);
}


int main(void)
{
if (!g_thread_supported ()) g_thread_init (NULL);

int size = 10, i;
int count = 2000;
Stuff *buffer = g_malloc0(sizeof(Stuff) + 2000), *ret;
BufferQueue_Consumer *cons[size];
BufferQueue_Producer *prod = bq_producer_new(g_free, NULL);
GThreadPool *pool = g_thread_pool_new(fill_cb, prod, 6, FALSE, NULL);
mux = g_mutex_new();
//init
for (i = 0; i< size; i++) {
    cons[i] = NULL;
}

//fill stuff

    buffer->foo = 0;
    buffer->bar = 10;
    buffer->data_size = 2000;
    buffer->data[0] = 'a';
    bq_producer_put(prod, g_memdup (buffer, sizeof(Stuff) + 2000));
//consume stuff

    while(count--) {
        BufferQueue_Consumer *c = bq_consumer_new(prod);
        for (i = 0; i < size; i++) {
            if (!cons[i]) break;
            ret = bq_consumer_get(cons[i]);
            if (ret)
                fprintf(stderr, "Foo: %p %f %f ", ret, ret->bar, ret->foo);
            else
                fprintf(stderr, "Foo: NULL ");
            if(!bq_consumer_move(cons[i]))
                fprintf(stderr, "no next\n");
            else
                fprintf(stderr, "next %p\n", bq_consumer_get(cons[i]));
            g_thread_pool_push (pool, cons[i], NULL);
        }
        if (i < size && !cons[i]) {
            fprintf(stderr,"---- Allocating consumer %d ", i);
            cons[i] = bq_consumer_new(prod);
            fprintf(stderr,"%p\n", cons[i]);
        }
        bq_consumer_get(c);
        bq_consumer_free(c);
    }
    stop_fill = 1;
    g_thread_pool_free(pool, TRUE, TRUE);
    for (i = 0; i < size; i++)
        bq_consumer_free(cons[i]);
    bq_producer_unref(prod);
    g_mutex_free(mux);
    g_free(buffer);
    return 0;
}