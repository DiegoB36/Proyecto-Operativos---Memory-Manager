#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using namespace std;

struct Frame
{
    std::string content;
    int frame_number;
    bool is_free;
    bool libre;
    int page_number;
    int process_id;
    int segment_id;
};

class MemoryCalculator
{
public:
    MemoryCalculator(const std::vector<Frame> &frames) : frames(frames) {}

    // Método para calcular la memoria disponible
    int calculateAvailableMemory()
    {
        int free_frames = 0;
        for (const auto &frame : frames)
        {
            if (frame.is_free)
            {
                free_frames++;
            }
        }
        return free_frames * FRAME_SIZE;
    }

    // Método para calcular la memoria consumida por un proceso específico
    int calculateMemoryUsedByProcess(int process_id)
    {
        int used_frames = 0;
        for (const auto &frame : frames)
        {
            if (frame.process_id == process_id && !frame.is_free)
            {
                used_frames++;
            }
        }
        return used_frames * FRAME_SIZE;
    }

private:
    std::vector<Frame> frames;
    static const int FRAME_SIZE = 4 * 1024;
};

std::vector<Frame> loadFramesFromJson(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("No se pudo abrir el archivo JSON");
    }

    json j;
    file >> j;

    std::vector<Frame> frames;
    for (const auto &item : j["frames"])
    {
        frames.push_back({item["content"].get<std::string>(),
                          item["frame_number"].get<int>(),
                          item["is_free"].get<bool>(),
                          item["libre"].get<bool>(),
                          item["page_number"].get<int>(),
                          item["process_id"].get<int>(),
                          item["segment_id"].get<int>()});
    }

    return frames;
}

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
            }
            auto pageInProgress = pagination(segmentString, pageSize);
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

