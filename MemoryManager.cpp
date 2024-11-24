#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using namespace std;

// Función para dividir una cadena en pages de un tamaño específico
vector<string> pagination(const string &texto, int tamano)
{
    vector<string> pages;
    for (size_t i = 0; i < texto.size(); i += tamano)
    {
        pages.push_back(texto.substr(i, tamano));
    }
    return pages;
}

// Función para dividir el archivo en segment y pages
vector<vector<string>> memoryAllocation(const string &rutaArchivo, int segmentSize, int pageSize)
{
    ifstream archivo(rutaArchivo);
    if (!archivo.is_open())
    {
        cerr << "No se pudo abrir el archivo: " << rutaArchivo << endl;
        return {};
    }

    vector<vector<string>> segment;
    vector<string> currentSegment;
    string line;
    int lineCounter = 0;

    while (getline(archivo, line))
    {
        currentSegment.push_back(line);
        lineCounter++;

        // Si alcanzamos el límite de líneas por parte, procesamos la parte
        if (lineCounter == segmentSize)
        {
            vector<string> pages;
            string segmentString;
            for (const auto &l : currentSegment)
            {
                segmentString += l + "\n";
            }auto pageInProgress = pagination(segmentString, pageSize);
            pages.insert(pages.end(), pageInProgress.begin(), pageInProgress.end());
            segment.push_back(pages);
            currentSegment.clear();
            lineCounter = 0;
        }
    }

    // Procesar la última parte si quedó incompleta
    if (!currentSegment.empty())
    {
        vector<string> pages;
        for (const auto &l : currentSegment)
        {
            auto pageInProgress = pagination(l, pageSize);
            pages.insert(pages.end(), pageInProgress.begin(), pageInProgress.end());
        }
        segment.push_back(pages);
    }

    archivo.close();
    return segment;
}

void uploadToRam2(const string &pathRAM, const string &pathSwap, const vector<vector<string>> &absoluteProgram)
{
    // Leer ambos archivos JSON existentes
    ifstream jsonRAM_File(pathRAM);
    ifstream jsonSwap_File(pathSwap);

    json jsonRAM;
    json jsonSwap;

    // Manejo del JSON principal
    if (jsonRAM_File.is_open())
    {
        jsonRAM_File >> jsonRAM;
        jsonRAM_File.close();
    }
    else
    {
        cerr << "No se pudo abrir el archivo principal JSON: " << pathRAM << endl;
        return;
    }

    // Manejo del JSON secundario
    if (jsonSwap_File.is_open())
    {
        jsonSwap_File >> jsonSwap;
        jsonSwap_File.close();
    }
    else
    {
        cerr << "No se pudo abrir el archivo secundario JSON: " << pathSwap << endl;
        return;
    }

    int idRAM = 0;
    int idSwap = 0;

    // Iterar sobre las partes y pages para organizarlas en los JSON
    for (size_t i = 0; i < absoluteProgram.size(); ++i)
    {
        const auto &pages = absoluteProgram[i];

        // Guardar la primera subparte en el JSON principal
        if (!pages.empty())
        {
            // Buscar el próximo campo "libre" en el JSON principal
            while (idRAM < jsonRAM["frames"].size() && jsonRAM["frames"][idRAM]["libre"] == false)
            {
                idRAM++; // Saltar campos ocupados
            }

            if (idRAM >= jsonRAM["frames"].size())
            {
                cerr << "Advertencia: más pages principales que entradas libres en el principal JSON." << endl;
                return;
            }

            // Actualizar la entrada correspondiente en el JSON principal
            jsonRAM["frames"][idRAM]["segment_id"] = static_cast<int>(i + 1);
            jsonRAM["frames"][idRAM]["page_number"] = 1;
            jsonRAM["frames"][idRAM]["content"] = pages[0];
            jsonRAM["frames"][idRAM]["is_free"] = false; // Marcar como ocupado
        }

        // Guardar las pages restantes en el JSON secundario
        for (size_t j = 1; j < pages.size(); ++j)
        {
            // Buscar el próximo campo "libre" en el JSON secundario
            while (idSwap < jsonSwap["frames"].size() && jsonSwap["frames"][idSwap]["libre"] == false)
            {
                idSwap++; // Saltar campos ocupados
            }

            if (idSwap >= jsonSwap["frames"].size())
            {
                cerr << "Advertencia: más pages secundarias que entradas libres en el secundario JSON." << endl;
                return;
            }

            // Actualizar la entrada correspondiente en el JSON secundario
            jsonSwap["frames"][idSwap]["segment_id"] = static_cast<int>(i + 1);
            jsonSwap["frames"][idSwap]["page_number"] = static_cast<int>(j + 1);
            jsonSwap["frames"][idSwap]["content"] = pages[j];
            jsonSwap["frames"][idSwap]["is_free"] = false; // Marcar como ocupado
        }
    }

    // Guardar los JSON actualizados en sus archivos correspondientes
    ofstream jsonRAM_endFile(pathRAM);
    if (jsonRAM_endFile.is_open())
    {
        jsonRAM_endFile << jsonRAM.dump(4); // Escribir el JSON principal formateado con 4 espacios
        jsonRAM_endFile.close();
    }
    else
    {
        cerr << "No se pudo guardar el archivo principal JSON: " << pathRAM << endl;
        return;
    }

    ofstream jsonSwap_endFile(pathSwap);
    if (jsonSwap_endFile.is_open())
    {
        jsonSwap_endFile << jsonSwap.dump(4); // Escribir el JSON secundario formateado con 4 espacios
        jsonSwap_endFile.close();
    }
    else
    {
        cerr << "No se pudo guardar el archivo secundario JSON: " << pathSwap << endl;
        return;
    }

    cout << "JSON principal y secundario actualizados correctamente." << endl;
}

