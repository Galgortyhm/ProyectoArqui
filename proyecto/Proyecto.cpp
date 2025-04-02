#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string>
#include <unistd.h>
#include <sstream>

#define NOMBRE_ARCHIVO_ENTRADA "cybersecurity_attacks.csv"
#define SEPARADOR_LINEA cout << "\t+----------------------------------------------------+" << endl;
#define SEPARADOR_SECCION cout << "\t+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+" << endl;
#define CABECERA_TABLA cout << "\t|  Iteracion   |   Direccion  |   Bloque   |  Resultado   |" << endl \
                           << "\t+----------------------------------------------------+" << endl;

using namespace std;

void simularCacheDirecta(int numBloquesCache, char *punteroMemoriaMapeada, struct stat infoArchivo);
void imprimirContenidoCache1D(int cache[], int numBloquesCache);
void actualizarTiemposLRU(int *pTiemposLRUConjunto, int viasPorConjunto);
void simularCacheAsociativaPorConjuntos(int viasPorConjunto, int cantidadConjuntos, char *punteroMemoriaMapeada, struct stat infoArchivo);
void simularCacheTotalmenteAsociativa(int numBloquesCache, char *punteroMemoriaMapeada, struct stat infoArchivo);
void cargarDireccionesDesdeArchivo(vector<int> &vectorDireccionesDestino);
void mostrarMenu(int *pOpcionMenu, int *pTamanoCacheGlobal, int *pCantidadConjuntos);
void imprimirConPadding(string texto, int anchoMaximo);
unsigned long int direccionHexADecimal(string hexString);
void imprimirEstadisticasCache(double contadorFallos, double contadorAciertos, int totalAccesos);

int main()
{
    int descriptorArchivo = open(NOMBRE_ARCHIVO_ENTRADA, O_RDONLY, S_IRUSR | S_IWUSR);
    struct stat infoArchivo;

    if (fstat(descriptorArchivo, &infoArchivo) == -1)
    {
        perror("Error al obtener el tamaño del archivo");
        exit(EXIT_FAILURE);
    }

    char *punteroMemoriaMapeada = (char *)mmap(NULL, infoArchivo.st_size, PROT_READ, MAP_PRIVATE, descriptorArchivo, 0);

    if (punteroMemoriaMapeada == MAP_FAILED)
    {
        perror("Error al mapear el archivo");
        close(descriptorArchivo);
        exit(EXIT_FAILURE);
    }

    vector<int> direccionesInputNoUsadas;
    cargarDireccionesDesdeArchivo(direccionesInputNoUsadas);

    int opcionMenu = 0;
    int tamanoCacheGlobal = 0;
    int cantidadConjuntos = 0;

    mostrarMenu(&opcionMenu, &tamanoCacheGlobal, &cantidadConjuntos);

    switch (opcionMenu)
    {
    case 1:
        simularCacheDirecta(tamanoCacheGlobal, punteroMemoriaMapeada, infoArchivo);
        break;
    case 2:
        simularCacheAsociativaPorConjuntos(tamanoCacheGlobal, cantidadConjuntos, punteroMemoriaMapeada, infoArchivo);
        break;
    case 3:
        simularCacheTotalmenteAsociativa(tamanoCacheGlobal, punteroMemoriaMapeada, infoArchivo);
        break;
    case 4:
        simularCacheDirecta(tamanoCacheGlobal, punteroMemoriaMapeada, infoArchivo);
        simularCacheAsociativaPorConjuntos(tamanoCacheGlobal, cantidadConjuntos, punteroMemoriaMapeada, infoArchivo);
        simularCacheTotalmenteAsociativa(tamanoCacheGlobal, punteroMemoriaMapeada, infoArchivo);
        break;
    }

    munmap(punteroMemoriaMapeada, infoArchivo.st_size);
    close(descriptorArchivo);

    return 0;
}

std::string formatNumber(double number) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(0) << number;
    std::string s = ss.str();

    int posicionPuntoDecimal = s.find('.');
    if (posicionPuntoDecimal == std::string::npos) {
        posicionPuntoDecimal = s.length();
    }

    for (int i = posicionPuntoDecimal - 3; i > 0; i -= 3) {
        s.insert(i, ".");
    }
    return s;
}

