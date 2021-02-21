#!/bin/sh

RED='\033[0;31m'
LGRAY='\033[0;37m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
ORANGE='\033[0;33m'
LGREEN='\033[1;32m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# print tests a bit better
function testPass {
  echo "${GREEN}"
  echo "-----------------------------------------"
  echo "\t\t$1"
  echo "-----------------------------------------"
  echo "PASSED"
  echo "Ok, passed $2 test(s)"
  echo "-----------------------------------------"
  echo "${NC}"
}

# falied test
function testFail {
  echo "${RED}"
  echo "-----------------------------------------"
  echo "\t\t$1"
  echo "-----------------------------------------"
  echo "FAILED"
  echo "Error, could not pass all tests"
  echo "-----------------------------------------"
  echo "${NC}"
}

# actually run tests

# defer
if [[ $(mt defer/defer.mt) ]]; then
  testFail "defer"
else 
  testPass "defer" 1
fi

# return
if [[ $(mt return/return.mt) ]]; then
 testFail "return"
else
 testPass "return" 1
fi 

# switch
if [[ $(mt switch/switch.mt) ]]; then
 testFail "switch"
else
 testPass "switch" 1
fi 


# use 
if [[ $(mt use/use.mt) ]]; then
 testFail "use"
else
 testPass "use" 3
fi 

