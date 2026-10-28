# Desktop build of ubDMR for the README screenshots (Qt 5.12 as on the phone).
FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -qq && apt-get install -y -qq --no-install-recommends \
      qtbase5-dev qtdeclarative5-dev qtmultimedia5-dev qttools5-dev qttools5-dev-tools \
      qml-module-qtquick2 qml-module-qtquick-controls2 qml-module-qtquick-layouts \
      qml-module-qtquick-window2 qml-module-qt-labs-settings \
      libqt5multimedia5-plugins pulseaudio pulseaudio-utils cmake g++ make xvfb xdotool imagemagick fonts-dejavu-core python3 \
    && rm -rf /var/lib/apt/lists/*
