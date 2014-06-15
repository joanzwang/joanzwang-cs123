import itertools
from collections import OrderedDict
import csv
from mrjob.job import MRJob
import sys
import argparse
import os

#use name of file generated from knn_file_setup.py as first argument
class Knn_model(MRJob):
    OUTPUT_PROTOCOL = JSONValueProtocol

    def create_map(self, _, line):
	print "create map"
	self.line_arr = line.split(",");
        granularity = 4 #hardcoded, change to argparse later
	sum = 0
	value = []
	key = self.line_arr[0]

        test_sum = 0
	for data in self.line_arr[1:granularity+1]:
	  if data.isdigit():
	    test_sum += float(data)
        current_min = sum**sum
        for i in xrange(len(line_arr)-granularity-1):
          for j in xrange(granularity):
            block_sum = 0
            block = []
            while self.line_arr[i+j].isdigit():
  	      block_sum += self.line_arr[i+j]
  	      block.append(self.line_arr[i+j])
  	      temp_dist = abs(test_sum - block_sum)
  	      if temp_dist < current_min:
  		value.append(train_sum)
  		value.append(block)
  		current_min = temp_dist

	yield key, value

    #reducer averages the disaggregated data values for each key
    def create_reducer(self, key, value):
       	self.predicted_obs = []
        avg = 0
	for k in range(1,len(self.line_arr)+2):
	  avg = sum(value[k])/len(self.line_arr)
	  self.predicted_obs[key] = avg
	yield key, predicted_obs

    def steps(self):
	return [
	    self.mr(mapper = self.create_map,
		    reducer = self.create_reducer)
	]

if __name__ == '__main__':
    Knn_model.run()


