#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>

/* Output Method (OM) */
typedef enum {
  OM_NET,
  OM_FILE,
  OM_CONS
} om_t;

typedef struct {
  long tid;
  FILE *out;
} hello_arg_t;

/* Set the default output method here */
#define OM_DEF OM_FILE
//#define OM_DEF OM_CONS

/* Number of threads */
#define NUM_THREADS 2
/* Amount of time to waste in the threads */
//#define THREAD_WORK 1000000
#define THREAD_WORK 10000000

/* Wait for a remote listener to connect over TCP at port_num*/
FILE *connect_remote(int port_num) {
  int listen_sock, client_sock;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
 
  listen_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_sock < 0) {
    fprintf(stderr, "ERROR opening socket: %s\n", strerror(errno));
    return NULL;
  }

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  /*serv_addr.sin_addr.s_addr = INADDR_ANY;*/
  serv_addr.sin_port = htons(port_num);
  if (bind(listen_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "ERROR on binding: %s\n", strerror(errno));
    return NULL;
  }

  printf("Listening on socket: %d\n", port_num);
  listen(listen_sock,5);

  clilen = sizeof(cli_addr);
  client_sock = accept(listen_sock, 
              (struct sockaddr *) &cli_addr, 
              &clilen);
  if (client_sock < 0) {
    fprintf(stderr, "ERROR on accept: %s\n", strerror(errno));
    return NULL;
  }
  close(listen_sock);
  
  printf("Connected!\n");
  /* Return a FILE* for the newly connected client */
  return fdopen(client_sock, "w");
}

void *print_hello(void *varg)
{
  /* Waste some time */
  volatile int counter = 1;
  for(int i = 0; i < THREAD_WORK; i++) {
    for(int j = 0; j < 1000; j++) {
      counter = counter+=1;
    }
  }
  hello_arg_t *arg = (hello_arg_t*)varg;
  fprintf(arg->out, "Hello World! It's me, thread #%ld! Val is %d\n", arg->tid, counter);
  pthread_exit(NULL);
}

#define NSAMPLE_DEF 100000
#define THRESH 900
int main(int argc, char *argv[]) {
  FILE *out = NULL;
  om_t om = OM_DEF;
  int rc;

  opterr = 0;
  int c;
  while ((c = getopt(argc, argv, "nfc")) != -1)
    switch (c) {
      
      case 'n': om = OM_NET; break;
      case 'f': om = OM_FILE; break;
      case 'c': om = OM_CONS; break;
      default:
        fprintf(stderr, "Unrecognized output mode %c. Valid options are \'n\' (network), \'f\' (file), or \'c\' (console).\n", *optarg);
            exit(EXIT_FAILURE);
      break;
    }

  /* Setup the output method */
  switch(om) {
    case OM_NET:
      out = connect_remote(12345);
      break;
    case OM_FILE:
      out = fopen("./hello.out", "w");
      break;
    case OM_CONS:
      out = stderr;
      break;
    default:
      out = NULL;
  }
  if(!out) {
    fprintf(stderr, "Failed to open output: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  struct timeval t_start, t_end;
  rc = gettimeofday(&t_start, NULL);
  if(rc != 0) {
    printf("Problem with gettimeofday\n");
    return EXIT_FAILURE;
  }

  time_t start;
  start = time(NULL);

  pthread_t threads[NUM_THREADS];
  hello_arg_t *arg;
  long t;
  for(t=0;t<NUM_THREADS;t++){
    printf("In main: creating thread %ld\n", t);
    arg = malloc(sizeof(hello_arg_t)); /* this leaks, don't care */
    arg->tid = t;
    arg->out = out;
    rc = pthread_create(&threads[t], NULL, print_hello, (void *)arg);
    if (rc){
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
      }
    }  

  /* Clean up all but OM_CONS */
  if(om == OM_NET || om == OM_FILE) {
    printf("Cleaning up file\n");
    fflush(out);
    fclose(out);
  }

  void *retval;
  for(t=0;t<NUM_THREADS;t++){
    pthread_join(threads[t], &retval);
  }

  rc = gettimeofday(&t_end, NULL);
  if(rc != 0) {
    printf("Problem with gettimeofday\n");
    return EXIT_FAILURE;
  }

  double t_tot = t_end.tv_sec - t_start.tv_sec;
  t_tot += (double)(t_end.tv_usec - t_start.tv_usec) / 1000000;
  printf ("Time: %lf\n", t_tot);

  pthread_exit(NULL);
  return 0;
}
