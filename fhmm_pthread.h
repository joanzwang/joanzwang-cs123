#ifndef SHORTEN_H
#define SHORTEN_H
#include <stdbool.h>
#include <assert.h>


/*hmm model to be returned*/
typedef struct lambda{
  double **A;
  double **B;
  double *pi;
} lambda;

/*contains data for building passes*/
typedef struct{
  //int house_id;
  int N;
  int M;
  int T;
  int iter;
  int nmax_iter;
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
    double **O_house;
    int T;
}*load_arg_t;

/*initial array population, making sure that all rows add up to 1*/
double *row_stochastic(double *array, int size);

/* load_queue: start function for the thread that will fill the queue */
void *load_data(void *arg);

/*compute the alpha array for this hmm model*/
void *hmm_calc(void *arg);

/*compute the alpha array for this hmm model*/
void alpha_pass(double *c_array, int N, double **alpha, lambda *l, double *O, int T);

void beta_pass(lambda *l, int N, int T, double *O, double **beta, double *c_array);

void gamma_pass(int T, int N, lambda *l, double *O, double **alpha, double **beta, double **gamma, double **digamma);

void recalc(lambda *l, int N, int M, int T, double *O, double **gamma, double **digamma);

/*find the log probability of observing our data set given model lambda l*/
double log_prob(int T, double *c_array);

bool calc_iterate(int iter, int nmax_iter, double n_log_prob, double o_log_prob);



#endif
