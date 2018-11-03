#!test

mkdir ./dir1
mkdir ./dir2

cd dir2
touch f2.txt
cd ..

sleep 125

cd dir1
touch f1.txt
cd ..

./daemon start

sleep 60

./daemon stop
