#import numpy as np
import itertools
from collections import OrderedDict
import csv
from mrjob.job import MRJob
import sys
import argparse

#use name of file generated from knn_file_setup.py
class Knn_model(MRJob):

    def init_map(self):
        print "map init"
        self.test_id = sys.argv[1]
        self.ndevices = sys.argv[2]
        #hardcode for now
        self.train_folder = sys.argv[3]
        self.test_folder = sys.argv[4]
        #self.test_id = 3
        #self.ndevices = 20

        #get labels that are valid for this case
        self.labels = []
        fl = open(test_folder, 'r')
        for line in fl.readlines():
          if line[0] not in "timestamp\n":
            self.labels.append(line[0])

        fl.close()

        self.test_data = [[[] for j in xrange(self.ndevices)] for i in xrange(6)]
        for folder in self.train_folder:
          for i in range(1, 7):
            if i is not test_id:
              ftrain = open(folder,"r")

              for line in ftrain:
                line = line.split(",")
                for label in labels:
                  if line[0] in label:
                    test_data[i].append(line[1:])
        self.line_arr = []

    def create_map(self, _, line):
        print "create map"
        self.line_arr = line.split(",");

    def final_map(self):
        print "final map"
        sum = 0
        value = []
        key = self.line_arr[0]

        for data in self.line_arr[1:]:
          sum += float(data)

        for i in range(1,7):

          current_min = sum**sum #magic
          for j in xrange(self.ndevices):
            k =(len(self.line_arr)-1)
            n = 0
            while self.test_data[i][j][k+n] is not None:
              train_sum = 0
              for l in xrange(k):
                train_sum += (self.test_data[i][j][k+n])
                n += 1
              temp_dist = abs(sum - train_sum)
              if temp_dist < current_min:
                value.append(train_sum)
                value.append(self.test_data[i][j][n-k:n+1])
                current_min = temp_dist

        yield key, value

    def init_reducer(self):
        self.predicted_obs = []

    def create_reducer(self, key, value):
        avg = 0
        for k in range(1,len(self.line_arr)+2):
          avg = sum(value[k])/len(self.line_arr)
          self.predicted_obs[key] = avg

    def final_reducer(self):
        fo = open("knn_file/predicted_sequence_house_"+self.test_id, 'w')
        fo.write(predicted_obs)
        fo.close()
        yield key, predicted_obs

    def steps(self):
        return [
            self.mr(mapper_init = self.init_map,
                    mapper = self.create_map,
                    mapper_final = self.final_map,
                    reducer_init = self.init_reducer,
                    reducer = self.create_reducer,
                    reducer_final = self.final_reducer)
        ]

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--param', '-p', type=int)
    parser.add_argument('files', nargs='+', type=str)

    args=parser.parse_args()
    Knn_model.run(args=args.files)


