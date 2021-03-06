#!/bin/bash

if [ -z "$1" ]
then
  STATIONPAIR=HOT05_HOT25
else
  STATIONPAIR=$1
fi

mkdir -p Final_${STATIONPAIR}_Rayleigh

FMIN=0.025
FMAX=0.35
RHO=0.5e3
VS=0.5e3
XI=0.05
VPVS=0.05

SKIP=15

/usr/bin/time -o Final_${STATIONPAIR}_Rayleigh/opt.time ../Phase/optimizer/optimizerayleigh \
    -i ../example_data/RayleighResponse/dispersion_${STATIONPAIR}.txt \
    -o Final_${STATIONPAIR}_Rayleigh/opt \
    -r Initial_${STATIONPAIR}_Rayleigh/opt.model \
    -f $FMIN \
    -F $FMAX \
    -R $RHO \
    -V $VS \
    -X $XI \
    -S $VPVS \
    -M 0 \
    -G 1.0 \
    -N 30 \
    -e 1.0 \
    -J \
    -T $SKIP


