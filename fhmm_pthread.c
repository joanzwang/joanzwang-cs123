#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>

#include "fhmm_pthread.h"
#include "util.h"
#define NSTATE 4
#define MOBSRV 450

int ndevices = 19;
int T = 9999999;
double **O_array ;
lambda **l;
int log_probn = 0;

pthread_mutex_t A_mutex;
pthread_mutex_t B_mutex;

/*prints specified array into a file*/
void print_array(double parr[],int size, FILE *fp) {
	int i;
  for (i = 0; i < size; i++) {
      fprintf(fp, "%lf\n", parr[i]);
      printf("parr[%d] = %lf\n",i,parr[i]);
  }
}

/*initialize array population, making sure that all rows add up to 1*/
double *row_stochastic(double *array, int size) {
    int i;
    array = ckMalloc(sizeof(double) * size);
    double even = (double)1/size;  //even is the value of the element of the array after splitting '1' evenly across the row
    for ( i = 0; i < size; i++) {
      array[i] = even;
    }
    return array;
}

/* load_queue: start function for the thread that will fill the queue */
void *load_data(void *arg) {
    assert(arg != NULL);
    load_arg_t la = (load_arg_t ) arg;
    char *s;
    char *ptr;

    printf("filename %s\n", la->filename);

    FILE *f = fopen(la->filename, "r");
    if (f == NULL) {
        fprintf(stderr, "Unable to open file %s\n", la->filename);
        exit(1);
    }
    la->T = 0;
    int i = 0;

    while ((s = readLine(f)) != NULL) {
        printf("ndevices: %d T: %d\n", ndevices,T);
        if( i>ndevices ) {
          break;
        }

        char *token = strtok(s, ",");
          //printf("%d: %s\n",i,token);
        //for every line in the file that isn't the time, so for every device
        //write the energy output of the device into the array O_house
        if (strcmp(token,"timestamp") != 0) {
            la->T = 0;
            while(token !=NULL) {
                //printf("i:%d la->T:%d %s\n", i,la->T,token);
                *(*(O_array+i)+la->T) = strtod(token, &ptr);
                //printf("O_array:%lf \n", O_array[i][la->T]);
                la->T++;
                if(la->T > T) {
                  break;
                }
                token = strtok(NULL, ",");
            }
           i++;
        }
    }
    printf(" end of read file");
    pthread_exit(NULL);
}

/*compute the alpha array for this hmm model*/
void *hmm_calc(void *arg) {
  assert(arg != NULL);
  data_arg_t d = (data_arg_t) arg;
  lambda *lp = d->l;
  int N = d->N;
  int M = d->M;
  int T = d->T;
  int index = d->i;
  double *O = d->O;

  double log_prob_old = d->log_prob_old;

  //perform the alpha and beta passes to determine arrays alpha and beta
  alpha_pass(index, d->c_array, N, d->alpha, O, T);
  beta_pass(lp, N, T, O, d->beta, d->c_array);

  //perform gamma_pass, which uses alpha and beta matrices to determine gamma and digamma
  gamma_pass(T, N, lp, O, d->alpha, d->beta, d->gamma, d->digamma);

  //recalculate the components of the model based off of gamma and digamma
  recalc(lp, N, M, T, O, d->gamma, d->digamma);

  //calculate the log likelihood of the observations given our model, and decide whether or not to iterate
  log_prob(T, d->c_array);  //calculate the new value for log_probn
  double log_prob_new;
  log_prob_new = log_probn;

  d->iter++;

  if (d->iter <d->nmax_iter && log_prob_new >log_prob_old) {
    d->log_prob_old = log_prob_new;
    hmm_calc(d);
  }

  free(d->alpha);
  free(d->beta);
  free(d->c_array);
  free(d->gamma);
  free(d->digamma);
  pthread_exit(NULL);
}

/*compute the alpha array for this hmm model*/
void alpha_pass(int index, double c_array[], int N, double alpha[T][NSTATE], double *O, int T) {
  printf("in alpha \n");
  c_array[0] = 0;
  printf("A: %lf\n", *(*(l[index]->A + 0) + 1));
  int i;
  for (i = 0; i < N; i++) {
    if (O != NULL) {
        printf("in alpha %lf \n", alpha[0][i] );
        alpha[0][i] = (l[index]->pi)[i] * (l[index]->B)[i][(int)O[0]];
        printf("in alpha %lf \n",alpha[0][i]);
    }

    c_array[0] += alpha[0][i];
    printf("in alpha%lf \n", c_array[0]);
  }

  //scale the first element of alpha;
  c_array[0] = 1/c_array[0];
  for (i = 0; i < N; i++) {
    alpha[0][i] = c_array[0] * alpha[0][i];
  }

  //compute the rest of alpha
  int t;
  int j;
  for (t = 1; t < T; t++) {
    c_array[t] = 0;
    for (i = 0; i<N; i++) {
      alpha[t][i] = 0;
      for (j = 0; j < N; j++) {
        alpha[t][i] += alpha[t-1][j] *(l[index]-> A)[i][j];
      }

      if (O != NULL) {
        alpha[t][i] = alpha[t][i] *(l[index]-> B)[i][(int)O[t]];
      }

      c_array[t] += alpha[t][i];
    }
    //scale alpha[t][i]
    c_array[t] = 1/c_array[t];
    for (i = 0; i < N; i++) {
      alpha[t][i] = c_array[t] * alpha[t][i];
    }
  }

 printf("end of alpha\n");
}