void mostrarMenu(int *pOpcionMenu, int *pTamanoCacheGlobal, int *pCantidadConjuntos)
{
    cout << endl
         << "\t+----------------------- MENU -----------------------+" << endl;
    cout << "\t| Entrada: " << NOMBRE_ARCHIVO_ENTRADA << "  |" << endl;
    cout << "\t|            -----------------------------            |" << endl;
    SEPARADOR_LINEA;
    cout << "\t| Seleccione el tipo de correspondencia              |" << endl;
    cout << "\t| [1] Correspondencia Directa                        |" << endl;
    cout << "\t| [2] Correspondencia Asociativa Por Conjuntos       |" << endl;
    cout << "\t| [3] Correspondencia Completamente Asociativa       |" << endl;
    cout << "\t| [4] Todas                                          |" << endl;
    SEPARADOR_LINEA;
    cout << "\t  Correspondencia tipo: ";
    do
    {
        cin >> *pOpcionMenu;
        if (*pOpcionMenu < 1 || *pOpcionMenu > 4)
            cout << "\t  Opcion invalida, intente de nuevo: ";

    } while (*pOpcionMenu < 1 || *pOpcionMenu > 4);

    cout << "\t  Ingrese el numero de bloques de cache (Total): ";
    do
    {
        cin >> *pTamanoCacheGlobal;
        if (*pTamanoCacheGlobal < 1)
            cout << "\t  Opcion invalida, intente de nuevo: ";

    } while (*pTamanoCacheGlobal < 1);

    if (*pOpcionMenu == 2 || *pOpcionMenu == 4)
    {
        cout << "\t  Ingrese el numero de conjuntos: ";
        do
        {
            cin >> *pCantidadConjuntos;
            if (*pCantidadConjuntos < 1 || *pCantidadConjuntos > *pTamanoCacheGlobal)
                cout << "\t  Opcion invalida (debe ser > 0 y <= numero de bloques ingresado), intente de nuevo: ";

        } while (*pCantidadConjuntos < 1 || *pCantidadConjuntos > *pTamanoCacheGlobal);
    }
    SEPARADOR_LINEA;
    cout << endl;
}

void cargarDireccionesDesdeArchivo(vector<int> &vectorDireccionesDestino)
{
    ifstream streamArchivoEntrada("input.txt");
    if (streamArchivoEntrada.is_open())
    {
        int direccionLeida;
        while (streamArchivoEntrada >> direccionLeida)
        {
            vectorDireccionesDestino.push_back(direccionLeida);
        }
        streamArchivoEntrada.close();
    }
    else
    {
        cerr << "Advertencia: No se pudo abrir 'input.txt'. La simulación procederá con el archivo CSV mapeado." << endl;
    }
}

void simularCacheDirecta(int numBloquesCache, char *punteroMemoriaMapeada, struct stat infoArchivo)
{
    vector<unsigned long int> cache(numBloquesCache, -1);
    double contadorAciertos = 0;
    double contadorFallos = 0;
    int contadorAccesos = 0;

    bool esFallo = false;
    SEPARADOR_SECCION;
    cout << "\t| Correspondencia Directa:                           |" << endl;
    SEPARADOR_SECCION;
    CABECERA_TABLA;

    string direccionHexActual = "";
    for (long long idxByte = 0; idxByte < infoArchivo.st_size; idxByte++)
    {
        if (*(punteroMemoriaMapeada + idxByte) == ',')
        {
            if (!direccionHexActual.empty()) {
                unsigned long int direccionDecimal = direccionHexADecimal(direccionHexActual);
                int indiceBloque = direccionDecimal % cache.size();

                if (cache[indiceBloque] == direccionDecimal)
                {
                    esFallo = false;
                    contadorAciertos++;
                }
                else
                {
                    contadorFallos++;
                    esFallo = true;
                    cache[indiceBloque] = direccionDecimal;
                }
                contadorAccesos++;
            }
            direccionHexActual = "";
        }
        else if (*(punteroMemoriaMapeada + idxByte) == '\n')
        {
             if (!direccionHexActual.empty()) {
                unsigned long int direccionDecimal = direccionHexADecimal(direccionHexActual);
                int indiceBloque = direccionDecimal % cache.size();

                if (cache[indiceBloque] == direccionDecimal)
                {
                    esFallo = false;
                    contadorAciertos++;
                }
                else
                {
                    contadorFallos++;
                    esFallo = true;
                    cache[indiceBloque] = direccionDecimal;
                }
                contadorAccesos++;
            }
            direccionHexActual = "";
        }
        else if (*(punteroMemoriaMapeada + idxByte) != '\r')
        {
            direccionHexActual += *(punteroMemoriaMapeada + idxByte);
        }
    }
     if (!direccionHexActual.empty()) {
        unsigned long int direccionDecimal = direccionHexADecimal(direccionHexActual);
        int indiceBloque = direccionDecimal % cache.size();
        if (cache[indiceBloque] == direccionDecimal) { esFallo = false; contadorAciertos++; }
        else { contadorFallos++; esFallo = true; cache[indiceBloque] = direccionDecimal; }
        contadorAccesos++;
     }

    imprimirEstadisticasCache(contadorFallos, contadorAciertos, contadorAccesos);
}

