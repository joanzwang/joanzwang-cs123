import itertools
from collections import OrderedDict
import csv
from mrjob.job import MRJob
import sys
import argparse
import os

#use name of file generated from knn_file_setup.py as first argument
class Knn_model(MRJob):

    #init name takes the data from the files specified and puts them into their digestible formats
    def init_map(self):
        
        #my program is unable to run because of this section: I need to input multiple files
        #but mrjob won't recognize these files. I haven't deleted the commented out code
        #because I am still not sure which method would work
        self.test_folder = sys.argv[0]
        self.train_files = []
        for i in range(1,7):
          self.train_files.append("redd_data/disagg_house_"+str(i)+".csv")

        #filename_arr
        #for line in csv.reader(open("redd_data/disagg_house_1.csv", 'r')):
          #self.train_files.append(line)
        '''
        self.train_files.append(sys.argv[2])
        self.train_files.append(sys.argv[3])
        self.train_files.append(sys.argv[4])
        self.train_files.append(sys.argv[5])
        '''
        #self.ndevices = int(sys.argv[6])

        #hardcode for now
        #self.train_files.append(redd_data/disagg_house_1.csv)
        self.ndevices = 20
        #self.test_id = 3
        #self.ndevices = 20

        #get labels that are valid for this case
        self.labels = []
        fl = open(self.test_folder, 'r')
        for line in fl.readlines():
          if line[0] not in "timestamp\n":
            self.labels.append(line[0])

        fl.close()

        self.test_data = [[[] for j in xrange(self.ndevices)] for i in xrange(6)]
        for filename in self.train_files:
          ftrain = open(filename,"r")
          for line in ftrain:
            line = line.split(",")
            for label in labels:
              if line[0] in label:
                test_data[i].append(line[1:])
        self.line_arr = []

    def create_map(self, _, line):
        print "create map"
        self.line_arr = line.split(",");

    #final map compares the line of data (the block of aggregated data of the testing house)
    #and find the euclidean distance between this block and every block of the same size
    #in the training data sets
    def final_map(self):
        print "final map"
        sum = 0
        value = []
        key = self.line_arr[0]

        for data in self.line_arr[1:]:
          if data.isdigit():
            sum += float(data)

        for i in range(1,7):

          current_min = sum**sum
          for j in xrange(self.ndevices):
            k =(len(self.line_arr))
            n = 0

            while self.test_data[i][j][k+n].isdigit():
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

    #init reducer creates a vector of predicted observations, to be written to a file
    def init_reducer(self):
        self.predicted_obs = []

    #reducer averages the disaggregated data values for each key
    def create_reducer(self, key, value):
        avg = 0
        for k in range(1,len(self.line_arr)+2):
          avg = sum(value[k])/len(self.line_arr)
          self.predicted_obs[key] = avg

    #final reducer writes key and values to file before
    def final_reducer(self):
        fo = open("knn_file/predicted_sequence_house_"+self.test_id+".csv", 'w')
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
    '''
    def configure_options(self):
      super(Knn_model, self).configure_options()
      self.add_file_option('--scoring-db')
    '''
    '''
    parser = argparse.ArgumentParser()
    parser.add_argument('--param', '-p', type=int)
    parser.add_argume
    parser.add_argument('files', nargs='+', type=str)

    args=parser.parse_args()
    model = Knn_model(args=args.files)
    model.execute()
    '''

    Knn_model.run()


