# Symulacja fizyczna rozrywania tkanin w czasie rzeczywistym z wykorzystaniem modelu masa-sprężyna


## TO DO
- [x] - Ukrywanie/ujawnianie mas
- [x] - Ukrywanie/ujawnianie spreżyn
- [x] - Siła wiatru
- [x] - Podstawowe GUI do edycji parametrów początkowych
- [x] - Tekstura/obraz tkaniny
- [x] - Skybox
- [x] - Oswietlenie
- [x] - Cienie
- [x] - Analiza parametrow wybranych punktów
- [x] - Generowanie danych pomiarowych
- [x] - Zmiana rozmiaru tkaniny
- [x] - Mozliwosc przemieszcania zrodlem swiatla
- [x] - Może poprawić ciecie tkainy bo dalej pod pewnymi katami/pewnej dlugosci nie przecina badz robi to dosc upo
- [ ] - Możliwość dodawania wlasnych plikow jako tekstura
## Fixes to do
- [x] - Trzeba zrobic jakis inny pomysl rozrywania tej tkaniny bo za bardzo laguje
- [x] - Trzeba zrobic fix przy rozdzielaniu pojedynczej masy

## Installation
Works for fedora ;p

g++
sudo dnf install g++ 

opengl - GLEW, SDL2 (+SDL2_Image), GLM and FreeType
sudo dnf install glew-devel SDL2-devel SDL2_image-devel glm-devel freetype-devel

GLFW
sudo dnf install glfw-devel

download glad header file
https://glad.dav1d.de/
unzip glad.zip
sudo mv /path/glad/include/glad /usr/include
mv src file
mv /path/glad/src/glad.c /project_path/src

stb_image.h
https://github.com/nothings/stb
sudo mv /path/stb_image.h /usr/include/ 


## Building
mkdir build
cd build
cmake ..
make
./my_opengl_in_test

## Inzynierku
1. Wstep
    - cel projektu
    - problem do rozwiazania
    - mozliwosci jego realizacji
    - zakres prac
    - krotkie omowienie
    - koncowe wyboru
2. Przeglad zagadnien teoretycznych
    - opis pojec
    - model masa sprezyna
    - modele sieci FEM
    - inne metody
    - opis wybranych publikacji i jakis innych tresci
    - charakterystyka wykorzystywanychg w fizyce komputerowej tkanin
    - zasady modelowania, deformnacji, laczen, rozrywania
3. Analiza problemu
    - wymagania funkcjonalne
    - wymagania niefunkcjonalne
    - parametry fizyczne ktore ospialem w projekcie
    - zalozenia srodowsika - opengl itp
    - ograniczenia modelu
    - wydajnosc symulacji
    - artefakty/to drugie ale slowa zapo,nialem - jednak pamietam anomalie
4. Projekt systemu
    - architetkrua projektyu 
    - struktura
    - diagramy klas
    - opisy algorytmow uzytych do danych rzeczy
        - rozrywanie, sily, grawitacja, wiatr, integracja ze sama soba, polaczenie itp
5. Implementacja
    - opis nawajzniszych fragmentowq koda - bo bez koda to nie robota
    - zastosowane biblioteki
    - nartzedzia
    - struktuyrty dabnycgh czemyu takie nie inne pozdrawiam
    - omowienie wczensiej wymienionych algoryutmow wraz z implemetnacja i czemu taka - sposob ich realizacji
    - uzasadnbienie moich upo dezycji - niew iem czy to zrobie bo jestem leniwy
    - no i fragmenty kodu zeby zobaczyli te dzielo sztuki podczas obrony i recenzji
6. Testy i walidacjia
    - no tutaj nie wiem czy sie popisze ale zobaczyemy
    - porownaie symulacji mojej z literatura
    - mozna rozne ilosci, rozdzielosci tkaniny i fps essa
    - ocena stablinosci tego goracego shitu - pewnie tragiczna bo ja to pisalem a na dodatek masa sprezyna to jest
    - przyklady jakiues moze sski z symulacju
7. Podusmowanko
    - omowienie zrealizowanego zakresa
    - osiagniecia preojekta
    - mzliwe dalsze keirunki rozwjou
8. Bibnliografiu i zalaczniki
