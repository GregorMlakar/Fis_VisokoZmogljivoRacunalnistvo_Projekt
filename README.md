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

1. Video datoteka se mora nahajati na namizju/desktopu
2. Namestitev: msmpisdk.msi
3. Namestitev: msmpisetup.exe
5. Extrakcija: opencv-4.7.0-windows.exe na C:
6. Spremenljivke okolja: "C:\opencv\build\x64\vc16\lib" in "C:\opencv\build\x64\vc16\bin"
7. Namestitev: DeployCpp.msi

## Analiza
[Podrobna analiza](./Analiza.md)
