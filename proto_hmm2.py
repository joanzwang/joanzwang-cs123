from sklearn import hmm
import numpy as np
import itertools
from collections import OrderedDict
import csv
from mrjob.job import MRJob

#r'C:\Users\Acer\Documents\Terrance\UChicago\3rd Year\SPRING\CS123\Project\home2_data_transpose.csv'
class MRfhmm(MRJob):

    #list_x can be list Pi's or list of transmat's
    def compute_fhmm_x(self, list_x):
        if (list_x is None):
            print "this is not good"
            return 
            
        else:
            print "HII"
            print len(list_x)
            rv = list_x[0]
            for i in range(1, len(list_x)):
                rv = np.kron(rv, list_x[i])
            return rv

    def compute_fhmm_means_cov(self, list_means):
        #list of all combination of products of means
        combination_of_states = list(itertools.product(*list_means))
        number_of_combinations = len(combination_of_states)
        means = np.reshape(np.array([sum(x) for x in combination_of_states]), (number_of_combinations, 1))
        cov = np.tile(5*np.identity(1), (number_of_combinations, 1, 1))
        return [means, cov]

    def init_create_hmm(self):
        self.datasheet = []
        self.hmm_models = OrderedDict()

    def create_hmm(self, _, line):
        line_arr = line.split(",");
        fix_line = [line_arr[0]] + [float(x) for x in line_arr[1:]]
        self.datasheet.append(fix_line)

    def final_create_hmm(self):
        for i in range(1, len(self.datasheet)):
            print "fitting currently" + str(i)
            X = np.column_stack([self.datasheet[0][1:], self.datasheet[i][1:]])
            key = self.datasheet[i][0]
            self.hmm_models[key] = hmm.GaussianHMM(2, "full",
            startprob=np.array([ 0.5,  0.5]))
            self.hmm_models[key]._algorithm = "viterbi"
            self.hmm_models[key]._init([X]) #self.hmm_models[key].init_params

            yield key, (self.hmm_models[key], [X])

    #def fill_hmm_init(self):
     #   logprob = []
      #  keys =

    def fill_hmm(self, key, value):
        model = value[0]
        obs = value[1]
        logprob = []
        for i in range(model.n_iter):
            stats = model._initialize_sufficient_statistics()
            curr_logprob = 0
            for seq in obs:
                framelogprob = model._compute_log_likelihood(seq)
                lpr, fwdlattice = model._do_forward_pass(framelogprob)
                bwdlattice = model._do_backward_pass(framelogprob)
                gamma = fwdlattice + bwdlattice
                posteriors = np.exp(gamma.T - logsumexp(gamma, axis=1)).T
                curr_logprob += lpr
                model._accumulate_sufficient_statistics(
                    stats, seq, framelogprob, posteriors, fwdlattice,
                    bwdlattice, model.params)
            logprob.append(curr_logprob)

            # Check for convergence.
            if i > 0 and abs(logprob[-1] - logprob[-2]) < model.thresh:
                break

            # Maximization step
            model._do_mstep(stats, model.params)
            yield key, model

    def init_create_fhmm(self):
        self.list_pi = []
        self.list_transmat = []
        self.list_means = []

    def create_fhmm(self, key, model):
        self.list_pi.append(model[appliance].startprob_)
        self.list_transmat.append(model[appliance].transmat_)
        self.list_means.append(model[appliance].means_.flatten().tolist())
        print model[appliance].startprob_

    def final_create_fhmm(self):
        
        list_fhmm_pi = self.compute_fhmm_x(self.list_pi)
        list_fhmm_transmat = self.compute_fhmm_x(self.list_transmat)
        [means_fhmm, cov_fhmm] = self.compute_fhmm_means_cov(self.list_means)
        fhmm_model = hmm.GaussianHMM(n_components=len(list_fhmm_pi), covariance_type='full', startprob=list_fhmm_pi, transmat=list_fhmm_transmat)
        fhmm_model.means = means_fhmm
        fhmm_model.covars_ = cov_fhmm

        yield None, fhmm_model

    def steps(self):
        return [
            self.mr(mapper_init = self.init_create_hmm,
                    mapper = self.create_hmm,
                    mapper_final = self.final_create_hmm,
                    #combiner_init = self.fill_hmm_init,
                    combiner = self.fill_hmm,
                    #combiner_final = self.fill_hmm_final,
                    reducer_init = self.init_create_fhmm,
                    reducer = self.create_fhmm,
                    reducer_final = self.final_create_fhmm)
        ]

if __name__ == '__main__':
    MRfhmm.run()


