if not exist report mkdir report

SET CC=gcc
SET CPP=g++
SET GCOV_TOOL=gcov

%CC% -g -c -O0 -DCONFIG_COVERAGE coverage\coverage.c -o report\coverage.o
%CPP% -g -c -O0 -DCONFIG_COVERAGE -fprofile-arcs -ftest-coverage sample\test.cpp -o report\test.o
%CPP% -g -O0 -lgcov --coverage report\test.o report\coverage.o -o report\test.exe
.\report\test.exe > report\handler.log
python gen-cov-rp.py %GCOV_TOOL% . report
