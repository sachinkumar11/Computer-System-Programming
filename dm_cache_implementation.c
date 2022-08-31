/*
 * cache.c
 *
 * S SACHIN KUMAR
 *
 */


#include <iostream>
#include <string>
#include <string.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <map>
#include <utility>

using namespace std;

/*a sturcture to represent a line of memory*/
struct MemoryLine
{
    string address;
    string operation;
    string cacheLine;
    string tagBits;
    string byteOffset;
    string HitMiss;
    vector<string> bytes;
};

/*A structure representing a line in the cache*/
struct CacheLine
{
    int valid;
    int modified;
    string tag;
    vector<string> block;
};

/*Global variable section*/
vector<string> ram(1048576, "00");
bool debug = false;
bool refs = false;
string memFileName;
vector<MemoryLine> memoryLines;
map<string, CacheLine> cacheLines;
vector<string> hexVals = {"0", "1", "2", "3",
                          "4", "5", "6", "7",
                          "8", "9", "a", "b",
                          "c", "d", "e", "f"};


/*Initializes an empty cache*/
void initialize_cache()
{
    for (auto el : hexVals)
    {
        /// Sets the elements of each cache line to 0
        struct CacheLine line;
        line.valid = 0;
        line.modified = 0;
        line.tag = "000";
        vector<string> block(16, "00");
        line.block = block;

        cacheLines[el] = line;
    }
}


/*Processes a line of memory nd alters the cache accordingly*/
void process_line(int i)
{
    string cacheLine = memoryLines[i].cacheLine;

    // Hit/Miss processing
    int isValid = cacheLines[cacheLine].valid;
    string cacheTag = cacheLines[cacheLine].tag;
    string memTag = memoryLines[i].tagBits;
    if (!isValid || cacheTag != memTag) // v = 0 or tags don't match (MISS)
    {
        memoryLines[i].HitMiss = "M";
    }
    else if (isValid && cacheTag == memTag) // v = 1 and tags match (HIT)
    {
        memoryLines[i].HitMiss = "H";
    }

    // converts block to hex and gets block address
    string block = memoryLines[i].tagBits + cacheLine + "0";
    int hexBlock = stoi(block, 0, 16);


    // Process the read
    if (memoryLines[i].operation == "R") // READ
    {
        if (memoryLines[i].HitMiss == "M") // Go into RAM
        {
            // If we need to write back
            if (cacheLines[cacheLine].modified)
            {
                // Recompose the addy
                string address = cacheLines[cacheLine].tag + cacheLine + "0";
                int hexAddress = stoi(address, 0, 16);

                // Writeback
                for (int j = 0; j < 16; j++)
                {
                    ram[hexAddress + j] = cacheLines[cacheLine].block[j];
                }

            }

            // Gets the block bytes from ram
            for (int j = 0; j < 16; j++)
            {
                cacheLines[cacheLine].block[j] = ram[hexBlock + j];
            }

            // Updates the cahce validity and tag
            cacheLines[cacheLine].valid = 1;
            cacheLines[cacheLine].modified = 0;
            cacheLines[cacheLine].tag = memTag;
        }

        // Gets the four bytes for the cpu
        string blockOffset = memoryLines[i].byteOffset;
        int cacheOffset = stoi(blockOffset, 0 , 16);
        for (int j = 0; j < 4; j++)
        {
            memoryLines[i].bytes[j] = cacheLines[cacheLine].block[cacheOffset + j];
        }
    }

    // Process the write
    else if (memoryLines[i].operation == "W") //WRITE
    {
        if (memoryLines[i].HitMiss == "M") // Go into RAM
        {
            // If we need to write back
            if (cacheLines[cacheLine].modified)
            {
                // Recompose the addy
                string address = cacheLines[cacheLine].tag + cacheLine + "0";
                int hexAddress = stoi(address, 0, 16);
                
                // Writeback
                for (int j = 0; j < 16; j++)
                {
                    ram[hexAddress + j] = cacheLines[cacheLine].block[j];
                }

            }

            // Gets the block bytes from ram
            for (int j = 0; j < 16; j++)
            {
                cacheLines[cacheLine].block[j] = ram[hexBlock + j];
            }

            // Updates the cache validity and tag
            cacheLines[cacheLine].valid = 1;
            cacheLines[cacheLine].modified = 0;
            cacheLines[cacheLine].tag = memTag;
        }

        // Writes the bytes into the cache
        int offset = stoi(memoryLines[i].byteOffset, 0, 16);
        for (int j = offset; j < offset + 4; j++)
        {
            cacheLines[cacheLine].block[j] = memoryLines[i].bytes[j - offset];
        }

        cacheLines[cacheLine].modified = 1; // Now ram is modified
    }

    else //INVALID
    {
        cout << "Invalid Operation\n";
        exit(-5);
    }
}


