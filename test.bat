if not exist report mkdir report
gcc -g -c -O0 -DCONFIG_COVERAGE coverage\coverage.c -o report\coverage.o
g++ -g -c -O0 -DCONFIG_COVERAGE -fprofile-arcs -ftest-coverage sample\test.cpp -o report\test.o
g++ -g -O0 -lgcov --coverage report\test.o report\coverage.o -o report\test.exe
.\report\test.exe > report\handler.log
python gen-cov-rp.py . report
