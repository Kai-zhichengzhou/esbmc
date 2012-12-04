#!/bin/python
#############################
# Script to display test suite results
##############################

import sys
import os
from sys import argv
import xml.etree.ElementTree as ET


def error(message):
    sys.stderr.write("error: %s\n" % message)
    #sys.exit(1)

error_file = "resultados_error.log"
f = open(error_file, 'w')

suc   = 0
fai   = 0
fsuc  = 0
fneg  = 0
total = 0

def disp_resul():
    """
    Display the verification result
    """
    print "Verification Success: ", suc
    print "Verification Fail: ", fai
    print "False positive:", fsuc
    print "False negative:", fneg 
    print "Total: ", total 

def resultados(ite_ex, ite_ac):
    """
    Check the result. Sucess, Fail, false sucess and false fail
    """
    global suc, fai, fsuc, fneg, total
    if ite_ex.text == ite_ac.text == "[SUCCESSFUL]":
      suc+=1
    elif ite_ex.text == ite_ac.text == "[FAILED]":
      fai+=1
    elif ite_ex.text == "[FAILED]":
      fsuc+=1
    else:
      fneg+=1
    
    total+=1

def show_info(path):
    """
    Parse tree
    """
    global suc, fai, fsuc, fneg, total
    suc = fai = fsuc = fneg = total = 0
    os.chdir(path)
    print "##### Directory: " + path
    try:
        tree = ET.parse("test_log.xml")
    except IOError as e:
        #sys.exit("Could not open test_log.xml")
        error("Could not open test_log.xml")
        return
    #XML file
    root = tree.getroot()
    for res in root.findall('run-test'):
      ite_ex = res.find('item_09_expected-result')
      ite_ac = res.find('item_10_actual-result')
      if ite_ex != None and ite_ac != None:
        resultados(ite_ex, ite_ac)
        
        
      else:
        f.write('Error file: ')
        f.write(res.find('item_01_test-name').text + '\n') 
        error("Parser error at " + res.find('item_01_test-name').text) 
     
    disp_resul() #display result


def main():
    if len(sys.argv) < 2:
      print "usage: %s <PATH>" % argv[0]
      sys.exit(1)
    path = argv[1];
    listing = os.listdir(path)
    listing.sort() #sort files
    os.chdir(path)
    for infile in listing:
      if os.path.isdir(infile):
        show_info(infile)
        print
        os.chdir("..")
          
if __name__ == "__main__":
    main()