void beta_pass(lambda *lp, int N, int T, double *O, double beta[T][NSTATE], double c_array[]){
  printf("in beta \n");

  double **A = lp->A;
  double **B = lp->B;

  printf("T: %d\n",T);
  int i;
  for (i = 0; i < N; i++) {
    beta[T-1][i] = c_array[T-1];
    printf("beta: %lf\n", beta[T-1][i]);
  }
  int t;
  int j;

  for ( t = T-2; t >=0; t--) {
    for ( i = 0; i < N; i++) {
      beta[t][i] = 0;
      for ( j = 0; j < N; j++) {
        if (O != NULL) {
          beta[t][i]+= A[i][j] * B[j][(int)O[t+1]] * beta[t+1][j];
        }
      }

      //scale the beta;
      beta[t][i] = c_array[t] * beta[t][i];
    }
  }
}

void gamma_pass(int T, int N, lambda *lp, double *O, double alpha[T][NSTATE], double beta[T][NSTATE], double gamma[T][NSTATE], double digamma[T][NSTATE]){
  printf("in gamma \n");
  double **A = lp->A;
  double **B = lp->B;

  int i,j,t;
  double denom = 0;
  for ( t = 0; t<T-1; t++) {
    for ( i = 0; i < N; i++) {
      for ( j = 0; j < N; j++) {
        if (O != NULL) {

          denom += alpha[t][i] * A[i][j] * B[j][(int)O[t+1]] * beta[t+1][j];
          digamma[t][j] = alpha[t][i] * A[i][j] * B[j][(int)O[t+1]] * beta[t+1][j];

          /**
          printf("original digamma[%d][%d] = %lf\n",t,j ,digamma[t][j]);
          printf("alpha: %lf\n",alpha[t][i]);
          printf("A: %lf\n",A[i][j]);
          printf("B: %lf\n",B[j][(int)O[t+1]]);
          printf("beta: %lf\n",beta[t+1][j]);
          printf("denom: %lf\n",denom);
          **/
        }
      }
    }

    for ( i = 0; i < N; i++) {
      gamma[t][i] = 0;
        for( j = 0; j < N; j++) {
          digamma[t][j] = digamma[t][j]/denom;
          gamma[t][i] += digamma[t][j];
        }
    }
    denom = 0;
  }

}

void recalc(lambda *lp, int N, int M, int T, double *O, double gamma[T][NSTATE], double digamma[T][NSTATE]){
  printf("in recalc \n");

  double **A = lp->A;
  double **B = lp->B;
  double *pi = lp->pi;

  int i,j,t;
  double numer = 0;
  double denom = 0;
  //recalc pi
  for ( i = 0; i < N; i++) {
    pi[i] = gamma[0][i];
    printf("pi[%d] = %lf\n",i,pi[i]);
    printf("in recalc A \n");

    //recalc A
    for ( j = 0; j < N; j++) {
        for( t = 0; t < T-1; t++) {
          numer += digamma[t][j];
          denom += gamma[t][i];
        }
        printf("numer = %d\n",numer);
        printf("denom = %d\n",denom);
        if(!denom )break;
	  pthread_mutex_lock(&A_mutex);
        A[i][j] = numer/denom;
        pthread_mutex_unlock(&A_mutex);
    }

    printf("in recalc B \n");
    //recalc B
    for ( j = 0; j < M; j++) {
      printf("J is %d\n", j);
      for ( t = 0; t < T-1; t++) {
        printf("T is %d\n", t);
        if (O[t] == j) {
          numer+= gamma[t][i];
          printf("numer is %d\n", numer);
        }
        pthread_mutex_lock(&A_mutex);
        if (denom == 0) {
          denom = 1;
        }
        B[i][j] = numer/denom;
        printf("B[i][j] is %f\n", B[i][j]);
        pthread_mutex_unlock(&A_mutex);
      }
    }
  }

}

/*find the log probability of observing our data set given model lambda l*/
void log_prob(int T, double c_array[]) {
  printf("in logprob \n");
  int t;
  for ( t = 0; t < T; t++) {
    log_probn += log(c_array[t]);
  }
}

