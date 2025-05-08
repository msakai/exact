FROM quay.io/pypa/manylinux_2_34_x86_64

RUN dnf install wget -y
RUN wget -O boost_1_88_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.88.0/boost_1_88_0.tar.gz/download
RUN mkdir boost-ver
RUN cd boost-ver && tar -xzf ../boost_*.tar.gz
WORKDIR boost-ver/boost_1_88_0
RUN ./bootstrap.sh
RUN ./b2 install --prefix=/usr/local
RUN /opt/python/cp310-cp310/bin/python -m pip install twine
RUN /opt/python/cp311-cp311/bin/python -m pip install twine
RUN /opt/python/cp312-cp312/bin/python -m pip install twine
RUN /opt/python/cp313-cp313/bin/python -m pip install twine
WORKDIR /
