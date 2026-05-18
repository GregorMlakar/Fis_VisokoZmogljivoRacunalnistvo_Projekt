# Paralelna obdelava videa z uporabo MPI in OpenCV
## Pretvorba video sličic v sivinsko obliko in detekcija robov s Sobelovim operatorjem

### 1.	Analizo zmogljivosti: Meritve časa na 1, 2, 4, 8... jedrih (povprečje treh zagonov).

Program zaženemo 3x za vsako število procesov:
-	mpiexec -n 1 MpiVideoProc.exe mp4_example.mp4
-	mpiexec -n 2 MpiVideoProc.exe mp4_example.mp4
-	mpiexec -n 4 MpiVideoProc.exe mp4_example.mp4
-	mpiexec -n 8 MpiVideoProc.exe mp4_example.mp4

Procesor:

Pomnilnik:

Meritev:

###  2.	Izračun metrik: Določitev pospeška in Karp-Flattove metrike e.


### 3.	Interpretacijo rezultatov: Razlago odmikov od idealnega pospeška in identifikacijo ozkih grl (npr. preveč MPI komunikacije ali neenakomerna porazdelitev dela).
 
