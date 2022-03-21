if not exist report mkdir report
gcc -g -c -O0 coverage\coverage.c -o report\coverage.o
g++ -g -c -O0 -fprofile-arcs -ftest-coverage sample\test.cpp -o report\test.o
g++ -g -O0 -lgcov --coverage report\test.o report\coverage.o -o report\test.exe
.\report\test.exe > report\handler.log
python scripts\coverage.py . report