void uploadToRam(const std::string &rutaPrincipalJson, const std::string &rutaSecundarioJson, const std::vector<std::vector<std::string>> &partes)
{
    // Leer ambos archivos JSON existentes
    std::ifstream archivoPrincipalJson(rutaPrincipalJson);
    std::ifstream archivoSecundarioJson(rutaSecundarioJson);

    json jsonPrincipal;
    json jsonSecundario;

    // Manejo del JSON principal
    if (archivoPrincipalJson.is_open())
    {
        archivoPrincipalJson >> jsonPrincipal;
        archivoPrincipalJson.close();
    }
    else
    {
        std::cerr << "No se pudo abrir el archivo principal JSON: " << rutaPrincipalJson << std::endl;
        return;
    }

    // Manejo del JSON secundario
    if (archivoSecundarioJson.is_open())
    {
        archivoSecundarioJson >> jsonSecundario;
        archivoSecundarioJson.close();
    }
    else
    {
        std::cerr << "No se pudo abrir el archivo secundario JSON: " << rutaSecundarioJson << std::endl;
        return;
    }

    int idPrincipal = 0;
    int idSecundario = 0;

    // Iterar sobre las partes y subpartes para organizarlas en los JSON
    for (size_t i = 0; i < partes.size(); ++i)
    {
        const auto &subpartes = partes[i];

        // Guardar la primera subparte en el JSON principal
        if (!subpartes.empty())
        {
            // Buscar el próximo campo "libre" en el JSON principal
            while (idPrincipal < jsonPrincipal["frames"].size() && jsonPrincipal["frames"][idPrincipal]["libre"] == false)
            {
                idPrincipal++; // Saltar campos ocupados
            }

            if (idPrincipal >= jsonPrincipal["frames"].size())
            {
                std::cerr << "Advertencia: más subpartes principales que entradas libres en el principal JSON." << std::endl;
                return;
            }

            // Actualizar la entrada correspondiente en el JSON principal
            jsonPrincipal["frames"][idPrincipal]["segment_id"] = static_cast<int>(i + 1);
            jsonPrincipal["frames"][idPrincipal]["page_number"] = 1;
            jsonPrincipal["frames"][idPrincipal]["content"] = subpartes[0];
            jsonPrincipal["frames"][idPrincipal]["libre"] = false; // Marcar como ocupado
        }

        // Guardar las subpartes restantes en el JSON secundario
        for (size_t j = 1; j < subpartes.size(); ++j)
        {
            // Buscar el próximo campo "libre" en el JSON secundario
            while (idSecundario < jsonSecundario["frames"].size() && jsonSecundario["frames"][idSecundario]["libre"] == false)
            {
                idSecundario++; // Saltar campos ocupados
            }

            if (idSecundario >= jsonSecundario["frames"].size())
            {
                std::cerr << "Advertencia: más subpartes secundarias que entradas libres en el secundario JSON." << std::endl;
                return;
            }

            // Actualizar la entrada correspondiente en el JSON secundario
            jsonSecundario["frames"][idSecundario]["segment_id"] = static_cast<int>(i + 1);
            jsonSecundario["frames"][idSecundario]["page_number"] = static_cast<int>(j + 1);
            jsonSecundario["frames"][idSecundario]["content"] = subpartes[j];
            jsonSecundario["frames"][idSecundario]["libre"] = false; // Marcar como ocupado
        }
    }

    // Guardar los JSON actualizados en sus archivos correspondientes
    std::ofstream archivoPrincipalJsonSalida(rutaPrincipalJson);
    if (archivoPrincipalJsonSalida.is_open())
    {
        archivoPrincipalJsonSalida << jsonPrincipal.dump(4); // Escribir el JSON principal formateado con 4 espacios
        archivoPrincipalJsonSalida.close();
    }
    else
    {
        std::cerr << "No se pudo guardar el archivo principal JSON: " << rutaPrincipalJson << std::endl;
        return;
    }

    std::ofstream archivoSecundarioJsonSalida(rutaSecundarioJson);
    if (archivoSecundarioJsonSalida.is_open())
    {
        archivoSecundarioJsonSalida << jsonSecundario.dump(4); // Escribir el JSON secundario formateado con 4 espacios
        archivoSecundarioJsonSalida.close();
    }
    else
    {
        std::cerr << "No se pudo guardar el archivo secundario JSON: " << rutaSecundarioJson << std::endl;
        return;
    }

    std::cout << "JSON principal y secundario actualizados correctamente." << std::endl;
}

