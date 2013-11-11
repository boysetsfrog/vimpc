#!/bin/bash

# Debian compilers
GCC46=/usr/bin/g++-4.6
GCC46_LIB=/usr/lib/gcc/i486-linux-gnu/4.6/
GCC47=/usr/bin/g++-4.7
GCC47_LIB=/usr/lib/gcc/i486-linux-gnu/4.7/

# Colours
RED=$(tput setaf 1)
GREEN=$(tput setaf 2)
NORMAL=$(tput sgr0)

col=$(($(tput cols) - 8))

compile_test ()
{
    FLAGS=$@

    echo "Running configure $FLAGS... "

    RESULT=`eval ./configure $FLAGS 2>&1`

    if [ $? -eq 0 ]
    then
        printf '%*s%s%s' $col "[ $GREEN" "OK" "$NORMAL ]"
        echo ""
        echo "Running make... "
        make clean >/dev/null 2>&1
        RESULT=`make -j4 2>&1`

        if [ $? -eq 0 ]
        then
            printf '%*s%s%s' $col "[ $GREEN" "OK" "$NORMAL ]"
            echo ""
        else
            printf '%*s%s%s' $col "[ $RED" "FAIL" "$NORMAL ]"
            echo ""
            echo "DUMPING OUTPUT"
            echo "===================="
            echo "$RESULT"
            echo "===================="
        fi
    else
        printf '%*s%s%s' $col "[ $RED" "FAIL" "$NORMAL ]"
        echo ""
        echo "DUMPING OUTPUT"
        echo "===================="
        echo "$RESULT"
        echo "===================="
    fi
}

compile_sets ()
{
    compile_test
    compile_test --enable-debug=yes --enable-test=yes
    compile_test --enable-boost=yes
    compile_test --enable-taglib=no
}

echo "Default compiler..."
echo "------------------------------"
compile_sets

echo "Default g++ as compiler..."
echo "------------------------------"
export CXX=g++ 
compile_sets

echo "g++ 4.6 as compiler..."
echo "------------------------------"
export CXX=$GCC46 LDFLAGS="-L$GCC46_LIB" 
compile_sets

echo "g++ 4.7 as compiler..."
echo "------------------------------"
export CXX=$GCC47 LDFLAGS="-L$GCC47_LIB" 
compile_sets

echo "clang++ as compiler..."
echo "------------------------------"
export CC=clang CXX=clang++ 
compile_sets

echo "clang++ with libc++ as compiler..."
echo "------------------------------"
export CC=clang CXX=clang++ CXXFLAGS="-stdlib=libc++" LDFLAGS="-lc++abi"
compile_sets