int main(int argc, char *argv[]) {
    int house_id = 1;
    int nmax_iter = 5;

    if (argc == 5) {
        house_id = atoi(argv[1]);
        ndevices = atoi(argv[2]);
        nmax_iter = atoi(argv[3]);
        T = atoi(argv[4]);
    } else if (argc != 1) {
        fprintf(stderr, "usage: <house_id (ie. 1, 2, 3...)> <number of devices> <max number of iterations><total time>\n");
        exit(1);
    }

    pthread_t load_thread_p;

    char filename_in[50];
    sprintf(filename_in,"redd_data/disagg_house_%d.csv%c", house_id,'\0');

    /* construct the argument for the load thread */
    load_arg_t la = (load_arg_t) ckMalloc(sizeof(*la));
    O_array = (double**)ckMalloc(sizeof(double)*(ndevices+1)*(T+1));

    int o;
    for ( o = 0; o < ndevices+1; o++) {
      O_array[o] = (double*)ckMalloc(sizeof(double)*(T+1));
    }

    la->filename = filename_in;
    printf("filename: %s \n",la->filename);
    la->T = T;

    if (pthread_create(&load_thread_p, NULL, load_data, la) != 0) {
        perror("pthread_create for load_data failed in main\n");
        exit(1);
    }

    /*initialize void pointers to hold output of threads*/
    void *pv;
    void *rv[ndevices];

    /*start thread that loads the CSV files into the array of observations O_array*/
    pthread_join(load_thread_p, &pv);

   /*initialize the number of blocks per thread, depending on total number of blocks in a queue and the total number of threads*/
    pthread_t device_p[ndevices];
    data_arg_t *dt = ckMalloc(sizeof(*dt) * ndevices);
    l = (lambda **)ckMalloc(sizeof(*l) * ndevices);
    /*initialize and join nthreads*/
    int i,j,t,q,n;
    for ( i = 0; i < ndevices; i++) {
      printf("start device %d \n", i);

      //initialize the arguments for each thread
      dt[i] = (data_arg_t)ckMalloc(sizeof(data_arg_t));
      dt[i]->N = NSTATE;
      dt[i]->i = i;
      dt[i]->M = MOBSRV;
      dt[i]->T = T;
      dt[i]->iter = 0;
      dt[i]->nmax_iter = nmax_iter;
      dt[i]->O = (double*)ckMalloc(sizeof(double) * (T+1));

      for ( j = 0; j < T+1; j++) {
          //printf("O_array[%d][%d]= %lf \n",i,j,O_array[i][j]);
          dt[i]->O = O_array[i];
          //printf("dt[i][%d]->O=%lf\n",j,*(dt[i]->O + j));
      }
      l[i] = (lambda*)malloc(sizeof(lambda));

      l[i]->A = (double**)ckMalloc(sizeof(double)*NSTATE*NSTATE);
      l[i]->B = (double**)ckMalloc(sizeof(double)*NSTATE*MOBSRV);
      l[i]->pi = (double*)ckMalloc(sizeof(double)*NSTATE);

      for ( n = 0; n < NSTATE; n++) {
        l[i]->A[n] = row_stochastic(l[i]->A[n], NSTATE);
        l[i]->B[n] = row_stochastic(l[i]->B[n], MOBSRV);
      }

      l[i]->pi = row_stochastic(l[i]->pi, NSTATE);

      dt[i]->l = l[i];

      dt[i]->alpha = ckMalloc(sizeof(double)*T*NSTATE);
      dt[i]->beta = ckMalloc(sizeof(double)*T*NSTATE);
      dt[i]->c_array = ckMalloc(sizeof(double)*T);
      dt[i]->gamma = ckMalloc(sizeof(double)*T*NSTATE);
      dt[i]->digamma = ckMalloc(sizeof(double)*T*NSTATE);

      for ( t = 0; t < T; t++) {
        dt[i]->alpha[t] = ckMalloc(sizeof(double)*NSTATE);
        dt[i]->beta[t] = ckMalloc(sizeof(double)*NSTATE);
        dt[i]->gamma[t] = ckMalloc(sizeof(double)*NSTATE);
        dt[i]->digamma[t] = ckMalloc(sizeof(double)*NSTATE);
      }

      if (pthread_create(&device_p[i], NULL, hmm_calc, dt[i]) != 0) {
          perror("pthread_create for device_p failed in main\n");
          exit(1);
      }
      pthread_join(device_p[i], &rv[i]);
	  printf("End of device_p\n");
    }
    free(O_array);
    free(la);

    /*Write lambdas to a file*/
    FILE *fp;
    char filename_out[30];
    sprintf(filename_out, "house_%d_hmm_output.txt%c", house_id,'\0');
    fp = fopen(filename_out, "w");

    for ( i = 0; i < ndevices; i++) {
      for ( q = 0; q < NSTATE; q++) {
        print_array((l[i]->A)[q], NSTATE, fp);
      }
      fprintf(fp, "\n, A, \n");
      for( q = 0; q < NSTATE; q++) {
        print_array((l[i]->B)[q], MOBSRV, fp);
      }
      fprintf(fp, "\n,B,\n");
      print_array(l[i]->pi, NSTATE, fp);
      fprintf(fp, "\n,pi,\n");
    }
    fclose(fp);


    for ( i = 0; i < ndevices; i++) {
        //free(l[i]->A);
        //free(l[i]->B);
        //free(l[i]->pi);
        free(l[i]);
        //free(dt[i]);
    }
    pthread_exit(NULL);

}