int contarLineas(const string &rutaArchivo)
{
    ifstream archivo(rutaArchivo);
    if (!archivo.is_open())
    {
        cerr << "No se pudo abrir el archivo: " << rutaArchivo << endl;
        return -1; // Devuelve -1 si no se puede abrir el archivo
    }

    int contadorLineas = 0;
    string linea;

    while (getline(archivo, linea))
    {
        contadorLineas++;
    }

    archivo.close();
    return contadorLineas;
}

void releaseMemory(const std::string &rutaPrincipalJson, const std::string &rutaSecundarioJson, int process_id)
{
    // Leer ambos archivos JSON existentes
    std::ifstream archivoPrincipalJson(rutaPrincipalJson);
    std::ifstream archivoSecundarioJson(rutaSecundarioJson);

    json jsonPrincipal;
    json jsonSecundario;

    // Manejo del JSON principal
    if (archivoPrincipalJson.is_open())
    {
        archivoPrincipalJson >> jsonPrincipal;
        archivoPrincipalJson.close();
    }
    else
    {
        std::cerr << "No se pudo abrir el archivo principal JSON: " << rutaPrincipalJson << std::endl;
        return;
    }

    // Manejo del JSON secundario
    if (archivoSecundarioJson.is_open())
    {
        archivoSecundarioJson >> jsonSecundario;
        archivoSecundarioJson.close();
    }
    else
    {
        std::cerr << "No se pudo abrir el archivo secundario JSON: " << rutaSecundarioJson << std::endl;
        return;
    }

    // Liberar frames en el JSON principal
    for (auto &frame : jsonPrincipal["frames"])
    {
        if (frame["process_id"] == process_id && !frame["libre"])
        {
            frame["libre"] = true;           // Marcar como libre
            frame["is_free"] = true;         // Actualizar is_free para mantener consistencia
            frame["segment_id"] = 0;         // Reiniciar segment_id
            frame["page_number"] = 0;        // Reiniciar page_number
            frame["content"] = "";           // Limpiar contenido
            frame["process_id"] = 0;         // Reiniciar process_id
            std::cout << "Frame liberado en JSON principal, process_id: " << process_id << std::endl;
        }
    }

    // Liberar frames en el JSON secundario
    for (auto &frame : jsonSecundario["frames"])
    {
        if (frame["process_id"] == process_id && !frame["libre"])
        {
            frame["libre"] = true;           // Marcar como libre
            frame["is_free"] = true;         // Actualizar is_free para mantener consistencia
            frame["segment_id"] = 0;         // Reiniciar segment_id
            frame["page_number"] = 0;        // Reiniciar page_number
            frame["content"] = "";           // Limpiar contenido
            frame["process_id"] = 0;         // Reiniciar process_id
            std::cout << "Frame liberado en JSON secundario, process_id: " << process_id << std::endl;
        }
    }

    // Guardar los JSON actualizados en sus archivos correspondientes
    std::ofstream archivoPrincipalJsonSalida(rutaPrincipalJson);
    if (archivoPrincipalJsonSalida.is_open())
    {
        archivoPrincipalJsonSalida << jsonPrincipal.dump(4); // Escribir el JSON principal formateado con 4 espacios
        archivoPrincipalJsonSalida.close();
    }
    else
    {
        std::cerr << "No se pudo guardar el archivo principal JSON: " << rutaPrincipalJson << std::endl;
        return;
    }

    std::ofstream archivoSecundarioJsonSalida(rutaSecundarioJson);
    if (archivoSecundarioJsonSalida.is_open())
    {
        archivoSecundarioJsonSalida << jsonSecundario.dump(4); // Escribir el JSON secundario formateado con 4 espacios
        archivoSecundarioJsonSalida.close();
    }
    else
    {
        std::cerr << "No se pudo guardar el archivo secundario JSON: " << rutaSecundarioJson << std::endl;
        return;
    }

    std::cout << "Memoria liberada en JSON principal y secundario para process_id: " << process_id << std::endl;
}



int main()
{
    // Ruta al archivo de texto y JSON
    string rutaArchivo = "../ProgramaEjemplo.cpp";
    string rutaRAM = "../RAM.json";
    string rutaSwap = "../Swap.json";
    
    // Parámetros de división
    int segmentSize = ceil(contarLineas(rutaArchivo) / 3.0); // Número de líneas por parte
    int pageSize = 50;                                       // Número de caracteres por subparte

    // Dividir el archivo en segment y pages
    auto result = memoryAllocation(rutaArchivo, segmentSize, pageSize);

    // Actualizar el JSON con las pages generadas
    uploadToRam(rutaRAM, rutaSwap, result);
    
    /*
    int process_id = 0;
    releaseMemory(rutaRAM, rutaSwap, process_id);
    */

    return 0;
}