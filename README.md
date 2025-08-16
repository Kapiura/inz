# daj bogu litosci

### 1. wstepu
- problem - jak wyglada realistyczne rozrywanie tkanin w czasie rzeczyswitym
- cel - implementacja symulatora 2D/3D z rozrywaniem
- zakres - model masa-sprezyna, kolizje, rozrywanie, srodowisko

### 2. literatura - przeglad
- modele - masa-sprezyna vs FEM vs position-based dynamics
- typy sprezyn w tkaninach - strukturalna
    - strukturalna
    - shear
    - bend
- Integracja
    - Explicit/Implicit Euler
    - Semi-implicit
    - Verlet - to wybiore
- rozywanie - progowe odksztalcenie, energia sprezysta, akumulacja zmeczeniowa

### 3. model matematyczny
- Równania ruchu cząstki:
- Sprężyna Hooke’a:

### 4. implementacju
- reprezentacja siatki - kwadraciak, structural shear bend - jeszcze nei wiem
- verlet
- kolizje - nie wiem
- co sie dzieje po rozerwaniu - usuniecie krawdzei - sprezyny
- ImGui OpenGL GLFW
- dane wyjsciowe nie wiem, wejsciowe tez nie wiem

### 5. wynik
- wykresy
- zrzuty klatek pozkaujze ewolucje rozerwania
- tabele parametrow

### 6. wniosek, do czego mozna wykorzystac
- lepsze kryteria rozrywania
- realna symtulacja idk

### 7. coski
- gestosc siatki
- masa
- sprezystosc