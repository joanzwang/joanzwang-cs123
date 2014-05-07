from sklearn import hmm
import numpy as np
import itertools
from collections import OrderedDict
import csv
from mrjob.job import MRJob

#r'C:\Users\Acer\Documents\Terrance\UChicago\3rd Year\SPRING\CS123\Project\home2_data_transpose.csv'
class MRfhmm(MRJob):

    #list_x can be list Pi's or list of transmat's
    def compute_fhmm_x(list_x):
        rv = list_x[0]
        for i in range(1, len(list_x)):
            rv = np.kron(rv, list_x[i])
        return rv

    def compute_fhmm_means_cov(list_means):
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
            X = np.column_stack([self.datasheet[0][1:], self.datasheet[i][1:]])
            key = self.datasheet[i][0]
            self.hmm_models[key] = hmm.GaussianHMM(2, "full")
            self.hmm_models[key].fit([X], n_iter = 1000)

            yield key, self.hmm_models[key]

    def init_create_fhmm(self):
        list_pi = []
        list_transmat = []
        list_means = []

    def create_fhmm(self, key, model):
        list_pi.append(model[appliance].startprob_)
        list_transmat.append(model[appliance].transmat_)
        list_means.append(model[appliance].means_.flatten().tolist())

    def final_create_fhmm(self):
        list_fhmm_pi = compute_fhmm_x(list_pi)
        list_fhmm_transmat = compute_fhmm_x(list_transmat)
        [means_fhmm, cov_fhmm] = compute_fhmm_means_cov(list_means)
        fhmm_model = hmm.GaussianHMM(n_components=len(list_fhmm_pi), covariance_type='full', startprob=list_fhmm_pi, transmat=list_fhmm_transmat)
        fhmm_model.means = means_fhmm
        fhmm_model.covars_ = cov_fhmm

        yield None, fhmm_model

    def steps(self):
        return [
            self.mr(mapper_init = self.init_create_hmm,
                    mapper = self.create_hmm,
                    mapper_final = self.final_create_hmm,
                    reducer_init = self.init_create_fhmm,
                    reducer = self.create_fhmm,
                    reducer_final = self.final_create_fhmm)
        ]

if __name__ == '__main__':
    MRfhmm.run()


