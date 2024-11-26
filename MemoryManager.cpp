#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using namespace std;

string jsonRAMPath = "../RAM.json";
string jsonSwapPath = "../Swap.json";
string filePath = "../ProgramaEjemplo.cpp";

struct Frame
{
    std::string content;
    int frame_number;
    bool is_free;
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
                          item["page_number"].get<int>(),
                          item["process_id"].get<int>(),
                          item["segment_id"].get<int>()});
    }

    return frames;
}

// Método para calcular la memoria libre de todo el sistema
int freeMem()
{
    vector<Frame> frames = loadFramesFromJson(jsonRAMPath);
    MemoryCalculator memoryCalculator(frames);
    int available_memory = memoryCalculator.calculateAvailableMemory();
    return available_memory;
}

// Función para dividir una cadena en páginas de un tamaño específico
vector<string> pagination(const string &text, int size)
{
    vector<string> pages;
    for (size_t i = 0; i < text.size(); i += size)
    {
        pages.push_back(text.substr(i, size));
    }
    return pages;
}

// Función usada para contar el numero de lineas de un archivo
int countLines(const string &filePath)
{
    ifstream archivo(filePath);
    if (!archivo.is_open())
    {
        cerr << "No se pudo abrir el archivo: " << filePath << endl;
        return -1; // Devuelve -1 si no se puede abrir el archivo
    }

    int line_counter = 0;
    string linea;

    while (getline(archivo, linea))
    {
        line_counter++;
    }

    archivo.close();
    return line_counter;
}

// Función usada para liberar la memoria de un proceso
void releaseMemory(int process_id)
{
    std::ifstream ramJsonFile(jsonRAMPath);
    std::ifstream swapJsonFile(jsonSwapPath);

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
        std::cerr << "No se pudo abrir el archivo secundario JSON: " << jsonSwapPath << std::endl;
        return;
    }

    // Liberar frames en el JSON principal
    for (auto &frame : jsonRAM["frames"])
    {
        if (frame["process_id"] == process_id && !frame["is_free"])
        {
            frame["is_free"] = true;  // Actualizar is_free
            frame["segment_id"] = 0;  // Reiniciar segment_id
            frame["page_number"] = 0; // Reiniciar page_number
            frame["content"] = "";    // Limpiar contenido
        }
    }

    // Borrar tablas de direcciones asociadas al proceso
    jsonRAM["SO"].erase(
        std::remove_if(
            jsonRAM["SO"].begin(),
            jsonRAM["SO"].end(),
            [process_id](const json &item)
            { return item["process_id"] == process_id; }),
        jsonRAM["SO"].end());

    // Liberar frames en el JSON secundario
    for (auto &frame : jsonSwap["frames"])
    {
        if (frame["process_id"] == process_id && !frame["is_free"])
        {
            frame["is_free"] = true;  // Actualizar is_free
            frame["segment_id"] = 0;  // Reiniciar segment_id
            frame["page_number"] = 0; // Reiniciar page_number
            frame["content"] = "";    // Limpiar contenido
        }
    }

    // Guardar los JSON actualizados en sus archivos correspondientes
    std::ofstream archivoPrincipalJsonSalida(jsonRAMPath);
    if (archivoPrincipalJsonSalida.is_open())
    {
        archivoPrincipalJsonSalida << jsonRAM.dump(4);
        archivoPrincipalJsonSalida.close();
    }
    else
    {
        std::cerr << "No se pudo guardar el archivo principal JSON: " << jsonRAMPath << std::endl;
        return;
    }

    std::ofstream archivoSecundarioJsonSalida(jsonSwapPath);
    if (archivoSecundarioJsonSalida.is_open())
    {
        archivoSecundarioJsonSalida << jsonSwap.dump(4);
        archivoSecundarioJsonSalida.close();
    }
    else
    {
        std::cerr << "No se pudo guardar el archivo secundario JSON: " << jsonSwapPath << std::endl;
        return;
    }

    std::cout << "Memoria liberada en JSON principal y secundario para process_id: " << process_id << std::endl;
}

