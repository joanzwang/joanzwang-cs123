import sys

def main(argv):

  if len(argv) < 3:
    sys.stderr.write("Usage: <filename test output> <filename real disagg output> <filename real agg output>")
    return 1

  test_out_f = sys.argv[1]
  real_out_f = sys.argv[2]
  real_agg_f = sys.argv[3]

  ft = open (test_out_f, 'r')
  fr = open(real_out_f, 'r')
  fa = open(real_agg_f, 'r')

  acc = 0;
  num = 0;
  den = 0;
  y_t = []
  y_r = []
  y_bar = []

  line_t =  ft.readlines()
  line_r = fr.readlines()
  line_a = fa.readlines()

  for line in ft.readlines():
    y_t.append(line)
  for line in fr.readlines():
    y_r.append(line)
  for line in fa.readlines():
    y_bar.append(line)

  for t in range(1,len(y_r[0])):
    for i in xrange(len(y_r)-1):
      num += abs(y_t[t][i]-y_r[t][i])
    den += y_bar[1][t]

  acc = 1-num/(2 * den)

  print acc

  ft.close()
  fr.close()
  fa.close()

if __name__ == "__main__":
  main(sys.argv[1:])