void imprimirEstadisticasCache(double contadorFallos, double contadorAciertos, int totalAccesos) {
    SEPARADOR_SECCION;
    if (totalAccesos > 0) {
        std::cout << "\t| Fallos: " << formatNumber(contadorFallos) << std::endl;
        std::cout << "\t| Aciertos: " << formatNumber(contadorAciertos) << std::endl;
        double porcentajeAciertos = (contadorAciertos / totalAccesos) * 100.0;
        std::cout << "\t| Porcentaje de aciertos: " << fixed << setprecision(2) << porcentajeAciertos << "%" << std::endl;
    } else {
        std::cout << "\t| No se procesaron accesos." << std::endl;
    }
    SEPARADOR_SECCION;
    std::cout << std::endl;
}

void imprimirConPadding(string texto, int anchoMaximo)
{
    int longitudTexto = texto.length();
    int paddingTotal = anchoMaximo - longitudTexto;
    int paddingIzquierdo = paddingTotal / 2;
    int paddingDerecho = paddingTotal - paddingIzquierdo;

    for (int k = 0; k < paddingIzquierdo; k++) { cout << " "; }
    cout << texto;
    for (int k = 0; k < paddingDerecho; k++) { cout << " "; }
}

void simularCacheTotalmenteAsociativa(int numBloquesCache, char *punteroMemoriaMapeada, struct stat infoArchivo)
{
    vector<unsigned long int> cache(numBloquesCache, -1);
    double contadorAciertos = 0;
    double contadorFallos = 0;
    int contadorAccesos = 0;

    int indiceBloqueDestino = -1;
    bool esFallo = false;
    SEPARADOR_SECCION;
    cout << "\t|  Correspondencia Completamente Asociativa:        |" << endl;
    SEPARADOR_SECCION;
    CABECERA_TABLA;

    string direccionHexActual = "";
    for (long long idxByte = 0; idxByte < infoArchivo.st_size; idxByte++)
    {
        if (*(punteroMemoriaMapeada + idxByte) == ',')
        {
           if (!direccionHexActual.empty()) {
                unsigned long int direccionDecimal = direccionHexADecimal(direccionHexActual);
                indiceBloqueDestino = -1;
                esFallo = true;
                for (int k = 0; k < cache.size(); k++)
                {
                    if (cache[k] == direccionDecimal)
                    {
                        contadorAciertos++;
                        esFallo = false;
                        indiceBloqueDestino = k;
                        break;
                    }
                }

                bool hayEspacioLibre = false;
                if (esFallo)
                {
                    contadorFallos++;
                    for (int k = 0; k < cache.size(); k++)
                    {
                        if (cache[k] == -1)
                        {
                            indiceBloqueDestino = k;
                            cache[k] = direccionDecimal;
                            hayEspacioLibre = true;
                            break;
                        }
                    }
                    if (!hayEspacioLibre)
                    {
                        indiceBloqueDestino = rand() % cache.size();
                        cache[indiceBloqueDestino] = direccionDecimal;
                    }
                }
                contadorAccesos++;
            }
            direccionHexActual = "";
        }
        else if (*(punteroMemoriaMapeada + idxByte) == '\n')
        {
             if (!direccionHexActual.empty()) {
                unsigned long int direccionDecimal = direccionHexADecimal(direccionHexActual);
                indiceBloqueDestino = -1; esFallo = true;
                for (int k = 0; k < cache.size(); k++) {
                    if (cache[k] == direccionDecimal) { contadorAciertos++; esFallo = false; indiceBloqueDestino = k; break; }
                }
                bool hayEspacioLibre = false;
                if (esFallo) {
                    contadorFallos++;
                    for (int k = 0; k < cache.size(); k++) {
                        if (cache[k] == -1) { indiceBloqueDestino = k; cache[k] = direccionDecimal; hayEspacioLibre = true; break; }
                    }
                    if (!hayEspacioLibre) { indiceBloqueDestino = rand() % cache.size(); cache[indiceBloqueDestino] = direccionDecimal; }
                }
                contadorAccesos++;
             }
            direccionHexActual = "";
        }
         else if (*(punteroMemoriaMapeada + idxByte) != '\r')
        {
            direccionHexActual += *(punteroMemoriaMapeada + idxByte);
        }
    }
     if (!direccionHexActual.empty()) {
        unsigned long int direccionDecimal = direccionHexADecimal(direccionHexActual);
        indiceBloqueDestino = -1; esFallo = true;
        for (int k = 0; k < cache.size(); k++) { if (cache[k] == direccionDecimal) { contadorAciertos++; esFallo = false; indiceBloqueDestino = k; break; } }
        bool hayEspacioLibre = false;
        if (esFallo) {
            contadorFallos++;
            for (int k = 0; k < cache.size(); k++) { if (cache[k] == -1) { indiceBloqueDestino = k; cache[k] = direccionDecimal; hayEspacioLibre = true; break; } }
            if (!hayEspacioLibre) { indiceBloqueDestino = rand() % cache.size(); cache[indiceBloqueDestino] = direccionDecimal; }
        }
        contadorAccesos++;
     }

    imprimirEstadisticasCache(contadorFallos, contadorAciertos, contadorAccesos);
}