void uploadToRam(const std::string &jsonRAMPath, const std::string &swapRAMPath, const std::vector<std::vector<std::string>> &segments)
{
    // Leer ambos archivos JSON existentes
    std::ifstream ramJsonFile(jsonRAMPath);
    std::ifstream swapJsonFile(swapRAMPath);

    json jsonRAM;
    json jsonSwap;

    // Manejo del JSON principal
    if (ramJsonFile.is_open())
    {
        ramJsonFile >> jsonRAM;
        ramJsonFile.close();
    }

    // Manejo del JSON secundario
    if (swapJsonFile.is_open())
    {
        swapJsonFile >> jsonSwap;
        swapJsonFile.close();
    }

    int ramFrame_id = 0;
    int swapFrame_id = 0;

    // Iterar sobre las segments y pages para organizarlas en los JSON
    for (size_t i = 0; i < segments.size(); ++i)
    {
        const auto &pages = segments[i];

        // Guardar la primera subparte en el JSON principal
        if (!pages.empty())
        {
            // Buscar el próximo campo "libre" en el JSON principal
            while (ramFrame_id < jsonRAM["frames"].size() && jsonRAM["frames"][ramFrame_id]["libre"] == false)
            {
                ramFrame_id++; // Saltar campos ocupados
            }

            if (ramFrame_id >= jsonRAM["frames"].size())
            {
                std::cerr << "Memoria RAM Insuficiente" << std::endl;
                return;
            }

            // Actualizar la entrada correspondiente en el JSON principal
            jsonRAM["frames"][ramFrame_id]["segment_id"] = static_cast<int>(i + 1);
            jsonRAM["frames"][ramFrame_id]["page_number"] = 1;
            jsonRAM["frames"][ramFrame_id]["content"] = pages[0];
            jsonRAM["frames"][ramFrame_id]["libre"] = false;
        }

        // Guardar las pages restantes en el JSON secundario
        for (size_t j = 1; j < pages.size(); ++j)
        {
            // Buscar el próximo campo "libre" en el JSON secundario
            while (swapFrame_id < jsonSwap["frames"].size() && jsonSwap["frames"][swapFrame_id]["libre"] == false)
            {
                swapFrame_id++; // Saltar campos ocupados
            }

            if (swapFrame_id >= jsonSwap["frames"].size())
            {
                std::cerr << "Memoria Swap Insuficiente" << std::endl;
                return;
            }

            // Actualizar la entrada correspondiente en el JSON secundario
            jsonSwap["frames"][swapFrame_id]["segment_id"] = static_cast<int>(i + 1);
            jsonSwap["frames"][swapFrame_id]["page_number"] = static_cast<int>(j + 1);
            jsonSwap["frames"][swapFrame_id]["content"] = pages[j];
            jsonSwap["frames"][swapFrame_id]["libre"] = false; // Marcar como ocupado
        }
    }

    // Guardar los JSON actualizados en sus archivos correspondientes
    std::ofstream archivoPrincipalJsonSalida(jsonRAMPath);
    if (archivoPrincipalJsonSalida.is_open())
    {
        archivoPrincipalJsonSalida << jsonRAM.dump(4); // Escribir el JSON principal formateado con 4 espacios
        archivoPrincipalJsonSalida.close();
    }

    std::ofstream archivoSecundarioJsonSalida(swapRAMPath);
    if (archivoSecundarioJsonSalida.is_open())
    {
        archivoSecundarioJsonSalida << jsonSwap.dump(4); // Escribir el JSON secundario formateado con 4 espacios
        archivoSecundarioJsonSalida.close();
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

void releaseMemory(const std::string &jsonRAMPath, const std::string &swapRAMPath, int process_id)
{
    // Leer ambos archivos JSON existentes
    std::ifstream ramJsonFile(jsonRAMPath);
    std::ifstream swapJsonFile(swapRAMPath);

    json jsonRAM;
    json jsonSwap;

    // Manejo del JSON principal
    if (ramJsonFile.is_open())
    {
        ramJsonFile >> jsonRAM;
        ramJsonFile.close();
    }
    else
    {
        std::cerr << "No se pudo abrir el archivo principal JSON: " << jsonRAMPath << std::endl;
        return;
    }

    // Manejo del JSON secundario
    if (swapJsonFile.is_open())
    {
        swapJsonFile >> jsonSwap;
        swapJsonFile.close();
    }
    else
    {
        std::cerr << "No se pudo abrir el archivo secundario JSON: " << swapRAMPath << std::endl;
        return;
    }

    // Liberar frames en el JSON principal
    for (auto &frame : jsonRAM["frames"])
    {
        if (frame["process_id"] == process_id && !frame["libre"])
        {
            frame["libre"] = true;    // Marcar como libre
            frame["is_free"] = true;  // Actualizar is_free para mantener consistencia
            frame["segment_id"] = 0;  // Reiniciar segment_id
            frame["page_number"] = 0; // Reiniciar page_number
            frame["content"] = "";    // Limpiar contenido
            frame["process_id"] = 0;  // Reiniciar process_id
            std::cout << "Frame liberado en JSON principal, process_id: " << process_id << std::endl;
        }
    }

    // Liberar frames en el JSON secundario
    for (auto &frame : jsonSwap["frames"])
    {
        if (frame["process_id"] == process_id && !frame["libre"])
        {
            frame["libre"] = true;    // Marcar como libre
            frame["is_free"] = true;  // Actualizar is_free para mantener consistencia
            frame["segment_id"] = 0;  // Reiniciar segment_id
            frame["page_number"] = 0; // Reiniciar page_number
            frame["content"] = "";    // Limpiar contenido
            frame["process_id"] = 0;  // Reiniciar process_id
            std::cout << "Frame liberado en JSON secundario, process_id: " << process_id << std::endl;
        }
    }

    // Guardar los JSON actualizados en sus archivos correspondientes
    std::ofstream archivoPrincipalJsonSalida(jsonRAMPath);
    if (archivoPrincipalJsonSalida.is_open())
    {
        archivoPrincipalJsonSalida << jsonRAM.dump(4); // Escribir el JSON principal formateado con 4 espacios
        archivoPrincipalJsonSalida.close();
    }
    else
    {
        std::cerr << "No se pudo guardar el archivo principal JSON: " << jsonRAMPath << std::endl;
        return;
    }

    std::ofstream archivoSecundarioJsonSalida(swapRAMPath);
    if (archivoSecundarioJsonSalida.is_open())
    {
        archivoSecundarioJsonSalida << jsonSwap.dump(4); // Escribir el JSON secundario formateado con 4 espacios
        archivoSecundarioJsonSalida.close();
    }
    else
    {
        std::cerr << "No se pudo guardar el archivo secundario JSON: " << swapRAMPath << std::endl;
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

    // MEMORY ALLOCATION

    // Parámetros de división
    int segmentSize = ceil(contarLineas(rutaArchivo) / 3.0); // Número de líneas por parte
    int pageSize = 50;                                       // Número de caracteres por subparte
    auto result = memoryAllocation(rutaArchivo, segmentSize, pageSize);
    uploadToRam(rutaRAM, rutaSwap, result);

    // Consultas a la Memoria
    vector<Frame> frames = loadFramesFromJson(rutaRAM);
    MemoryCalculator memoryCalculator(frames);
    int available_memory = memoryCalculator.calculateAvailableMemory();
    cout << "Memoria disponible: " << available_memory << " KB" << endl;

    /*
    int process_id = 0;
    releaseMemory(rutaRAM, rutaSwap, process_id);
    */

    return 0;
}