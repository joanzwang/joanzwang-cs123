import sys

#quick program to get data into format for knn map reduce job, with aggregated test data
#broken into blocks of (granularity) length, repeated for the number of houses in the data set
def main(argv):
  if len(argv) < 5:
    sys.stderr.write("Usage: <test house id number> <(int) granularity> <num devices for test house><time 745878><time to limit run by ie. 5>")
    return 1

  test_id = sys.argv[1]
  granularity = int(sys.argv[2])
  ndevices = int(sys.argv[3])
  time = int(sys.argv[4])
  max_run = int(sys.argv[5])

  flabels = open("redd_data/disagg_house_"+str(test_id)+".csv", 'r')
  labels = []

  for line in flabels.readlines():
    line_arr = str(line).split(",");
    if line_arr[0] not in "timestamp\n":
      name = line_arr[0].split()
      labels.append(name[1])
  flabels.close()
  print labels

  ftest = open("redd_data/agg_house_"+str(test_id)+".csv", 'r')
  fwrite = open("knn_file/input_house_"+str(test_id)+".csv", 'w')
  data_array = [[] for i in xrange(time)]

  for line in ftest.readlines():
    line_arr = line.split(",")
    i = 0
    if line_arr[0] not in "timestamp\n":
      for data in line_arr[1:]:
        data_array[i/granularity].append(data)  #break the data into blocks of size (granularity)
        i += 1

  train_data_arr = []
  for i in range(1, 7):
    if i is not test_id:
      ftrain = open("redd_data/disagg_house_"+str(i)+".csv", 'r')
      train_data = []
      for line in ftrain:
        line = line.split(",")
        name = line[0].split()
        for label in labels:
          if name[0] not in "timestamp":
            if name[1] in label:
              train_data.append(line[1:])

      train_data_arr.append(train_data)
      ftrain.close()
      print i
  print "DoNE"

  k = 0
  for row in data_array:
    while k <= max_run:
      print "row"
      for i in range(0, 5): #magic number, there are five testing houses
        fwrite.write(str(k) + ",")
        fwrite.write(str(row))
        print "i2: " + str(i)
        for house in train_data_arr:
          print "house"
          time_len = []
          for device in house:
            time_len.append(len(device))
          min_time = min(time_len)
          print min_time
          for t in xrange(min_time):
            for device in house:
              if device[t] is not None:
                fwrite.write(device[t])
                fwrite.write(",")
  
        fwrite.write("\n")
      k += 1
      print "added to k so " + str(k)
  ftest.close()
  fwrite.close()

if __name__ == "__main__":
  main(sys.argv[1:])