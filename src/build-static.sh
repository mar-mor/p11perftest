#!/usr/bin/env bash
set -x
g++ -std=gnu++11 -g -O2  -L/usr/local/lib  -o p11perftest-static p11benchmark.o p11rsasig.o p11des3ecb.o p11des3cbc.o p11aesecb.o p11aescbc.o p11perftest.o -fstack-protector -m64 -pthread -L/usr/local/lib -Wl,-Bstatic -lbotan-2 -lboost_timer -lboost_program_options -lboost_chrono -lboost_system -Wl,-Bdynamic -lcrypto -ldl