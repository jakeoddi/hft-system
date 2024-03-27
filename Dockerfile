FROM ubuntu:22.04

# Avoid prompts from apt during build
ARG DEBIAN_FRONTEND=noninteractive

# Install build essentials
RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    cmake \
    clang \
    gdb \
    libboost-all-dev \
    libssl-dev \
    wget \
    curl \
    git \
    ca-certificates \
    python3 \
    && rm -rf /var/lib/apt/lists/*

# Set environment variables for Boost installation
ENV BOOST_VERSION=1.84.0
ENV BOOST_VERSION_UNDERSCORE=1_84_0
ENV BOOST_ROOT=/usr/local/boost

# Download and extract Boost
RUN cd /tmp && \
    wget -q https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION_UNDERSCORE}.tar.gz && \
    tar xzf boost_${BOOST_VERSION_UNDERSCORE}.tar.gz && \
    mv boost_${BOOST_VERSION_UNDERSCORE} ${BOOST_ROOT} && \
    rm boost_${BOOST_VERSION_UNDERSCORE}.tar.gz

# Build and install Boost
RUN cd ${BOOST_ROOT} && \
    ./bootstrap.sh --with-libraries=system,filesystem,thread,regex,chrono,atomic,date_time,json && \
    ./b2 install

# Install Miniconda
RUN wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O /tmp/miniconda.sh \
    && bash /tmp/miniconda.sh -b -p /opt/conda \
    && rm /tmp/miniconda.sh

# Set PATH to include conda
ENV PATH="/opt/conda/bin:${PATH}"

COPY hft-environment.yml /tmp/hft-environment.yml

RUN conda env create -f /tmp/hft-environment.yml
RUN echo ". /opt/conda/etc/profile.d/conda.sh" >> ~/.bashrc && \
    echo "conda activate hft" >> ~/.bashrc

WORKDIR /base

COPY . /base
