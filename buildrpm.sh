#!/bin/sh

cd ..
tar czf libole2-2.2.8.tar.gz libole2-2.2.8
mv -f libole2-2.2.8.tar.gz /usr/src/redhat/SOURCES
cd libole2-2.2.8
rpmbuild -bb libole2.spec

