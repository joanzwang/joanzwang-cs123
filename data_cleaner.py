#quick program to get data into format that I want, with all devices per house in one file, and then the aggregated energy output per house in another file
def main():
  for i in range(1, 7):
    print "i is " + str(i)
    labels = []

    fo = open("redd_data/low_freq/house_"+str(i)+"/labels.dat", "r");
    fwd = open("redd_data/disagg_house_"+str(i)+".csv","w")
    fwa = open("redd_data/agg_house_"+str(i)+".csv", "w")
    
    #read the labels of the different devices in the house, to be used as the first element of each row
    for line in fo.readlines():
      labels.append(line.strip("\n"));

    fo.close();
    channels = len(labels)

    agg_val = [] #list of floats which are sums of output for all channels per timestamp
    for k in range(1, channels):
      j = k-1
      time_line = "timestamp,"
      disagg_line = labels[j] + ","
      print "j is : " + str(j)
      fo = open("redd_data/low_freq/house_" + str(i)+"/channel_"+str(k)+".dat", "r")

      #append the values to the line disagg_val for the disaggregated file
      #add the values to agg_val for the aggregated file
      for line in fo.readlines():
          t = 0
          time_output = line.split()
          time_line += time_output[0] + ","
          disagg_line += time_output[1] + ","
          if j is 0:
            agg_val.append(float(time_output[1]))
          else:
            agg_val[t] += (float(time_output[1]))
          t += 1

      time_line += "\n"
      disagg_line += "\n"
      agg_line = "output,"
      for val in agg_val:
        agg_line += str(val) + ","
      agg_line += "\n"

      if j is 0:
        fwd.write(time_line)
        fwa.write(time_line)
      fwd.write(disagg_line)
    fwa.write(agg_line)

    fo.close()
    fwd.close()
    fwa.close()

if __name__ == "__main__":
  main()