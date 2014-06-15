#ifndef SHORTEN_H
#define SHORTEN_H
#include <stdbool.h>
#include <assert.h>
#define NSTATE 4
#define MOBSRV 450

/*hmm model to be returned*/
typedef struct {
  double **A;
  double **B;
  double *pi;
} lambda;

/*contains data for building passes*/
typedef struct{
  int N;
  int M;
  int T;
  int iter;
  int nmax_iter;
  int i;
  double *O; //the observations
  lambda *l;
  double **alpha;
  double **beta;
  double *c_array;
  double **gamma;
  double **digamma;
  double log_prob_old;
} *data_arg_t;


/* type for argument to load_queue */
typedef struct{
    char *filename;
    int T;
}*load_arg_t;

/*prints specified array into a file*/
void print_array(double *array, int size, FILE *fp);

/*initial array population, making sure that all rows add up to 1*/
double *row_stochastic(double *array, int size);

/* load_queue: start function for the thread that will fill the queue */
void *load_data(void *arg);

/*compute the alpha array for this hmm model*/
void *hmm_calc(void *arg);

/*compute the alpha array for this hmm model*/
//void alpha_pass(int index, double c_array[], int N, double **alpha, double *O, int T);

/*compute the beta array for this hmm model*/
//void beta_pass(lambda *lp, int N, int T, double *O, double **beta, double c_array[]);

/*compute the gamma and digamma arrays for this hmm model*/
//void gamma_pass(int T, int N, lambda *lp, double *O, double **alpha, double **beta, double **gamma, double **digamma);

/*recalculate the parameters of the model*/
//void recalc(lambda *lp, int N, int M, int T, double *O, double **gamma, double **digamma);

/*find the log probability of observing our data set given model lambda l*/
void log_prob(int T, double c_array[]);



#endif