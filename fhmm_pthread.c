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

/*prints specified array into a file*/
void print_array(double *array, int size, FILE *fp) {
  for (int i = 0; i < size; i++) {
    fprintf(fp, "%7.2f", array[i]);
  }
}

/*initialize array population, making sure that all rows add up to 1*/
double *row_stochastic(double *array, int size) {
  array = ckMalloc(sizeof(double) * size);
  double even = (double)1/size;
  for (int i = 0; i < size; i++) {
    array[i] = even;
  }

  return array;
}


/* load_queue: start function for the thread that will fill the queue */
void *load_data(void *arg) {
    assert(arg != NULL);
    load_arg_t la = (load_arg_t) arg;
    double **O_house = la->O_house;

    char *s;

    FILE *f = fopen(la->filename, "r");
    if (f == NULL) {
        fprintf(stderr, "Unable to open file %s\n", la->filename);
        exit(1);
    }
    la->T = 0;
    printf("filename %s\n", la->filename);
    while ((s = readLine(f)) != NULL) {
        char *token = strtok(s, ",");
        int i = 0;
        //for every line in the file that isn't the time, so for every device
        //write the energy output of the device into the array O_house
        if (strcmp(token,"timestamp") != 0) {

            printf("token %s ", token);
            la->T = 0;
            while(token !=NULL) {
              token = strtok(NULL, ",");
              if (token != NULL) {
                double temp = strtod(token, NULL);
                O_house[i][la->T] = temp;
                la->T++;
              }
            }
            i++;
        }
    }
    pthread_exit(NULL);
}

/*compute the alpha array for this hmm model*/
void *hmm_calc(void *arg) {
  assert(arg != NULL);
  data_arg_t d = (data_arg_t) arg;
  lambda *l = d->l;
  int N = d->N;
  int M = d->M;
  int T = d->T;

  double *O = d->O;
  double **alpha = d->alpha;
  double **beta = d->beta;
  double *c_array = d->c_array;
  double **gamma = d->gamma;
  double **digamma = d->digamma;
  double log_prob_old = d->log_prob_old;

  //perform the alpha and beta passes to determine arrays alpha and beta
  alpha_pass(c_array, N, alpha, l, O, T);
  beta_pass(l, N, T, O, beta, c_array);
  
  //perform gamma_pass, which uses alpha and beta matrices to determine gamma and digamma
  gamma_pass(T, N, l, O, alpha, beta, gamma, digamma);

  //recalculate the components of the model based off of gamma and digamma
  recalc(l, N, M, T, O, gamma, digamma);

  //calculate the log likelihood of the observations given our model, and decide whether or not to iterate
  double log_prob_new = log_prob(T, c_array);
  d->iter++;
  bool iterate = calc_iterate(d->iter, d->nmax_iter, log_prob_new, log_prob_old);

  if (iterate) {
    d->log_prob_old = log_prob_new;
    hmm_calc(d);
  }

  free(alpha);
  free(beta);
  free(c_array);
  free(gamma);
  free(digamma);
  pthread_exit(NULL);

}

/*compute the alpha array for this hmm model*/
void alpha_pass(double *c_array, int N, double **alpha, lambda *l, double *O, int T) {
  printf("in alpha \n");

  double **A = l->A;
  double **B = l->B;
  double *pi = l->pi;

  c_array[0] = 0;
  for (int i = 0; i < N; i++) {
    if (O != NULL) {
      alpha[0][i] = pi[i] * B[i][(int)O[0]];
    }
    c_array[0] += alpha[0][i];
  }

  //scale the first element of alpha;
  c_array[0] = 1/c_array[0];
  for (int i = 0; i < N; i++) {
    alpha[0][i] = c_array[0] * alpha[0][i];
  }

  //compute the rest of alpha
  for (int t = 1; t < T; t++) {
    c_array[t] = 0;
    for (int i = 0; i<N; i++) {
      alpha[t][i] = 0;
      for (int j = 0; j < N; j++) {
        alpha[t][i] += alpha[t-1][j] * A[i][j];
      }

      if (O != NULL) {
        alpha[t][i] = alpha[t][i] * B[i][(int)O[t]];
      }

      c_array[t] += alpha[t][i];
    }
    //scale alpha[t][i]
    c_array[t] = 1/c_array[t];
    for (int i = 0; i < N; i++) {
      alpha[t][i] = c_array[t] * alpha[t][i];
    }
  }
}

