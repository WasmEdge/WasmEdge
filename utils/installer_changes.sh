#!/bin/bash

bash ./utils/install.sh.old
cp ~/.wasmedge/env ~/env.old
python3 ./utils/install.py
cp ~/.wasmedge/env ~/env
diff --color -u \
    <(sed '1,/Please/d' ~/env.old | sed -e 's/\/\//\//g' |
        sed -e 's/include\/wasmedge\//include\//g' | sed 's/\/$//' |
        sed -e 's/lib\/wasmedge$/lib/g' | sort) \
    <(sed '1,/Please/d' ~/env | sed '\/bin$/d' | sort)

error=$?
if [ $error -eq 0 ]; then
    echo "All Safe"
elif [ $error -eq 1 ]; then
    echo "Raw Old:"
    cat ~/env.old
    echo "Raw New:"
    cat ~/env
    exit 1
else
    echo "There was something wrong with the diff command"
    exit 1
fi
