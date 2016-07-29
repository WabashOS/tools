#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

/* Output Method (OM) */
typedef enum {
  OM_NET,
  OM_FILE,
  OM_CONS
} om_t;

/* Set the default output method here */
#define OM_DEF OM_FILE
//#define OM_DEF OM_CONS

/* Set the TCP port to use */
#define PORTNUM 12345

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

#define NSAMPLE_DEF 100000
#define THRESH 900
int main(int argc, char *argv[], char **envp) {
  FILE *out = NULL;
  om_t om = OM_DEF;
  printf("Argc: %d\n", argc);
  for(int i = 0; i < argc; i++) {
    printf("Argv[%d]: %s\n", i, argv[i]);
  }

  /* Print environment variables,
   * doesn't work on all platforms (e.g. rumprun) */
	/*printf("Environment: \n");
  for (char** env = envp; *env != 0; env++)
  {
    char* thisEnv = *env;
    printf("%s\n", thisEnv);    
  }*/
  
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
      out = connect_remote(PORTNUM);
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
  
  fprintf(out, "Hello World!\n");
  
  /* Clean up all but OM_CONS */
  if(om == OM_NET || om == OM_FILE) {
    printf("Cleaning up file\n");
    fflush(out);
    fclose(out);
  }

  return 0;
}