void beta_pass(lambda *l, int N, int T, double *O, double **beta, double *c_array){
  printf("in beta \n");

  //lambda la = (lambda) l;
  double **A = l->A;
  double **B = l->B;

  for (int i = 0; i < N; i++) {
    beta[T-1][i] = c_array[T-1];
  }

  for (int t = T-2; t >=0; t--) {
    for (int i = 0; i < N; i++) {
      beta[t][i] = 0;
      for (int j = 0; j < N; j++) {
        if (O != NULL) {
          beta[t][i]+= A[i][j] * B[j][(int)O[t+1]] * beta[t+1][j];
        }
      }

      //scale the beta;
      beta[t][i] = c_array[t] * beta[t][i];
    }
  }
}

void gamma_pass(int T, int N, lambda *l, double *O, double **alpha, double **beta, double **gamma, double **digamma){

  printf("in gamma \n");
  double **A = l->A;
  double **B = l->B;

  for (int t = 0; t<T-1; t++) {
    int denom = 0;
    for (int i = 0; i < N; i++) {
      for (int j = 0; j < N; j++) {
        if (O != NULL) {
          denom += alpha[t][i] * A[i][j] * B[j][(int)O[t+1]] * beta[t+1][j];
        }
      }
    }

    for (int i = 0; i < N; i++) {
      gamma[t][i] = 0;
        for(int j = 0; j < N; j++) {
          if (O != NULL) {
            digamma[t][j] = alpha[t][i] * A[i][j] * B[j][(int)O[t+1]] * beta[t+1][j];
          }
          digamma[t][j] = digamma[t][j]/denom;
          gamma[t][i] += digamma[t][j];
        }
    }
  }
}

void recalc(lambda *l, int N, int M, int T, double *O, double **gamma, double **digamma ){
  printf("in recalc \n");

  double **A = l->A;
  double **B = l->B;
  double *pi = l->pi;

  //recalc pi
  for (int i = 0; i < N; i++) {
    pi[i] = gamma[0][i];

    //recalc A
    for (int j = 0; j < N; j++) {
      int numer = 0;
      int denom = 0;
        for(int t = 0; t < T-1; t++) {
          numer += digamma[t][j];
          denom += gamma[t][i];
        }
      A[i][j] = numer/denom;
    }

  //recalc B
    for (int j = 0; j < M; j++) {
      int numer = 0;
      int denom = 0;
      for (int t = 0; t < T-1; t++) {
        if (O[t] == j) {
          numer+= gamma[t][i];
        }

        if (!(denom += gamma[t][i])) {
          return;
        }

        B[i][j] = numer/denom;
      }
    }
  }

}

/*find the log probability of observing our data set given model lambda l*/
double log_prob(int T, double *c_array) {
  printf("in logprob \n");
  int log_prob = 0;
  for (int t = 0; t < T; t++) {
    log_prob += log(c_array[t]);
  }

  return -log_prob;
}

bool calc_iterate(int iter, int nmax_iter, double n_log_prob, double o_log_prob) {
  printf("in calc \n");
  if (iter < nmax_iter && n_log_prob > o_log_prob) {
    return true;      //keep on iterating
  }

  return false; //return final lambda after this
}