void simularCacheAsociativaPorConjuntos(int viasPorConjunto, int cantidadConjuntos, char *punteroMemoriaMapeada, struct stat infoArchivo)
{
    if (viasPorConjunto <= 0 || cantidadConjuntos <= 0) {
        cerr << "Error: El número de vías y conjuntos debe ser positivo." << endl;
        return;
    }

    vector<vector<unsigned long int>> cache(cantidadConjuntos, vector<unsigned long int>(viasPorConjunto, -1));
    vector<vector<int>> marcasTiempoLRU(cantidadConjuntos, vector<int>(viasPorConjunto, -1));

    double contadorAciertos = 0;
    double contadorFallos = 0;
    int contadorAccesos = 0;

    bool esFallo = false;
    SEPARADOR_SECCION;
    cout << "\t|  Correspondencia Asociativa Por Conjuntos:        | " << endl;
    SEPARADOR_SECCION;
    CABECERA_TABLA;

    string direccionHexActual = "";
    for (long long idxByte = 0; idxByte < infoArchivo.st_size; idxByte++)
    {
        if (*(punteroMemoriaMapeada + idxByte) == ',')
        {
            if (!direccionHexActual.empty()) {
                contadorAccesos++;
                unsigned long int direccionDecimal = direccionHexADecimal(direccionHexActual);
                int indiceConjunto = direccionDecimal % cantidadConjuntos;
                int indiceVia = -1;

                esFallo = true;
                actualizarTiemposLRU(marcasTiempoLRU[indiceConjunto].data(), viasPorConjunto);

                for (int k = 0; k < viasPorConjunto; k++)
                {
                    if (cache[indiceConjunto][k] == direccionDecimal)
                    {
                        contadorAciertos++;
                        esFallo = false;
                        indiceVia = k;
                        marcasTiempoLRU[indiceConjunto][k] = 0;
                        break;
                    }
                }

                bool hayEspacioLibreEnConjunto = false;
                if (esFallo)
                {
                    contadorFallos++;
                    for (int k = 0; k < viasPorConjunto; k++)
                    {
                        if (cache[indiceConjunto][k] == -1)
                        {
                            indiceVia = k;
                            cache[indiceConjunto][k] = direccionDecimal;
                            marcasTiempoLRU[indiceConjunto][k] = 0;
                            hayEspacioLibreEnConjunto = true;
                            break;
                        }
                    }

                    if (!hayEspacioLibreEnConjunto)
                    {
                        int tiempoMaximoLRU = -1;
                        int viaAReemplazar = -1;
                        for (int k = 0; k < viasPorConjunto; k++)
                        {
                            if (marcasTiempoLRU[indiceConjunto][k] != -1 && marcasTiempoLRU[indiceConjunto][k] > tiempoMaximoLRU)
                            {
                                tiempoMaximoLRU = marcasTiempoLRU[indiceConjunto][k];
                                viaAReemplazar = k;
                            }
                        }
                        if(viaAReemplazar == -1) viaAReemplazar = 0;

                        indiceVia = viaAReemplazar;
                        cache[indiceConjunto][indiceVia] = direccionDecimal;
                        marcasTiempoLRU[indiceConjunto][indiceVia] = 0;
                    }
                }
            }
            direccionHexActual = "";
        }
         else if (*(punteroMemoriaMapeada + idxByte) == '\n')
        {
             if (!direccionHexActual.empty()) {
                contadorAccesos++;
                unsigned long int direccionDecimal = direccionHexADecimal(direccionHexActual);
                int indiceConjunto = direccionDecimal % cantidadConjuntos;
                int indiceVia = -1; esFallo = true;
                actualizarTiemposLRU(marcasTiempoLRU[indiceConjunto].data(), viasPorConjunto);
                for (int k = 0; k < viasPorConjunto; k++) {
                    if (cache[indiceConjunto][k] == direccionDecimal) { contadorAciertos++; esFallo = false; indiceVia = k; marcasTiempoLRU[indiceConjunto][k] = 0; break; }
                }
                bool hayEspacioLibreEnConjunto = false;
                if (esFallo) {
                    contadorFallos++;
                    for (int k = 0; k < viasPorConjunto; k++) {
                        if (cache[indiceConjunto][k] == -1) { indiceVia = k; cache[indiceConjunto][k] = direccionDecimal; marcasTiempoLRU[indiceConjunto][k] = 0; hayEspacioLibreEnConjunto = true; break; }
                    }
                    if (!hayEspacioLibreEnConjunto) {
                        int tiempoMaximoLRU = -1; int viaAReemplazar = -1;
                        for (int k = 0; k < viasPorConjunto; k++) { if (marcasTiempoLRU[indiceConjunto][k] != -1 && marcasTiempoLRU[indiceConjunto][k] > tiempoMaximoLRU) { tiempoMaximoLRU = marcasTiempoLRU[indiceConjunto][k]; viaAReemplazar = k; } }
                        if(viaAReemplazar == -1) viaAReemplazar = 0;
                        indiceVia = viaAReemplazar; cache[indiceConjunto][indiceVia] = direccionDecimal; marcasTiempoLRU[indiceConjunto][indiceVia] = 0;
                    }
                }
             }
            direccionHexActual = "";
        }
         else if (*(punteroMemoriaMapeada + idxByte) != '\r')
        {
            direccionHexActual += *(punteroMemoriaMapeada + idxByte);
        }
    }
     if (!direccionHexActual.empty()) {
        contadorAccesos++;
        unsigned long int direccionDecimal = direccionHexADecimal(direccionHexActual);
        int indiceConjunto = direccionDecimal % cantidadConjuntos;
        int indiceVia = -1; esFallo = true;
        actualizarTiemposLRU(marcasTiempoLRU[indiceConjunto].data(), viasPorConjunto);
        for (int k = 0; k < viasPorConjunto; k++) { if (cache[indiceConjunto][k] == direccionDecimal) { contadorAciertos++; esFallo = false; indiceVia = k; marcasTiempoLRU[indiceConjunto][k] = 0; break; } }
        bool hayEspacioLibreEnConjunto = false;
        if (esFallo) {
            contadorFallos++;
            for (int k = 0; k < viasPorConjunto; k++) { if (cache[indiceConjunto][k] == -1) { indiceVia = k; cache[indiceConjunto][k] = direccionDecimal; marcasTiempoLRU[indiceConjunto][k] = 0; hayEspacioLibreEnConjunto = true; break; } }
            if (!hayEspacioLibreEnConjunto) {
                int tiempoMaximoLRU = -1; int viaAReemplazar = -1;
                for (int k = 0; k < viasPorConjunto; k++) { if (marcasTiempoLRU[indiceConjunto][k] != -1 && marcasTiempoLRU[indiceConjunto][k] > tiempoMaximoLRU) { tiempoMaximoLRU = marcasTiempoLRU[indiceConjunto][k]; viaAReemplazar = k; } }
                if(viaAReemplazar == -1) viaAReemplazar = 0;
                indiceVia = viaAReemplazar; cache[indiceConjunto][indiceVia] = direccionDecimal; marcasTiempoLRU[indiceConjunto][indiceVia] = 0;
            }
        }
     }

    imprimirEstadisticasCache(contadorFallos, contadorAciertos, contadorAccesos);
}

void actualizarTiemposLRU(int *pTiemposLRUConjunto, int viasPorConjunto)
{
    for (int k = 0; k < viasPorConjunto; k++)
    {
        if (pTiemposLRUConjunto[k] != -1)
            pTiemposLRUConjunto[k]++;
    }
}

void imprimirContenidoCache1D(int cache[], int numBloquesCache)
{
    for (int k = 0; k < numBloquesCache; k++)
    {
        cout << cache[k] << " ";
    }
    cout << endl;
}

unsigned long int direccionHexADecimal(string hexString)
{
    if (hexString.length() >= 2 && hexString.substr(0, 2) == "0x")
    {
        hexString = hexString.substr(2);
    }
    if (hexString.find_first_not_of("0123456789abcdefABCDEF") != string::npos) {
       return 0;
    }

    unsigned long int valorDecimal = 0;
    stringstream ss;
    ss << hexString;
    ss >> hex >> valorDecimal;

    return valorDecimal;
}