/*displays the data cache*/
void display_cache()
{
    cout << "\n";
    cout << "i  V  M  Tag                      Blocks                     " << endl;
    cout << "-  -  -  ---  -----------------------------------------------" << endl;

    for (auto line : cacheLines)
    {
        cout << line. first << "  "
             << line.second.valid << "  "
             << line.second.modified << "  "
             << line.second.tag << "  ";

        // loops through the block 
        for (auto byte : line.second.block)
        {
            cout << byte << " ";
        }

        cout << endl;
    }
}


/*Displays a subset of ram*/
void display_ram()
{
    int currentLine = 0x20000;
    cout << "RAM: " << endl;
    for (int i = 0; i < 256; i++ )
    {
        if (!(i % 16)) // we on a new memory line
        {
            cout << '\n' << hex << currentLine << ":  ";
            currentLine += 0x10;
        }

        // prints a byte of memory
        cout << ram[0x20000 + i] << " ";
    }
}


/*Displays the memory file to the console*/
void process_memFile()
{
    ifstream memFile(memFileName);

    // The file failed to open
    if (memFile.fail())
    {
        cout << "Cannot open file '" << memFileName << "'" <<endl;
        exit(-1);
    }

    string line;

    // Reads through the lines of the file
    while (getline(memFile, line))
    {
        struct MemoryLine memoryline;
        int linePos = 0; // the current index in the file
        string addressLine = ""; // the line of memory that will be displayed
        
        // Gets the memory address
        while (line[linePos] != ' ')
        {
            addressLine += line[linePos];
            linePos++;
        }
        // Adds leading 0s if need be
        int addressLen = addressLine.length();
        if (addressLen < 5)
        {
            for (int i = 0; i < (5-addressLen); i++)
            {
                addressLine.insert(0, "0");
            }
        }
        memoryline.address = addressLine;

        // Adds the read or write signifier to the memory line
        linePos++;
        memoryline.operation = line[linePos];

        // Zero out bytes to pass to the cpu
        memoryline.bytes.insert(memoryline.bytes.end(), {"00", "00", "00", "00"});

        if (memoryline.operation == "W") // We need the 4 bytes to write
        {
            linePos++;
            for (int j = 0; j < 4; j++) // Grabs each byte
            {
                string byte = string(1, line[linePos + 1]) + string(1, line[linePos + 2]);
                memoryline.bytes[j] = byte;
                linePos += 3;
            }
            linePos -= 13;
        }

        // Adds the cache line number to the memory line
        linePos -= 3;
        memoryline.cacheLine = line[linePos];

        // Adds the tag bits to the memory ine
        linePos = 0;
        while (linePos < 3)
        {
            memoryline.tagBits += addressLine[linePos];
            linePos++;
        }

        // Adds the byte offset to the memory line
        linePos++;
        memoryline.byteOffset = addressLine[linePos];

        memoryLines.push_back(memoryline);
    }
    memFile.close();
}


/*Main entry point for the program*/
int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        cout << "No arguements provided to function" << endl;
        exit(-4);
    }

    //Loops through the command line arguements
    for (int i = 1; i < argc; i++)
    {
        // If the memory reference is set
        if (strcmp(argv[i], "-refs") == 0)
        {
            i++;
            if (i == argc || strcmp(argv[i], "-debug") == 0)
            {
                cout << "-refs must be follwed by a file" << endl;
                exit(-2);
            }
            memFileName = argv[i];
            refs = true;
        }

        // If the debug option is set
        else if (strcmp(argv[i], "-debug") == 0)
        {
            debug = true;
        }

        // Invalid command line arguement
        else 
        {
            if (refs)
            {
                cout << "Program ignoring arguement: " << argv[i] << endl;
            }
        }
    }

    // If refs is not set, the program will fail so exit
    if (!refs)
    {
        cout << "No memory reference file provided" << endl;
        exit(-3);
    }

    initialize_cache(); // initializes an empty cache

    if (debug) // displays an empty cache
    {
        display_cache();
    }

    process_memFile(); // displays the memory contents

    cout << "\n";
    int i = 0; //counter because i'm to lazy to change from range based loop
    // Processes each line in the memory file
    for (auto memoryline : memoryLines)
    {
        process_line(i);

        // Prints the line of memory
        cout << "\nLine: ";
        cout << memoryline.address << " " 
             << memoryline.operation << " "
             << memoryline.cacheLine << " "
             << memoryline.tagBits << " "
             << memoryline.byteOffset << " "
             << memoryLines[i].HitMiss << " ";

        for (auto byte : memoryLines[i].bytes)
        {
            cout << byte << " ";
        }

        cout << '\n';

        if (debug)
        {
            display_cache();
        }
        
        i++;
    }
    cout << "\nFinal Cache:\n" ;

    display_cache(); // displays the data cache
    cout << '\n';

    display_ram(); // displays the ram
    cout << "\n\n";
}
