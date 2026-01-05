## ~~1. Wstęp~~
- ~~1.1 Cel i zakres pracy~~
- ~~1.2 Motywacja i zastosowania symulacji tkanin~~
- ~~1.3 Zakres funkcjonalny aplikacji~~
- ~~1.4 Struktura pracy~~

## ~~2. Podstawy teoretyczne~~
- ~~2.1 Modele symulacji tkanin~~
- ~~2.1.1 Model masa–sprężyna~~
- ~~2.1.2 Rodzaje sprężyn (structural, shear, bend)~~
- ~~2.1.3 Właściwości fizyczne tkanin~~

### ~~2.2 Dynamika punktów masowych~~
- ~~2.2.1 Równania ruchu~~
- ~~2.2.2 Metoda Verlet~~

### ~~2.3 Siły działające na tkaninę~~
- ~~2.3.1 Grawitacja~~
- ~~2.3.2 Wiatr~~
- ~~2.3.3 Rozszerzalność systemu o nowe siły~~

### ~~2.4 Kolizje i detekcja kolizji
- ~~2.4.1 AABB – Axis Aligned Bounding Box~~
- ~~2.4.2 Ray casting i interakcja z użytkownikiem~~

### 2.5 Rendering 3D
- ~~2.5.1 Podstawy OpenGL~~  
- ~~2.5.2 Teksturowanie~~
- ~~2.5.3 Oświetlenie (Phong / Blinn-Phong)~~
- ~~2.5.4 Skybox~~

## 3. Projekt systemu
### ~~3.1 Wymagania funkcjonalne i niefunkcjonalne
### ~~3.2 Architektura aplikacji
- ~~3.2.1 Podział na moduły  
- ~~3.2.2 Diagramy UML (klasy, sekwencji)  

### 3.3 Kluczowe komponenty
- ~~3.3.1 `MassPoint`  
- ~~3.3.2 `Spring`  
- ~~3.3.3 `Cloth`  
- ~~3.3.4 `Force` (klasa abstrakcyjna)  
- ~~3.3.5 `Renderer`  
- ~~3.3.6 `Raycaster`  
- ~~3.3.7 `SimulationController`  

## 4. Implementacja
### 4.1 Technologie
- ~~C++, OpenGL, GLFW, ImGui, CMake  

### 4.2 Implementacja symulacji
- ~~4.2.1 Aktualizacja pozycji metodą Verlet  
- ~~4.2.2 Zerwanie sprężyn  
- ~~4.2.3 Kolizje i interakcje użytkownika  
- ~~4.2.4 Rozrywanie tkaniny raycastem  
- ~~4.2.5 Przeciąganie i odrywanie punktów  

### 4.3 Rendering i grafika
- ~~4.3.1 Ładowanie i nakładanie tekstury  
- ~~4.3.2 Oświetlenie  
- ~~4.3.3 Skybox  
- ~~4.3.4 Tryby wizualizacji (ukrywanie mas, sprężyn, tekstury)  

### 4.4 Interfejs użytkownika (ImGui)
- ~~4.4.1 Zmiana parametrów symulacji  
- ~~4.4.2 Włączanie/wyłączanie elementów  

### 4.5 Eksport danych
- ~~4.5.1 Zbieranie danych punktów masowych  
- ~~4.5.2 Generowanie CSV  

## 5. Testy i eksperymenty
- ~~5.1 Scenariusze testowe  
- ~~5.2 Wpływ parametrów sprężyn na zachowanie tkaniny  
- ~~5.3 Wpływ sił zewnętrznych  
- ~~5.4 Stabilność symulacji  
- ~~5.5 Wydajność (FPS vs. liczba punktów)  

## 6. Podsumowanie i wnioski
- ~~6.1 Osiągnięte cele  
- ~~6.2 Ograniczenia projektu  
- ~~6.3 Możliwe kierunki rozwoju  

## Bibliografia


Praca dotyka fizyki, grafiki komputerowej, algorytmów i programowania

# Hasła:

## 1. Modele tkanin i fizyka symulacji
- cloth simulation mass spring model
- mass-spring system physics
- cloth tearing simulation
- Verlet integration
- position based dynamics (PBD)
- Baraff & Witkin – Large Steps in Cloth Simulation* 
- Bridson – Cloth Simulation (SIGGRAPH notes)
- Müller et al. – Position Based Dynamics

## 2. OpenGL i rendering
- OpenGL modern pipeline
- Phong lighting
- Blinn-Phong model
- texture mapping
- skybox cubemap
- OpenGL Programming Guide - red book
- LearnOpenGL - Joey de Vries
- Dokumentacja OpenGL  

## 3. Algorytmy i struktury danych
- AABB collision detection
- ray casting algorithm
- broad phase collision detection*
- Christer Ericson – *Real-Time Collision Detection

## 4. Programowanie i architektura
Szukaj:
- *C++ design patterns*
- *game engine architecture*
- *component-based design*

## Biblioteki użyte w projekcie
- GLFW documentation  
- ImGui documentation  
- CMake documentation  
- stb_image
