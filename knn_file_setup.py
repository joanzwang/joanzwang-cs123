import sys

#quick program to get data into format for knn map reduce job, with aggregated test data
#broken into blocks of (granularity) length, repeated for the number of houses in the data set
def main(argv):
  if len(argv) < 4:
    sys.stderr.write("Usage: <test house id number> <(int) granularity> <num devices for test house><time 745878>")
    return 1

  test_id = sys.argv[1]
  granularity = int(sys.argv[2])
  ndevices = int(sys.argv[3])
  time = int(sys.argv[4])

  ftest = open("redd_data/agg_house_"+str(test_id)+".csv", 'r')
  fwrite = open("knn_file/input_house_"+str(test_id)+".csv", 'w')

  data_array = [[] for i in xrange(time)]
  for line in ftest.readlines():
    line = line.split(",")
    i = 0
    if line[0] not in "timestamp\n":
      print line[0]
      for data in line[1:]:
        data_array[i/granularity].append(data)  #break the data into blocks of size (granularity)
        i += 1
  k = 0
  for row in data_array:

    for i in range(0, 5): #magic number, there are five testing houses
      fwrite.write(str(k) + ",")
      fwrite.write(str(row))
      fwrite.write("\n")
    k += 1
  ftest.close()
  fwrite.close()

if __name__ == "__main__":
  main(sys.argv[1:])