int main(int argc, char *argv[]) {
    int house_id = 1;
    int ndevices = 19;
    int T = 99999999;
    int nmax_iter = 5;


    if (argc == 5) {
        house_id = atoi(argv[1]);
        ndevices = atoi(argv[2]);
        T = atoi(argv[3]);
        nmax_iter = atoi(argv[4]);
    } else if (argc != 1) {
        fprintf(stderr, "usage: <house_id (ie. 1, 2, 3...)> <number of devices> <total time> <max number of iterations>\n");
        exit(1);
    }


    pthread_t load_thread_p;

    char filename_in[28];
    sprintf(filename_in,"redd_data/disagg_house_%d.csv", house_id);

    /* construct the argument for the load thread */
    load_arg_t la = (load_arg_t) ckMalloc(sizeof(*la));
    double **O_array = (double**)ckMalloc(sizeof(double)*(ndevices+1)*(T+1));
    for (int o = 0; o < ndevices+1; o++) {
      O_array[o] = (double*)ckMalloc(sizeof(double)*T+1);
    }
    la->O_house = O_array;
    la->filename = filename_in;
    la->T = T;

    if (pthread_create(&load_thread_p, NULL, load_data, la) != 0) {
        perror("pthread_create for load_data failed in main\n");
        exit(1);
    }

    /*initialize void pointers to hold output of threads*/
    void *pv;
    void *rv[ndevices];

    printf("point a \n");

    /*start thread that loads the CSV files into the array of observations O_array*/
    pthread_join(load_thread_p, &pv);
    printf("here\n");
    T = la->T;
    /*initialize the number of blocks per thread, depending on total number of blocks in a queue and the total number of threads*/
    pthread_t device_p[ndevices];
    data_arg_t *dt = ckMalloc(sizeof(*dt) * ndevices);

    lambda **l = ckMalloc(sizeof(*l) * ndevices);
    /*initialize and join nthreads*/
    for (int i = 0; i < ndevices; i++) {
      printf("start device %d \n", i);
      //initialize the arguments for each thread
      dt[i] = (data_arg_t) ckMalloc(sizeof(*dt));
      dt[i]->N = NSTATE;
      dt[i]->M = MOBSRV;
      dt[i]->T = T;
      dt[i]->iter = 0;
      dt[i]->nmax_iter = nmax_iter;
      double *obs = (double*)ckMalloc(sizeof(double) * T+2);
      for (int j = 0; j < T+1; j++) {
          obs[j] = O_array[i][j];
      }

      dt[i]->O = obs;

      l[i] = (lambda*)ckMalloc(sizeof(lambda));

      l[i]->A = (double**)ckMalloc(sizeof(double)*NSTATE*NSTATE);
      l[i]->B = (double**)ckMalloc(sizeof(double)*NSTATE*MOBSRV);
      l[i]->pi = (double*)ckMalloc(sizeof(double)*NSTATE);
      for (int n = 0; n < NSTATE; n++) {
        l[i]->A[n] = row_stochastic(l[i]->A[n], NSTATE);
        l[i]->B[n] = row_stochastic(l[i]->B[n], MOBSRV);
        printf("row stochastic %d \n", n);
      }

      l[i]->pi = row_stochastic(l[i]->pi, NSTATE);

      dt[i]->l = l[i];

      dt[i]->alpha = ckMalloc(sizeof(double)*T*NSTATE);
      dt[i]->beta = ckMalloc(sizeof(double)*T*NSTATE);
      dt[i]->c_array = ckMalloc(sizeof(double)*T);
      dt[i]->gamma = ckMalloc(sizeof(double)*T*NSTATE);
      dt[i]->digamma = ckMalloc(sizeof(double)*T*NSTATE);

      for (int t = 0; t < T; t++) {
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
    }
    free(O_array);
    free(la);

    /*Write lambdas to a file*/
    FILE *fp;
    char filename_out[30];
    sprintf(filename_out, "house_%d_hmm_output.txt", house_id);
    fp = fopen(filename_out, "w");

    for (int i = 0; i < ndevices; i++) {
      for (int q = 0; q < NSTATE; q++) {
        print_array((l[i])->A[q], NSTATE, fp);
      }
      fprintf(fp, "\nA\n");
      for (int q = 0; q < NSTATE; q++) {
        print_array(l[i]->B[q], MOBSRV, fp);
      }
      fprintf(fp, "\nB\n");

      print_array(l[i]->pi, NSTATE, fp);

      fprintf(fp, "\npi\n");

    }
    fclose(fp);


    for (int i = 0; i < ndevices; i++) {
        free(dt[i]);
    }
    pthread_exit(NULL);

}
