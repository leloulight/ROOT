#!/usr/bin/env python

# This is a control script, which evaluates an extended option string
# and sends the resulting set of option strings together with the TMVA
# analysis program to a batch system.  Per default lsfbatch is used,
# but in principle this scrip can be modified to work with every batch
# system.
#

# The job (program) to be submitted is 'analysis'.  It must be
# compiled before using this script!  Please modify 'makefile'
# according to your needs (specify the TMVA directory) and type
#
# $ make
#
# to compile analysis.cxx

import os   # system()
import vary # helper functions to parse and expand the option string

### specify the event sample
#
# Sample    - filename of the event sample
# SigTree   - name of signal tree from the event sample
# BkgTree   - name of background tree from the event sample
# Variables - comma separated list of variables to be used
#
Sample    = "/afs/cern.ch/user/path/to/sample/sample.root"
SigTree   = "TreeS"
BkgTree   = "TreeB"
Variables = "var1,var2,var3,var4,var5"

### specify the output files
#
# ROOTOutput - ROOT file, which is generated by TMVA and contains
#              the classifier output histograms etc.
# TextOutput - The name of the text file, where all classifier
#              and factory parameters as well as the classifier output
#              (ROC integral, boosting output).  See 'analysis.cxx'.
#
ROOTOutput = "/afs/cern.ch/user/path/to/results/TMVA.root"
TextOutput = "/afs/cern.ch/user/path/to/results/results.txt"

### specify factory option string
#
# FactoryOptionString - option string of the factory
#
FactoryOptionString = "!V:nTrain_signal=10000:nTrain_background=10000:nTest_signal=10000:nTest_background=10000:SplitMode=Random:NormMode=EqualNumEvents"

### specify method to book
#
# MethodName - Name of the method to book, as defined by the macro
#              REGISTER_METHOD()
#
# MethodOptionString - Option string of the method.  Ranges and lists
#                      can be set as values for the variables.  For
#                      example one can specify intervals by
#
#    x=[2,10,1]
#
# which will generate values for x in the intervall [2,10] with a step
# size of 1 (including the endpoint).
#
# A set of discrete values to use can be specified by
#
#    x={T,F}  or  x={1,10,100,1000}
#
# All in all the option string
#
#    x=[1,3,1]{10}:y={T,F}:z=1
#
# will be expanded to the set of option strings
#
#    x=1:y=T:z=1
#    x=2:y=T:z=1
#    x=3:y=T:z=1
#    x=10:y=T:z=1
#    x=1:y=F:z=1
#    x=2:y=F:z=1
#    x=3:y=F:z=1
#    x=10:y=F:z=1
#
MethodName         = "PDEFoam"
MethodOptionString = "!H:!V:SigBgSeparate=F:TailCut=0.001:VolFrac=0.0333:nActiveCells=500:nSampl=2000:nBin=5:CutNmin=T:Nmin=100:Kernel=None:Compress=T"

### specify queue
#
# Queue - batch queue to send the jobs to
#
Queue = "1nd"

#=================================================================

def main():

    # parse option string and return list of option strings, where all
    # intervalls and discrete values are expanded
    OptStr_list = vary.ExpandOptionString(MethodOptionString)

    job_count = 0
    # loop over all option strings and submit the jobs
    for optstr in OptStr_list:

        # Extend ROOT output file name by the hash value of the method
        # option string.  This avoids name conflicts when two ore more
        # jobs run on the same machine.
        modROOTOutput = ROOTOutput.replace(".root", \
                                           "_"+str(abs(hash(optstr)))+".root")

        # create command string, which submits the job to the queue
        # 'bsub' is the job submission program
        exestr = "bsub" \
                 + " -q " + Queue \
                 + " \'analysis " \
                 + " -b " + BkgTree \
                 + " -s " + SigTree \
                 + " -e " + Sample \
                 + " -v " + Variables \
                 + " -r " + modROOTOutput \
                 + " -t " + TextOutput \
                 + " -f " + FactoryOptionString \
                 + " -m " + MethodName \
                 + " -o " + optstr \
                 + "\'"

        # execute command string
        print exestr
        os.system(exestr)
        job_count += 1

    print "--- finished ---  ", job_count, "jobs submitted"

main()
