#!/bin/bash

if [ -z "$SPINC" ]
then
    SPINC="./bin/openspin"
fi

if [ "$1" == "clean" ]
then
    find test/ -name \*.binary -exec rm {} \;
    exit
fi

rm -f spin.log

while read line
do
    echo "${SPINC} -L . $line"
    ${SPINC} -L . "$line" >/dev/null
    if [ $? != 0 ]
    then 
        echo "${SPINC} -L . $line" >> spin.log
        ${SPINC} -L . "$line"
    fi
done < <(find . -name \*.spin)


if [ -f spin.log ] ; then
    ERRORS=`cat spin.log | wc -l`
    echo
    echo "$ERRORS errors found."
    exit 1
else
    echo
    echo "0 errors found."
fi

exit 0
