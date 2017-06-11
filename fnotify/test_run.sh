#!/bin/sh

if [ -z "$1" ]; then
	echo "usage: $0 DIR" >&2
	exit 1
fi

cd "$1"
i=0


t=test$i
echo "$t"
date > $t-foo.txt
sleep 1
rm $t-foo.txt
sleep 2
i=$((i+1))


t=test$i
echo "$t"
date > $t-bar.txt
sleep 1
mv $t-bar.txt $t-qux.txt
sleep 1
rm $t-qux.txt
sleep 2
i=$((i+1))


t=test$i
echo "$t"
mkdir $t-abc
sleep 1
rmdir $t-abc
sleep 2
i=$((i+1))


t=test$i
echo "$t"
mkdir $t-abc
sleep 1
mv $t-abc $t-def
sleep 1
rmdir $t-def
sleep 2
i=$((i+1))


t=test$i
echo "$t"
mkdir $t-abc
date > $t-abc/qux.txt
sleep 1
rm $t-abc/qux.txt
rmdir $t-abc
sleep 2
i=$((i+1))


t=test$i
echo "$t"
mkdir $t-abc
date > $t-abc/qux.txt
sleep 1
mv $t-abc/qux.txt $t-abc/foo.txt
sleep 1
rm $t-abc/foo.txt
rmdir $t-abc
sleep 2
i=$((i+1))


t=test$i
echo "$t"
mkdir $t-abc
date > $t-abc/qux.txt
sleep 1
mv $t-abc/qux.txt $t-foo.txt
sleep 1
rm $t-foo.txt
rmdir $t-abc
sleep 2
i=$((i+1))


t=test$i
echo "$t"
mkdir $t-abc
date > $t-abc/qux.txt
sleep 1
mv $t-abc $t-def
sleep 1
rm $t-def/qux.txt
rmdir $t-def
sleep 2
i=$((i+1))