void uploadToRam(const std::vector<std::vector<std::string>> &segments, int process_id)
{
    // Leer ambos archivos JSON existentes
    std::ifstream ramJsonFile(jsonRAMPath);
    std::ifstream swapJsonFile(jsonSwapPath);

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

    // **Verificar si el proceso ya existe en JSON principal**
    auto &soEntries = jsonRAM["SO"];
    for (auto it = soEntries.begin(); it != soEntries.end(); ++it)
    {
        if ((*it)["process_id"] == process_id)
        {
            // Liberar memoria del proceso existente
            releaseMemory(process_id);

            // **Actualizar jsonRAM tras liberar memoria**
            // Leer de nuevo el archivo JSON principal para sincronizar los cambios
            std::ifstream updatedRamJsonFile(jsonRAMPath);
            if (updatedRamJsonFile.is_open())
            {
                updatedRamJsonFile >> jsonRAM;
                updatedRamJsonFile.close();
            }
            else
            {
                std::cerr << "Error al leer JSON principal tras liberar memoria" << std::endl;
                return;
            }

            // **Actualizar jsonSwap tras liberar memoria**
            // Leer de nuevo el archivo JSON secundario para sincronizar los cambios
            std::ifstream updatedSwapJsonFile(jsonSwapPath);
            if (updatedSwapJsonFile.is_open())
            {
                updatedSwapJsonFile >> jsonSwap;
                updatedSwapJsonFile.close();
            }
            else
            {
                std::cerr << "Error al leer JSON secundario tras liberar memoria" << std::endl;
                return;
            }

            break;
        }
    }

    int ramFrame_id = 0;
    int swapFrame_id = 0;

    // Crear un objeto para el proceso en el JSON principal
    json processEntry;
    processEntry["process_id"] = process_id;  // El ID del proceso
    processEntry["segments"] = json::array(); // Inicializar el array de segmentos

    // Iterar sobre las segments y pages para organizarlas en los JSON
    for (size_t i = 0; i < segments.size(); ++i)
    {
        const auto &pages = segments[i];

        // Crear las tablas de paginación para este proceso
        json segmentEntry;
        segmentEntry["segment_id"] = static_cast<int>(i + 1);
        segmentEntry["pages"] = json::array();

        // Guardar las pages restantes en el JSON secundario
        for (size_t j = 0; j < pages.size(); ++j)
        {
            // Buscar el próximo campo "libre" en el JSON secundario
            while (swapFrame_id < jsonSwap["frames"].size() && jsonSwap["frames"][swapFrame_id]["is_free"] == false)
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
            jsonSwap["frames"][swapFrame_id]["process_id"] = process_id;
            jsonSwap["frames"][swapFrame_id]["is_free"] = false; // Marcar como ocupado

            // Añadir la página en la tabla de paginación del segmento
            json pageEntry;
            pageEntry["page_number"] = static_cast<int>(j + 1);
            pageEntry["frame_swap"] = swapFrame_id;
            pageEntry["frame_ram"] = -1;
            pageEntry["presence_bit"] = 0;
            segmentEntry["pages"].push_back(pageEntry);
        }

        // Guardar la primera subparte en el JSON principal
        if (!pages.empty())
        {
            // Buscar el próximo campo "libre" en el JSON principal
            while (ramFrame_id < jsonRAM["frames"].size() && jsonRAM["frames"][ramFrame_id]["is_free"] == false)
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
            jsonRAM["frames"][ramFrame_id]["process_id"] = process_id;
            jsonRAM["frames"][ramFrame_id]["is_free"] = false;

            // Añadir la página en la tabla de paginación del segmento
            segmentEntry["pages"][0]["frame_ram"] = ramFrame_id;
            segmentEntry["pages"][0]["presence_bit"] = 1;
        }

        // Añadir el segmento con sus páginas a la entrada del proceso
        processEntry["segments"].push_back(segmentEntry);
    }
    // Agregar la entrada del proceso a la lista de procesos en RAM
    jsonRAM["SO"].push_back(processEntry);

    // Guardar los JSON actualizados en sus archivos correspondientes
    std::ofstream archivoPrincipalJsonSalida(jsonRAMPath);
    if (archivoPrincipalJsonSalida.is_open())
    {
        archivoPrincipalJsonSalida << jsonRAM.dump(4); // Escribir el JSON principal formateado con 4 espacios
        archivoPrincipalJsonSalida.close();
    }

    std::ofstream archivoSecundarioJsonSalida(jsonSwapPath);
    if (archivoSecundarioJsonSalida.is_open())
    {
        archivoSecundarioJsonSalida << jsonSwap.dump(4); // Escribir el JSON secundario formateado con 4 espacios
        archivoSecundarioJsonSalida.close();
    }

    std::cout << "JSON principal y secundario actualizados correctamente." << std::endl;
}

