from sklearn import hmm
import numpy as np
from scipy.optimize import fmin
import sys

def open_test(test_id):
  test_data = []
  time = []
  O = []
  test_data = [time, O]

  ftest = open("redd_data/agg_house_"+test_id+".csv")
  for line in fo.readlines():
    data_list = line.split(",");
    for data in data_list:
      while data is not "\n":
        time.append(data)
      O.append(data)
    fo.close()
  return test_data

def open_train(train_id):

  A = []
  B = []
  pi= []

  model = [A, B, pi]

  ftrain = open("house_"+i+"_hmm_output.txt", 'r');

  for line in fo.readlines():
    data_list = line.split(" ");
    for data in data_list:
      i = 0
      while data is not None:
        while (data is not "a"):
          A[i].append(line.split(" "))
        while(data is not "b"):
          B[i].append(line.split(" "))
        while(data is not "pi"):
          pi[i].append(line.split(" "))
        i += 1
    fo.close()
  return model

def get_params(models_arr):
  A_arr = models_arr[0]
  #B_arr = models_arr[1]
  pi_arr = models_arr[2]

  mu_arr = []   #means matrix
  for i in len(A_arr[0]):
    mu = []
    for j in len(A_arr):
      mu[i] += A_arr[i][j]
    mu[i] += pi[i]
    mu[i] = mu[i]/len(A_arr)
    mu_arr.append(mu)

  return mu_arr

def afamap(mu_arr, models_arr, test_data, T):
  A_arr = models_arr[0]
  B_arr = models_arr[1]
  last_sum = 100;  #magic max number for minimization problem
  thresh = 0.5 #magic threshhold number to determine number of iterations
  max_iter = 5 #magic number to determine max iterations
  num_iter = 0
  last_dif = 0
  add = True
  repeat = True

  predict_arr = B_arr

  while(repeat):
    #first part of the afamap algorithm
    first_sum = 0
    sum_mu_Q = 0
    for t in T:
      for i in len(A_arr):
        for j in len(A_arr[0]):
          sum_mu_Q += mu_arr[i][j] * predict_arr[i][j][t]
      first_sum = (test_data[t] - sum_mu_Q)**2
    first_sum = first_sum * 0.5

    second_sum = 0
    for t in T:
      for i in len(A_arr):
        for j in len(A_arr[0]):
          for k in len(A_arr[0]):
            if (k is not j):
              second_sum += (test_data[t]-test_data[t-1]-(mu_arr[i][k]-mu_arr[i][j]))**2 * (predict_arr[i][j][t-1]-predict_arr[i][k][t])
    second_sum = second_sum/2

    third_sum = 0
    for t in T:
      for i in len(A_arr):
        for j in len(A_arr[0]):
          third_sum += (predict_arr[i][j][t-1]-predict_arr[i][k][t])
    third_sum = 0.5*(1-third_sum)

    fourth_sum = 0
    for t in T:
      for i in len(A_arr):
        for j in len(A_arr[0]):
          for k in len(A_arr[0]):
            fourth_sum += (predict_arr[i][j][t-1]-predict_arr[i][k][t]) * (-1) * log(B_arr[i][k][j])


    total_sum = first_sum + second_sum + third_sum + fourth_sum
    num_iter += 1

    if total_sum-last_sum <= thresh and num_iter > max_iter:
      if total_sum-last_sum > last_dif:
        add = not add
      predict_arr = optimize(predict_arr, add)
      repeat = False

  result_arr = [[] for i in xrange(len(A_arr[0]))]
  for t in T:
    for i in len(A_arr[0]):
      result_arr[i].append(predict_arr[t] * mu[i][t])
  return result_arr

#really bad unmathematically sound method for handling optimization
def optimize(predict_arr, add):
  for row in predict_arr:
    for element in row:
      if add is True:
        element += random.uniform(0, 450)
      else:
        element -= random.uniform(0, 450)
  return predict_arr

def write_to_file(q_arr):
  fo = open("output/fhmm_"+test_id+".csv", "w")
  for q in q_arr:
    fo.write(q + ",")
  fo.close()

def main(argv):
  if len(argv) < 3:
    sys.stderr.write("Usage: <id of house for testing><id of house for training><time T>" )
    return 1

  test_id = sys.argv[1]
  train_id = sys.argv[2]
  T = sys.argv[3]

  test_data = open_test(test_id)
  models_arr = open_train(train_id)
  mu = []
  q_arr = []

  mu_arr = get_params(models_arr)
  q_arr = afamap(mu_arr, models_arr, test_data, T)
  write_to_file(q_arr)

if __name__ == "__main__":
  main(sys.argv[1:])


