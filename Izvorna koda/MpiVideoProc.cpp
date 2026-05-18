// MpiVideoProc.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <shlobj.h>

#include <mpi.h>

#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

//using namespace std;
//using namespace cv;

void razdeliFrame(int stFrameov, int size, int rank, int& zacetek, int& konec);
cv::Mat SobelEdgeDetection(const cv::Mat frame);
cv::String GetDesktopPath();
bool fileExists(const cv::String name);

/*
mpiexec -n 1 MpiVideoProc.exe mp4_example.mp4
mpiexec -n 2 MpiVideoProc.exe mp4_example.mp4
mpiexec -n 4 MpiVideoProc.exe mp4_example.mp4
mpiexec -n 8 MpiVideoProc.exe mp4_example.mp4
*/
int main(int argc, char** argv)
{
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // številka trenutnega procesa
    MPI_Comm_size(MPI_COMM_WORLD, &size); // skupno število procesov

    if (argc > 2)
    {
        if (rank == 0)
            std::cout << "Uporaba: " << "mpiexec -n <procesi> <./MpiVideoProc.exe> <vhodni_video_name.mp4>" << std::endl;

        MPI_Finalize();
        return 1;
    }

    const cv::String arg0 = argc >= 1 ? argv[0] : "NULL";
    const cv::String arg1 = argc >= 2 ? argv[1] : "mp4_example.mp4";
    const cv::String desktop = GetDesktopPath();

    // pot do namizja
    if (desktop == "")
    {
        std::cout << "\n\n==============================================================\n";
        std::cout << "Napaka: pot do namizja ni definirana. \n";
        std::cout << "==============================================================\n\n";
        MPI_Finalize();
        return -1;
    }

    // input datoteka
    const cv::String filename_in = desktop + "\\" + arg1;
    if (!fileExists(filename_in))
    {
        std::cout << "\n\n==============================================================\n";
        std::cout << "Napaka: vhodna datoteka ne obstaja: " << filename_in << " \n";
        std::cout << "==============================================================\n\n";
        MPI_Finalize();
        return -1;
    }

    int sirina = 0;
    int visina = 0;
    int stFrameov = 0;
    double fps = 0.0;
    std::vector<unsigned char> vsiPodatki;

    /*
    Rank 0 prebere video
    Prebere: širino videa, višino videa, FPS, vse frame-e.
    Video frame-i se pretvorijo v byte podatke kjer vse frame shrani v en velik buffer "vsiPodatki"
    To je potrebno zato, ker MPI najlažje pošilja navadne bajte.
    */
    if (rank == 0)
    {
        cv::VideoCapture cap(filename_in);
        if (!cap.isOpened())
        {
            std::cout << "\n\n==============================================================\n";
            std::cout << "Napaka: videa ni mogoce odpreti." << std::endl;
            std::cout << "==============================================================\n\n";
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        fps = cap.get(cv::CAP_PROP_FPS);
        sirina = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        visina = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        int velikostVhodnegaFrame = sirina * visina * 3; // BGR - 3 kanali

        cv::Mat frame;
        std::vector<cv::Mat> framei;
        while (cap.read(frame))
        {
            if (!frame.empty())
            {
                if (!frame.isContinuous())
                    frame = frame.clone();

                framei.push_back(frame.clone());
            }
        }

        cap.release();
        stFrameov = static_cast<int>(framei.size());
        vsiPodatki.resize(stFrameov * velikostVhodnegaFrame);

        // kopiraj podatke video frame-a v velik skupni buffer "vsiPodatki"
        // memcpy se uporablja zato, ker MPI najlažje pošilja zaporedne bajte v pomnilniku
        for (int i = 0; i < stFrameov; i++)
        {
            memcpy(
                vsiPodatki.data() + i * velikostVhodnegaFrame, // to je naslov oz. mesto, kjer se začne i-ti frame v velikem bufferju
                framei[i].data, // podatek i-tega frame-a
                velikostVhodnegaFrame // število bajtov, ki jih kopiramo za en frame
            );
        }

        std::cout << "\n\n==============================================================\n";
        std::cout << "Stevilo prebranih frame-ov: " << stFrameov << std::endl;
        std::cout << "Obdelava podatkov..." << std::endl;
        std::cout << "==============================================================\n\n";
    }

    /*
    Podatki o videu se pošljejo vsem procesom
    rank 0 pove vsem ostalim: širina videa, višina videa, število frame-ov, fps
    To morajo vedeti vsi procesi, da znajo pravilno obdelati svoje podatke.
    */
    MPI_Bcast(&sirina, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&visina, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&stFrameov, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // broadcastamo nekaj dodatnih informacij, ni obvezno potrebno, bolj za to, da vsi procesi imajo enake meta podatke
    MPI_Bcast(&fps, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD); 

    int velikostVhodnegaFrame = sirina * visina * 3; // BGR
    int velikostIzhodnegaFrame = sirina * visina;    // grayscale

    /*
    Razdelitev frame-ov: kateri frame - i pripadajo posameznemu procesu.
    Primer: video ima 100 frame-ov, procesov je 4
    Potem dobiš približno:
        rank 0: frame 0–24
        rank 1: frame 25–49
        rank 2: frame 50–74
        rank 3: frame 75–99
    */
    // Za inicializacijo "bufferja lokalniPodatki in lokalniRezultati"
    int zacetek, konec;
    razdeliFrame(stFrameov, size, rank, zacetek, konec);
    int lokalnoStevilo = konec - zacetek; // fram-i

    // razdelitev frame-ov
    std::vector<int> sendcounts(size); // Pove, koliko bajtov dobi vsak proces
    std::vector<int> displs(size); // Pove, od kod v velikem bufferju se začnejo podatki za posamezni proces
    if (rank == 0)
    {
        for (int r = 0; r < size; r++)
        {
            int z, k;
            razdeliFrame(stFrameov, size, r, z, k);

            int f = k - z; // koliko frame-ov bo ta proces obdelal
            sendcounts[r] = f * velikostVhodnegaFrame;
            displs[r] = z * velikostVhodnegaFrame;
        }
    }

    /*
    razdeli frame-e. rank 0 razdeli podatke med vse procese
    Vsak proces dobi samo svoj del videa, ki jih shrani v "lokalniPodatki"
    Zakaj Scatterv, ne navaden Scatter: 
        Ker število frame-ov ni nujno lepo deljivo s številom procesov
        Scatterv omogoča, da vsak proces dobi različno količino podatkov
    */
    std::vector<unsigned char> lokalniPodatki(lokalnoStevilo * velikostVhodnegaFrame);
    MPI_Scatterv(
        rank == 0 ? vsiPodatki.data() : nullptr, // velik bufferju
        rank == 0 ? sendcounts.data() : nullptr, // koliko bajtov dobi vsak proces
        rank == 0 ? displs.data() : nullptr, // od kod v velikem bufferju se začnejo podatki za posamezni proces
        MPI_UNSIGNED_CHAR,
        lokalniPodatki.data(), // podatki
        lokalnoStevilo * velikostVhodnegaFrame, // število bajtov
        MPI_UNSIGNED_CHAR,
        0,
        MPI_COMM_WORLD
    );

    // počaka, da vsi procesi pridejo do iste točke, nato se začne merjenje časa.
    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();

    /*
    Vsak proces obdela svoje frame-e in shrani v "lokalniRezultati"
    buffer rezultatov je sedaj manjši saj je izhodna slika sivinska in imamo samo 1 - kanal
    */
    std::vector<unsigned char> lokalniRezultati(lokalnoStevilo * velikostIzhodnegaFrame);
    for (int i = 0; i < lokalnoStevilo; i++)
    {
        unsigned char* zacetekFrame = lokalniPodatki.data() + i * velikostVhodnegaFrame; // bajti frame-a
        cv::Mat frame(visina, sirina, CV_8UC3, zacetekFrame); // Vsak proces iz svojih bajtov ponovno sestavi OpenCV sliko
        cv::Mat obdelan = SobelEdgeDetection(frame); // izvedba detekcija robov

        if (!obdelan.isContinuous())
            obdelan = obdelan.clone();

        // Rezultat se kopira v lokalni buffer
        memcpy(
            lokalniRezultati.data() + i * velikostIzhodnegaFrame, // to je naslov oz. mesto, kjer se začne i-ti frame obdelane slike
            obdelan.data, // podatek i-tega frame-a
            velikostIzhodnegaFrame // število bajtov, ki jih kopiramo za en frame
        );
    }

    // počaka, da vsi procesi pridejo do iste točke, nato se konča merjenje časa.
    MPI_Barrier(MPI_COMM_WORLD);
    double t1 = MPI_Wtime();

    /*
    Zbiranje rezultatnih frame-ov iz provesov.
    vsi procesi pošljejo svoje obdelane frame-e nazaj procesu 0
    Rank 0 dobi rezultate v "vsiRezultati"
    */
    std::vector<int> recvcounts(size); // Pove, koliko bajtov dobimo od vsakega procesa
    std::vector<int> recvdispls(size); // Pove, od kod v "lokalniRezultati" se začnejo podatki od posameznega proces
    if (rank == 0)
    {
        for (int r = 0; r < size; r++)
        {
            int z, k;
            razdeliFrame(stFrameov, size, r, z, k);

            int f = k - z; // koliko frame-ov bo dobil od procesa
            recvcounts[r] = f * velikostIzhodnegaFrame;
            recvdispls[r] = z * velikostIzhodnegaFrame;
        }
    }

    // priprava bufferja za rezultate
    std::vector<unsigned char> vsiRezultati;
    if (rank == 0)
    {
        vsiRezultati.resize(stFrameov * velikostIzhodnegaFrame);
    }

    // zbiranje rezultatov iz procesov.
    MPI_Gatherv(
        lokalniRezultati.data(), // podatki
        lokalnoStevilo * velikostIzhodnegaFrame, // število bajtov
        MPI_UNSIGNED_CHAR,
        rank == 0 ? vsiRezultati.data() : nullptr, // buffer vseh rezultatov
        rank == 0 ? recvcounts.data() : nullptr, // koliko bajtov dobimo od vsakega procesa
        rank == 0 ? recvdispls.data() : nullptr, // od kod v bufferju lokalnih rezultatov se začnejo podatki procesa
        MPI_UNSIGNED_CHAR,
        0,
        MPI_COMM_WORLD
    );

    // Rank 0 prikaže končni video. izhodni video je sivinski
    if (rank == 0)
    {
        std::vector<cv::Mat> vsiEdgeFramei;
        for (int i = 0; i < stFrameov; i++)
        {
            // iz bajtov ponovno sestavi OpenCV sliko
            cv::Mat frameEdge(visina, sirina, CV_8UC1, vsiRezultati.data() + i * velikostIzhodnegaFrame);
            vsiEdgeFramei.push_back(frameEdge);
        }

        std::cout << "\n\n==============================================================\n";
        std::cout << "Velikost: " << sirina << " x " << visina << std::endl;
        std::cout << "FPS: " << fps << std::endl;
        std::cout << "Stevilo frame-ov: " << stFrameov << std::endl;
        std::cout << "Stevilo procesov: " << size << std::endl;
        std::cout << "Cas obdelave: " << (t1 - t0) << " s" << std::endl;
        std::cout << "==============================================================\n\n";

        // prikaz videa
        for (int i = 0; i < vsiEdgeFramei.size(); i++)
        {
            // video
            cv::imshow("Frame", vsiEdgeFramei[i]);

            // ESC za exit
            if (cv::waitKey(25) == 27)
                break;
        }
    }

    MPI_Finalize();
    return 0;
}

void razdeliFrame(int stFrameov, int size, int rank, int& zacetek, int& konec)
{
    int osnovniDel = stFrameov / size;
    int ostanek = stFrameov % size;

    zacetek = rank * osnovniDel + cv::min(rank, ostanek);
    int stevilo = osnovniDel + (rank < ostanek ? 1 : 0);
    konec = zacetek + stevilo;
}

cv::Mat SobelEdgeDetection(const cv::Mat frame)
{
    cv::Mat frameGauss, frameGray, frameEdge;

    cv::Mat gx, gy;
    cv::Mat absGx, absGy;

    // z uporabo Gaussovega filtra odstranimo šum (kernel size = 3)
    cv::GaussianBlur(frame, frameGauss, cv::Size(3, 3), 0, 0, cv::BORDER_DEFAULT);

    // pretvorba slike v sivinsko
    cvtColor(frameGauss, frameGray, cv::COLOR_BGR2GRAY);

    // operacija Sobel
    Sobel(frameGray, gx, CV_16S, 1, 0, 3);
    Sobel(frameGray, gy, CV_16S, 0, 1, 3);

    // Pretvorimo nazaj v 8-bit
    convertScaleAbs(gx, absGx);
    convertScaleAbs(gy, absGy);

    addWeighted(absGx, 0.5, absGy, 0.5, 0, frameEdge);

    // slika robov
    return frameEdge;
}

cv::String GetDesktopPath()
{
    char path[MAX_PATH];

    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOP, NULL, 0, path)))
    {
        return path;
    }

    return path;
}

bool fileExists(const cv::String name) {
    std::ifstream f(name.c_str());
    return f.good();
}