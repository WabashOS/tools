#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "hrtimer.h"

/* Output Method (OM) */
typedef enum {
  OM_NET,
  OM_FILE,
  OM_CONS
} om_t;

/* Set the default output method here */
#define OM_DEF OM_FILE
//#define OM_DEF OM_CONS

unsigned long long g_timerfreq;
void perform_selfish(int num_runs, int threshold, uint64_t *results) {
	
	int cnt=0, num_not_smaller = 0;
	HRT_TIMESTAMP_T current, prev, start;
	uint64_t sample = 0;
	uint64_t elapsed, thr, min=(uint64_t)~0;
  int i;

	// we will do a "calibration run" of the detour benchmark to
	// get a reasonable value for the minimal detour time
	// just perform the benchmark and record the minimal detour time until
	// this minimal detour time does not get smaller for 1000 (as defined by NOT_SMALLER) 
	// consecutive runs
	
	#define NOT_SMALLER 101
  #define INNER_TRIES 50

  thr = min*(threshold/100.0);
	while (num_not_smaller < NOT_SMALLER) {
		cnt = 0;

		HRT_GET_TIMESTAMP(start);
		HRT_GET_TIMESTAMP(current);

    // this is exactly the same loop as below for measurement
    while (cnt < INNER_TRIES) {
      prev = current;
      HRT_GET_TIMESTAMP(current);

      sample++;

      HRT_GET_ELAPSED_TICKS(prev, current, &elapsed);
      // != instead of < in the benchmark loop in order to make the
      // notsmaller principle useful
      if ( elapsed != thr ) { 
        HRT_GET_ELAPSED_TICKS(start, prev, &results[cnt++]);
        HRT_GET_ELAPSED_TICKS(start, current, &results[cnt++]);
      }
    }

    // find minimum in results array - this is outside the
    // calibration/measurement loop!
    {
      if(min == 0) {
          printf("the initialization reached 0 clock cycles - the clock accuracy seems too low (setting min=1 and exiting calibration)\n");
          min = 1;
          break;
      }
      int smaller=0;
      for(i = 0; i < INNER_TRIES; i+=2) {
        if(results[i+1]-results[i] < min) {
          min = results[i+1]-results[i];
          smaller=1;
          //printf("[%i] min: %lu\n", r, min);
        }
        //printf("%lu\n", results[i+1] - results[i]);
      } 
      //printf("\n\n");
      if (!smaller) num_not_smaller++;
      else num_not_smaller = 0;
    }
	}
	
  printf("# Minimal cycle length [ticks]: %lu \n", min);

//  return;
  // now we perform the actual benchmark: Read a time-stamp-counter in a tight
	// loop ignore the results if the timestamps are close to each other, as we can assume
	// that nobody interrupted us. If the difference of the timestamps exceeds a certain
	// threshold, we assume that we have been "hit" by a "noise event" and record the
	// time difference for later analysis

	cnt = 2;
	sample = 0;

	HRT_GET_TIMESTAMP(start);
	HRT_GET_TIMESTAMP(current);
  
  // perform this outside measurement loop in order to save
  // time/increase measurement frequency
  thr = min*(threshold/100.0);
	while (cnt < num_runs) {
		prev = current;
		HRT_GET_TIMESTAMP(current);

		sample++;

		HRT_GET_ELAPSED_TICKS(prev, current, &elapsed);
		if ( elapsed > thr ) {
			HRT_GET_ELAPSED_TICKS(start, prev, &results[cnt++]);
			HRT_GET_ELAPSED_TICKS(start, current, &results[cnt++]);
		}
	}

	results[0] = min;
	results[1] = sample;
}

double get_ticks_per_second() {
	#define NUM_TESTS 10

	HRT_TIMESTAMP_T t1, t2;
	uint64_t res[NUM_TESTS];
	static uint64_t min=0; 
	int count;

	if (min > 0) {return ((double) min);}

	printf("Calibrating timer, this might take some time: ");
	for (count=0; count<NUM_TESTS; count++) {
		HRT_GET_TIMESTAMP(t1);
		sleep(1);
		HRT_GET_TIMESTAMP(t2);
		HRT_GET_ELAPSED_TICKS(t1, t2, &res[count])
	}

	min = res[0];
	for (count=0; count<NUM_TESTS; count++) {
		if (min > res[count]) min = res[count];
	}

	return ((double) min);
}

