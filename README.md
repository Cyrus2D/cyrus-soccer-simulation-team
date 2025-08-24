# CYRUS Soccer Simulation 2D Team

Open-source code for the CYRUS 2D Soccer Simulation team. CYRUS is a long-running, award-winning team in the RoboCup Soccer 2D Simulation League, including the 2021 World Championship. This codebase builds on the CYRUS base and the HELIOS base, using librcsc and Boost.

This repository contains the team (player) and coach binaries, tactics, roles, and utilities for the CYRUS 2D team. It is designed to work with the standard RoboCup Soccer Server (rcssserver) and visualizers (e.g., rcssmonitor).

## Awards

### RoboCup

- 2024 (Netherlands): Second Place [video](https://youtu.be/UhmY-Gkh2Z0?si=umGSDNmqnVxj0HpP)
- 2023 (France): Second Place [video](https://youtu.be/oj_SdlmGt6Q?si=zxMgXrOq_FvKFjF1)
- 2022 (Thailand): Second Place [video](https://youtu.be/DBbc-_7ptrs?si=dx_VBVXcD69CCVza)
- 2021 (Online): Champion [video](https://youtu.be/YUmnXWTu9u0?si=gACUal0kJwuNYLod)
- 2019 (Australia): Third Place
- 2018 (Canada): Second Place
- 2017 (Japan): Fourth Place
- 2014 (Brazil): Fifth Place

### IranOpen

- 2025 (Iran): Champion
- 2024 (Iran): Second Place
- 2023 (Iran): Champion
- 2022 (Iran): Second Place
- 2021 (Iran): Champion
- 2018 (Iran - AsiaPacific): Champion
- 2018 (Iran): Champion
- 2014 (Iran): Champion

## References

- Zare, N., Amini, O., Sayareh, A., Sarvmaili, M., Firouzkouhi, A., Matwin, S., Soares, A.: Improving Dribbling, Passing, and Marking Actions in Soccer Simulation 2D Games Using Machine Learning. In: RoboCup 2021: Robot World Cup XXIV, Springer (2021)
- Zare, N., Sayareh, A., Sarvmaili, M., Amini, O., Matwin, S., Soares, A.: Engineering Features to Improve Pass Prediction in 2D Soccer Simulation Games. In: RoboCup 2021: Robot World Cup XXIV, Springer (2021)
- Hidehisa Akiyama, Tomoharu Nakashima, HELIOS Base: An Open Source Package for the RoboCup Soccer 2D Simulation, In Sven Behnke, Manuela Veloso, Arnoud Visser, and Rong Xiong editors, RoboCup2013: Robot World XVII, Lecture Notes in Artificial Intelligence, Springer Verlag, Berlin, 2014. http://dx.doi.org/10.1007/978-3-662-44468-9_46

### Related works:

- Zare, N., Sayareh, A., Khanjari, S., Firouzkouhi, A., 2024. Observation Denoising in CYRUS Soccer
- Simulation 2D Team For RoboCup 2024. In: RoboCup 2024 Symposium and Competitions, Netherlands (2024). arXiv preprint arXiv:2406.05623.
- Zare, N., Sayareh, A., Sadraii, A., Firouzkouhi, A., Soares, A. :
- Cross Language Soccer Framework: An Open Source Framework for the RoboCup 2D Soccer Simulation. In: RoboCup 2024 Symposium and Competitions, Netherlands (2024). arXiv preprint arXiv:2406.05621.
- Sayareh, A., Zare, N., Amini, O., Firouzkouhi, A., Sarvmaili, M. and Matwin, S., 2023. Observation Denoising in CYRUS Soccer Simulation 2D Team For RoboCup 2023. In: RoboCup 2023 Symposium and Competitions, France (2023). arXiv preprint arXiv:2305.19283.
- Zare, N., Amini, O., Sayareh, A., Sarvmaili, M., Firouzkouhi, A., Ramezani Rad, S., Matwin, S., Soares, A.: Cyrus2D base: Source Code Base for RoboCup 2D Soccer Simulation League. In: RoboCup 2022: Robot World Cup XXV, Springer (2022)
- Zare, N., Firouzkouhi, A., Amini, O., Sarvmaili, M., Sayareh, A., Soares, A., Matwin, S.: CYRUS 2D Soccer Simulation Team Description Paper 2022. In: RoboCup 2022 Symposium and Competitions, Thailand (2022)
- Zare, N., Sayareh, A., Sarvmaili, M., Amini, O., Soares, A., Matwin, S.: CYRUS 2D Soccer Simulation Team Description Paper 2021. In: RoboCup 2021 Symposium and Competitions, Worldwide (2021)
- Zare, N., Sarvmaili, M., Mehrabian, O., Nikanjam, A., Khasteh, S.-H., Sayareh, A., Amini, O., Barahimi, B., Majidi, A., Mostajeran, A.: Cyrus 2D Simulation 2019. In: RoboCup (2019).
- Zare, N., Sadeghipour, M., Keshavarzi, A., Sarvmaili, M., Nikanjam, A., Aghayari, R., Firouzkoohi, A., Abolnejad, M., Elahimanesh, S., Akhgari, A.: Cyrus 2D Simulation Team Description Paper 2018. In: RoboCup(2018), Montreal, Canada (2018).
- Zare, N., Najimi, A., Sarvmaili, M., Akbarpour, A., NaghipourFar, M., Barahimi, B., Nikanjam, A.: Cyrus 2D Simulation Team Description Paper 2017, In: Robocup(2017), Hefei, China (2017).
- Zare, N., Keshavarzi, A., Beheshtian, S. E., Mowla, H., Akbarpour, A., Jafari, H., Arab Baraghi, K., Zarifi, M. A., Javidan, R.: Cyrus 2D Simulation Team Description Paper 2016. In: RoboCup 2016. Leipzig, Germany, (2016).
- Zare, N., Karimi, M., Keshavarzi, A., Asali, E., Ali Poor, H., Aminian, A., Beheshtian, E., Mola, H., Jafari, H. , Khademian, M. J.: Cyrus Soccer 2D Simulation Team Description Paper 2015. In: RoboCup 2015. Hefei, China, (2015).
- Khayami, R., Zare, N., Karimi, M., Mahor, P., Afshar, A., Najafi, M. S., Asadi, M., Tekrar, F., Asali, E., Keshavarzi, A.: CYRUS 2D simulation team description paper 2014. In: RoboCup 2014. Joao Pessoa, Brazil, (2014).
- Khayami, R., Zare, N., Zolanvar, H. M. Karimi, P., Mahor, F., Tekara, E., Asali, Fatehi, M. : Cyrus Soccer 2D Simulation Team Description Paper 2013. In The 17th annual RoboCup International Symposium, Eindhovenm, The Netherlands. (2013)
- Zare, N., Sayareh, A., Amini, O., Sarvmaili, M., Firouzkouhi, A., Matwin, S., Soares, A.: Pyrus Base: An Open Source Python Framework for the RoboCup 2D Soccer Simulation. arXiv preprint arXiv:2307.16875.

## Quick Start

The latest helios-base depends on the following libraries:
 - Boost 1.38 or later https://www.boost.org/
 - The latest librcsc https://github.com/Cyrus2D/cyrus-soccer-simulation-lib

In the case of Ubuntu 16.04 or later, execute the following commands for installing a basic development environment:
```
sudo apt update
sudo apt install build-essential libboost-all-dev cmake
```
install librcsc
```
git clone https://github.com/Cyrus2D/cyrus-soccer-simulation-lib
cd cyrus-soccer-simulation-lib
mkdir build
cd build
cmake ..
make -j
make install
```

install CppDNN:
```
sudo apt install libeigen3-dev
git clone https://github.com/Cyrus2D/CppDNN.git
cd CppDNN
mkdir build
cd build
cmake ..
make
sudo make install
```

To build binaries, execute commands from the root of source directory:
```
mkdir build
cd build
cmake ..
make
```

To start the agent2d team, invoke the start script in `src` directory.
```
cd build/src
./start.sh
```

