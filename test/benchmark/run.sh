#!/bin/bash


# Black        0;30     Dark Gray     1;30
# Red          0;31     Light Red     1;31
# Green        0;32     Light Green   1;32
# Brown/Orange 0;33     Yellow        1;33
# Blue         0;34     Light Blue    1;34
# Purple       0;35     Light Purple  1;35
# Cyan         0;36     Light Cyan    1;36
# Light Gray   0;37     White         1;37

echo "Setting Up"
clang -O3 -o fibc fib.c
g++ -o fibcpp -std=c++11 fib.cpp
rustc fib.rs
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

echo -e "${YELLOW}"
echo "----------------- C -------------------"
time ./fibc

echo -e "${LGRAY}"
echo "----------------- C++ -----------------"
time ./fibcpp

echo -e "${CYAN}"
echo "----------------- Go ------------------"
time go run fib.go

echo -e "${PURPLE}"
echo "-------------- Haskell  --------------"
time runghc fib.hs

echo -e "${BLUE}"
echo "----------------- mt ------------------"
time mt fib.mt 

echo -e "${GREEN}"
echo "-------------- Python 3  --------------"
time python3 fib.py 

echo -e "${RED}"
echo "-------------- Python 2  --------------"
time python2 fib.py 

echo -e "${ORANGE}"
echo "-------------- Rust  ------------------"
time ./fib

echo -e "${NC}"
echo -e "Cleaning Up"
rm fibc
rm fibcpp
rm fib