void print_results(uint64_t *results, int num_results, int threshold, FILE *out) {
  int i;
	double tpns, sum=0;
  uint64_t cycles=results[1];
  double cycletime;

	tpns = get_ticks_per_second()/1e9;
  cycletime = results[0] / tpns;

  /* Report Results */
  printf("# Selfish Benchmark\n");
	printf("# Minimal cycle length [ns]: %f \n", cycletime);
	printf("# Number of iterations (recorded+unrecorded): %lu \n", cycles);
	printf("# Threshold: [%% minimal cycle length]: %i \n", threshold);
	printf("# Time [ns]\tselfish duration [ns]\n");
  if(out) {
    fprintf(out, "# Selfish Benchmark\n");
	  fprintf(out, "# Minimal cycle length [ns]: %f \n", cycletime);
	  fprintf(out, "# Number of iterations (recorded+unrecorded): %lu \n", cycles);
	  fprintf(out, "# Threshold: [%% minimal cycle length]: %i \n", threshold);
	  fprintf(out, "# Time [ns]\tselfish duration [ns]\n");
  }

  for (i=2; i<num_results; i+=2) {
    sum += (results[i+1]-results[i]-results[0])/tpns;
    if(out) {
		  fprintf(out, "%.2f\t%.2f\n", (results[i] - results[2])/tpns, (results[i+1]-results[i]-results[0])/tpns);
    }
	}

  double duration=(results[num_results-1]-results[2])/tpns;
  printf("CPU overhead due to noise: %.2f%%\n", 100 * (sum/duration));
  printf("Measurement period: %.2f s\n", duration/1e9);
  if(out) {
    fprintf(out, "# CPU overhead due to noise: %.2f%%\n", 100 * (sum/duration));
    fprintf(out, "# Measurement period: %.2f s\n", duration/1e9);
  }

  if(duration/1e9 < 1.0) {
    printf("WARNING: the measurement period is less than a second, the results "
           "might be inaccurate and indicate too much noise! Increase the "
           "number of samples or the threshold to avoid this!\n");
  }
}

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
int main(int argc, char *argv[]) {
  FILE *out = NULL;
  uint64_t nsample = NSAMPLE_DEF;
  om_t om = OM_DEF;
  /*
  printf("Gonna try it, hold on to your butts\n");
  out = connect_remote(12346);
  if(!out) {
    fprintf(stderr, "Woa bra, no dice on the socket\n");
    return EXIT_FAILURE;
  }
  fprintf(out, "Gotcha babe\n");
  fflush(out);
  printf("We totes complete bra\n");
  fclose(out);
  //sanity_check(1);
  return EXIT_SUCCESS;
  */
  opterr = 0;
  int c;
  while ((c = getopt(argc, argv, "n:o:")) != -1)
    switch (c) {
      case 'n':
        nsample = atoi(optarg);
        break;
      case 'o':
        switch(*optarg) {
          case 'n': om = OM_NET; break;
          case 'f': om = OM_FILE; break;
          case 'c': om = OM_CONS; break;
          default:
            fprintf(stderr, "Unrecognized output mode %c. Valid options are \'n\' (network), \'f\' (file), or \'c\' (console).\n", *optarg);
            exit(EXIT_FAILURE);
        }
        break;
      default:
        fprintf(stderr, "Unrecognized option %c\n", optopt);
        fprintf(stderr, "Options were:\n");
        for(int i = 0; i < argc; i++) {
          fprintf(stderr, "\t%s ", argv[i]);
        }
        fprintf(stderr, "\n");
        exit(EXIT_FAILURE);
    }

  /* Get inputs
  int nsample = 0;
  if(argc < 2) {
    nsample = NSAMPLE;
  } else {
    nsample = atoi(argv[1]);
  }
  */
  printf("#samples: %lu\n", nsample);

  /* Setup the timer */
  HRT_INIT(1, g_timerfreq);
  uint64_t *res = malloc((nsample + 2)*sizeof(uint64_t));
  perform_selfish(nsample, THRESH, res);

  /* Setup the output method */
  switch(om) {
    case OM_NET:
      out = connect_remote(12345);
      break;
    case OM_FILE:
      out = fopen("./results.out", "w");
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
  
  print_results(res, nsample, THRESH, out);
  
  /* Clean up all but OM_CONS */
  if(om == OM_NET || om == OM_FILE) {
    printf("Cleaning up file\n");
    fflush(out);
    fclose(out);
  }

  return 0;
}
