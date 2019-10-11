#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import subprocess
import sys
import datetime
import os
from itertools import repeat
import multiprocessing
from multiprocessing import Pool, Lock, Value, Manager
multiprocessing.set_start_method('spawn', True)

### compile_test.py
### For each filename in file listed in @p, runs 'make name=@file graph
### and writes a browsable markdown-formatted test report containing stderr,
### stdout and return code output for each test.
### Files are compiled in a concurrent manner

# ------------------------------------------------------------------------------
# ----------------------------- Utility functions ------------------------------
# ------------------------------------------------------------------------------
def run(cmd):
    proc = subprocess.Popen(cmd,
        stdout = subprocess.PIPE,
        stderr = subprocess.PIPE,
    )
    stdout, stderr = proc.communicate()
 
    return proc.returncode, stdout.decode('utf-8'), stderr.decode('utf-8')

TESTFILE = "filelist.lst"
OUTFILE = "compile_test_report.md"

# courtesy of https://stackoverflow.com/a/34325723
def printProgressBar (iteration, total, prefix = '', suffix = '', decimals = 1,
        length = 60, fill = '█'):
    """
    Call in a loop to create terminal progress bar
    @params:
        iteration   - Required  : current iteration (Int)
        total       - Required  : total iterations (Int)
        prefix      - Optional  : prefix string (Str)
        suffix      - Optional  : suffix string (Str)
        decimals    - Optional  : positive number of decimals in percent complete (Int)
        length      - Optional  : character length of bar (Int)
        fill        - Optional  : bar fill character (Str)
    """
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    print('\r%s |%s| %s%% %s' % (prefix, bar, percent, suffix), end='\r', flush=True)
    # Print New Line on Complete
    if iteration == total: 
        print()


def printHeader():
    print("=" * 80)

def init_proc(lock_):
    global lock
    lock = lock_

# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------

def executeDHLSMake(file, nfiles, testResults, iteration):
    rc, stdout, stderr = run(["make", "name="+file, "graph"])
    with lock:
        iteration.value += 1
        testResults[file] = {"RC": rc, "stdout" : stdout, "stderr" : stderr}
        printProgressBar(iteration.value, nfiles, prefix="Progress", suffix=file.ljust(20))

if __name__ == "__main__":
    # Setup shared memory variables
    manager = Manager()
    testResults = manager.dict()
    iteration = manager.Value('i', 0)

    DIR = os.path.dirname(os.path.abspath(__file__))
    timestamp = str(datetime.datetime.now())

    # Ensure that we are in the examples directory
    os.chdir(DIR)
    printHeader()
    print("============================= DHLS Compilation test ============================")
    printHeader()
    with open(TESTFILE) as filelist:
        # Compile each example
        files = filelist.read().splitlines()
        nFiles = len(files)
        print("Compiling " + str(nFiles) + " files listed in:\n\t'"+ DIR + "/" + TESTFILE + "'\n")
        printProgressBar(iteration.value, nFiles, prefix="Progress", suffix="")
        lock = Lock()

        # run executeDHLSMake on each file listed in the input filelist using
        # a threadpool of size n_cpu's
        with Pool(os.cpu_count(), initializer=init_proc, initargs=(lock,)) as pool:
            for file in files:
                pool.apply_async(executeDHLSMake, args=(file, nFiles, testResults, iteration))
            pool.close()
            pool.join()

    print("\nCompilation complete!")

    # Format a report
    report = "# DHLS Compile test report\n"
    report += "## Executed at:" + timestamp + "\n"
    TOC = "## Table of contents\n"
    contents = ""
    summary = "## Summary\n"
    failingTests = []

    ## Format content of report
    for name, res in testResults.items():
        fail = res["RC"] != 0
        reportName = "File: '" + name + "'\tStatus: " + \
            ("Success" if fail == 0 else "Fail") + \
            "\t(return code: " + str(res["RC"]) + ")"

        # Header anchor link must be simplified a bit to adhere to markdown syntax
        reportNameLink =  reportName.replace('\t', '-').replace('\'', '').replace(':', '').replace(' ', '-').lower()
        reportNameAnchor = "[" + reportName + "]" + \
            "(#" + reportNameLink + ")"
        TOC += " - " + reportNameAnchor + "\n"
        contents += "## " + reportNameAnchor.replace('#', '') + "\n"
        # Add collapsible subsections containing stderr and stdout output
        contents += "<details>\n"
        contents +=  "<summary>`stdout` output:</summary>\n\n"
        contents += "```\n" + res["stdout"] + "```\n"
        contents += "</details>\n"
        contents += "<details>\n"
        contents += "<summary>`stderr` output:</summary>\n\n"
        contents += "```\n" +res["stderr"] + "```\n"
        contents += "</details>\n"
        contents += "\n"

        if fail:
            failingTests.append(reportNameAnchor)

    ## Format summary
    if len(failingTests) == 0:
        print("Status: SUCCESS - all tests compiled succesfully\n")
        summary += "All tests compiled successfully\n"
    else:
        print("Status: FAIL - some tests compiled with non-zero exit codes\n")
        summary += "The following tests returned a non-zero exit code:\n"
        for failingTest in failingTests:
            summary += " - " + failingTest + "\n"

    report += summary + "\n" + TOC + "\n" + contents + "\n"

    with open(OUTFILE, 'w+') as reportFile:
        reportFile.write(report)

    print("Report written to file: \n\t" + DIR + "/" + OUTFILE)
    printHeader()