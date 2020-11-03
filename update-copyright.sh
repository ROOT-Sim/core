#!/bin/bash

YEAR=`date +%Y`
STRING="Copyright (C) 2008-$YEAR HPDCS Group"

find . -type f | grep -v $0 | grep -v .svn | xargs perl -pi -e 's/Copyright\ \(C\)\ 2008-[0-9]{4}\ HPDCS Group/@ROOTSIM-COPY-STRING@/g'
find . -type f | grep -v $0 | grep -v .svn | xargs perl -pi -e "s/@ROOTSIM-COPY-STRING@/$STRING/g"
