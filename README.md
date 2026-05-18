# Paralelna obdelava videa z MPI in OpenCV

## Opis projekta

Projekt implementira paralelno obdelavo videa z uporabo MPI in knjižnice OpenCV v programskem jeziku C++. 
Program vhodni video razdeli med več MPI procesov, vsak proces obdela svoj del video sličic, nato pa se rezultati zberejo in zapišejo v izhodni video.

Obdelava posamezne sličice vključuje:
- glajenje z GaussianBlur,
- pretvorbo v sivinsko sliko,
- Sobelovo detekcijo robov.

## Uporabljene tehnologije

- C++
- MPI
- OpenCV

## Paralelizacija

Video je sestavljen iz zaporedja sličic. Program sličice razdeli med MPI procese. Vsak proces obdela svoj del sličic neodvisno od ostalih procesov.

Primer delitve pri 4 procesih:
- rank 0 obdela prvi del sličic,
- rank 1 obdela drugi del sličic,
- rank 2 obdela tretji del sličic,
- rank 3 obdela četrti del sličic.

Za razdelitev podatkov je uporabljena funkcija "MPI_Scatterv", za zbiranje rezultatov pa "MPI_Gatherv".

## Zagon programa

mpiexec -n 4 MpiVideoProc.exe mp4_example.mp4

## Predpogoj za zagon
### Namestitveni programi se nahajajo v mapi: [Dependencies](./Dependencies)
1. Video datoteka se mora nahajati na namizju/desktopu
2. Namestitev: msmpisdk.msi
3. Namestitev: msmpisetup.exe
4. Prenos OpenCV klnjižnice verzija 4.7.0 [OpenCV 4.7.0](https://opencv.org/releases/)
6. Extrakcija: opencv-4.7.0-windows.exe na C:
7. Spremenljivke okolja:
- C:\opencv\build\x64\vc16\bin
- C:\opencv\build\x64\vc16\lib
8. Namestitev aplikacije DeployCpp.msi [Dependencies](./Dependencies/App) (predhodna extrakcija datotek)
### Podrobna navodila so v mapi: [Poročilo](./Poročilo) Navodila za testiranje

## Izvorna koda
[Cpp](./IzvornaKoda)
Navodila za konfiguracijo OpenCV v Visual Studio: [OpenCV](https://www.youtube.com/watch?v=YUjamcyuKT4)
</br>
Navodila za konfiguracijo MS-MPI v Visual Studio: [MS-MPI](https://www.youtube.com/watch?v=L-xJreZ55aU)
</br>
Visual studio 2022 Community: [VS2022](https://visualstudio.microsoft.com/vs/older-downloads/)

## Analiza
[Podrobna analiza](./Analiza.md)
### Word: [Poročilo](./Poročilo) Analiza