// Función para dividir el archivo en segment y pages
bool memoryAllocation(int process_id) // solo pid
{
    ifstream archivo(filePath);
    int segmentSize = ceil(countLines(filePath) / 3.0); // Número de líneas por parte
    int pageSize = 50;

    if (!archivo.is_open())
    {
        cerr << "No se pudo abrir el archivo: " << filePath << endl;
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
    uploadToRam(segment, process_id);
    return true;
}

string getPage(int frame_number)
{
    ifstream inputFile(jsonSwapPath);
    json jsonData;
    inputFile >> jsonData;
    inputFile.close();

    string content;
    // Busca el frame que tiene la página y obtiene el contenido
    for (auto &frame : jsonData["frames"])
    {
        if (frame["frame_number"] == frame_number)
        {
            content = frame["content"];
            break;
        }
    }

    return content;
}

void updateTable(int segmento, int pagina, int process_id, int new_page_ram_frame)
{
    ifstream inputFile(jsonRAMPath);
    json jsonData;
    inputFile >> jsonData;
    inputFile.close();

    for (auto &process : jsonData["SO"])
    {
        if (process["process_id"] == process_id)
        {
            for (auto &segmentos : process["segments"])
            {
                if (segmentos["segment_id"] == segmento)
                {
                    for (auto &paginas : segmentos["pages"])
                    {
                        if (paginas["page_number"] == pagina)
                        {
                            paginas["frame_ram"] = new_page_ram_frame;
                            paginas["presence_bit"] = 1;
                        }
                    }
                }
            }
        }
    }

    // Guarda el archivo JSON con los cambios
    ofstream outputFile(jsonRAMPath);
    outputFile << jsonData.dump(4);
    outputFile.close();
}

bool memorySwap(int segmento, int pagina, int process_id)
{
    ifstream inputFile(jsonRAMPath);
    json jsonData;
    inputFile >> jsonData;
    inputFile.close();

    int frame_number_swap = 0;
    int frame_number_Ram = 0;
    for (auto &process : jsonData["SO"])
    {
        if (process["process_id"] == process_id)
        {
            for (auto &segmentos : process["segments"])
            {
                if (segmentos["segment_id"] == segmento)
                {
                    for (auto &paginas : segmentos["pages"])
                    {
                        if (paginas["page_number"] == pagina)
                        {
                            frame_number_swap = paginas["frame_swap"];
                        }
                        if (paginas["presence_bit"] == 1)
                        {
                            frame_number_Ram = paginas["frame_ram"];
                            paginas["frame_ram"] = -1;
                            paginas["presence_bit"] = 0;
                        }
                    }
                }
            }
        }
    }
    int new_ram_frame_assigned = 0;
    bool assigned = false;
    for (auto &frame : jsonData["frames"])
    {
        if (frame["is_free"] && !assigned)
        {
            frame["is_free"] = false;       // Actualizar is_free
            frame["segment_id"] = segmento; // Reiniciar segment_id
            frame["page_number"] = pagina;  // Reiniciar page_number
            frame["content"] = getPage(frame_number_swap);
            new_ram_frame_assigned = frame["frame_number"];
            assigned = true;
        }
        if (frame["frame_number"] == frame_number_Ram)
        {
            frame["is_free"] = true;  // Actualizar is_free
            frame["segment_id"] = 0;  // Reiniciar segment_id
            frame["page_number"] = 0; // Reiniciar page_number
            frame["content"] = "";    // Limpiar contenido
        }
    }

    // Guarda el archivo JSON con los cambios
    ofstream outputFile(jsonRAMPath);
    outputFile << jsonData.dump(4);
    outputFile.close();

    actualizar_tabla(segmento, pagina, process_id, new_ram_frame_assigned);
    return true;
}
int main()
{
    // MEMORY ALLOCATION
    int process_id = 0;

    // bool result = memoryAllocation(process_id);

    // Consultas a la Memoria
    // cout << "Memoria disponible: " << freeMem() << " KB" << endl;

    // releaseMemory(process_id);

    // cout << "Memoria disponible: " << freeMem() << " KB" << endl;
    memorySwap(1, 3, 0);

    return 0;
}