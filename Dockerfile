FROM ubuntu:18.04

MAINTAINER hydai hydai@secondstate.io

RUN apt update && apt install -y \
	cmake \
	g++ \
	libboost-all-dev \
	&& rm -rf /var/lib/apt/lists/*
