#!/bin/bash

mkdir ../Exec
cd ../../
git clone https://github.com/sisoputnfrba/so-nivel-gui-library.git
git clone https://github.com/sisoputnfrba/so-commons-library.git
cd tp-20131c-ahrelocoooo/Makes/Commons-library/
make clean all
cp 	libcommons.so ../../Exec
cd ../Nivel-Gui
make clean all
cp libnivel-gui.so ../../Exec
cd ../Memoria
make clean all
cp libmemoria.so ../../Exec
cd ../Nivel
make clean all
cp Nivel ../../Exec
cd ../Plataforma
make clean all
cp Plataforma ../../Exec
cd ../Personaje
make clean all
cp Personaje ../../Exec
export LD_LIBRARY_PATH=/home/utnso/tp-20131c-ahrelocoooo/Exec
cd ../../src/Memoria
cp koopa  ../../Exec
cd ../../Exec
ln -s libcommons.so libso-commons-library.so
chmod 777 koopa
