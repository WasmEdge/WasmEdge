FROM ubuntu:18.04

MAINTAINER hydai hydai@secondstate.io

RUN apt update && apt install -y \
	cmake \
	curl \
	g++ \
	libboost-all-dev \
	&& curl -sL https://deb.nodesource.com/setup_12.x | bash \
	&& apt install -y nodejs \
	&& rm -rf /var/lib/apt/lists/*
