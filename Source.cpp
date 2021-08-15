#pragma warning(disable:4146)
#pragma warning(disable:4996)

#include <Windows.h>
#include <ctime>
#include <iostream>
#include <SDL.h>

#include <fstream>
#include <conio.h>

#define OFF_COLOUR 0x00
#define ON_COLOUR 0xFF

// Limit loop rate for visibility
#define LIMIT_RATE 0
// Tick-rate in milliseconds (if LIMIT_RATE == 1)
#define TICK_RATE 50

// CELL STRUCTURE
/*
Celija se cuva u 8-bit char-u
gdje 0. bit reprezentuje stanje celije
a bitovi 1-4 reprezentuju broj susjeda te celije.
Ostali bitovi se ne koriste.
*/

// Cell map dimensions
unsigned int cellmap_width;
unsigned int cellmap_height;

// Visina i sirina celije u pikselima
unsigned int cell_size = 1;

// Randomisation seed
unsigned int seed;

//Flag for braking loops
bool flag = false;

// Graphics
SDL_Window* window = NULL;
SDL_Surface* surface = NULL;
unsigned int s_width;//sirina ekrana
unsigned int s_height;//visina ekrana
std::string imgPath="";

// CellMap stores an array of cells with their states
class CellMap
{
public:
	CellMap(unsigned int w, unsigned int h);
	~CellMap();
	void SetCell(unsigned int x, unsigned int y);
	void ClearCell(unsigned int x, unsigned int y);
	void DrawCell(unsigned int x, unsigned int y, unsigned int colour); //crtanje pixela

	int readBMP();
	int CellState(unsigned int x,unsigned int y);
	void NextGen();
	void Init();
private:
	unsigned char* cells;
	unsigned char* temp_cells;//kopija celija koje cuvam prilikom racunanja sledece generacije
	unsigned int width;
	unsigned int height;
	unsigned int length_in_bytes;
};


int CellMap::readBMP()
{
	FILE* fin, *fout;
	unsigned char info[54];
	int  cell = 0;
	int init_length,x,y;
	const char* path = imgPath.c_str();
	if ((fin = fopen(path, "rb")) != NULL)
	{
		fread(info, sizeof(unsigned char), 54, fin);
		width = *(int*)&info[18];
		height = *(int*)&info[22];

		int r,g,b;
		while ((r = fgetc(fin) != -1 && (b = fgetc(fin)) != -1) && (r = fgetc(fin)) != -1)
		{
			x = cell % width;
			y = (cell - x) / width;
			if (b == 255)
				SetCell(x, y);
			cell++;
		}
	}
	else
		std::cout << "greska prilikom otvaranja izlazne slike";
	return 0;
}

int main(int argc, char* argv[])
{
	int numOfSnep=-1;
	char c;
	std::cout << "Velicinu prozora: " << std::endl;
	std::cin >> cellmap_width>>cellmap_height;
	s_width = cellmap_width * cell_size;
	s_height = cellmap_height * cell_size;

	std::cout << "Da li zelite napraviti snapshoot simulacije? [y/n]" << std::endl;
	c = _getch();

	if (c == 'y')
	{
		std::cout << "Koju generaciju zelite sacuvati?";
		std::cin >> numOfSnep;
	}
	
	std::cout << "Unesite naziv slike: ";
	std::cin >> imgPath;

	// SDL boilerplate
	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("Conway's Game of Life", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, s_width, s_height, SDL_WINDOW_SHOWN);
	surface = SDL_GetWindowSurface(window);

	SDL_Surface* imageSurface = NULL;//ovo

	// Generation counter
	unsigned long generation = 0;

	// Initialise cell map
	CellMap current_map(cellmap_width, cellmap_height);

	//current_map.Init(); // Randomly initialize cell map
	current_map.readBMP();

	// SDL Event handler
	SDL_Event e;
	bool quit = false;
	while (!quit)
	{
		while (SDL_PollEvent(&e) != 0)
			if (e.type == SDL_QUIT) quit = true;

		// Recalculate and draw next generation
		current_map.NextGen();
		if (generation == numOfSnep)
			SDL_SaveBMP(surface, "snapshoot.bmp");
		generation++;
		// Update frame buffer
		SDL_UpdateWindowSurface(window);

#if LIMIT_RATE
		SDL_Delay(TICK_RATE);
#endif
	}

	// Destroy window 
	SDL_DestroyWindow(window);
	// Quit SDL subsystems 
	SDL_Quit();

	std::cout << "Total Generations: " << generation
		<< "\nSeed: " << seed << std::endl;

	system("pause");

	return EXIT_SUCCESS;
}


CellMap::CellMap(unsigned int w, unsigned int h)
{
	width = w;
	height = h;
	length_in_bytes = w * h;
	cells = new unsigned char[length_in_bytes];  
	temp_cells = new unsigned char[length_in_bytes]; 
	memset(cells, 0, length_in_bytes);  // setujem sve na 0 jer prva tri bita se ne koriste pa moraju biti 0
}

CellMap::~CellMap()
{
	delete[] cells;
	delete[] temp_cells;
}

void CellMap::SetCell(unsigned int x, unsigned int y)
{
	int w = width, h = height;
	int xoleft, xoright, yoabove, yobelow;
	unsigned char* cell_ptr = cells + (y * w) + x;//index celije koju setujemo

	// racunanje offseta susjednih celija,
	// setovanje celija da bi se dobio efekat beskonacnog polja, (kao u zmijici)
	xoleft = (x == 0) ? w - 1 : -1;
	xoright = (x == (w - 1)) ? -(w - 1) : 1;
	yoabove = (y == 0) ? length_in_bytes - w : -w;
	yobelow = (y == (h - 1)) ? -(length_in_bytes - w) : w;

 	*(cell_ptr) |= 0x01; // setovaanje prvog bita na 1

	// setovanje sukcesivnih bita za brojanje susjeda
	*(cell_ptr + yoabove + xoleft) += 0x02;
	*(cell_ptr + yoabove) += 0x02;
	*(cell_ptr + yoabove + xoright) += 0x02; 
	*(cell_ptr + xoleft) += 0x02;
	*(cell_ptr + xoright) += 0x02;
	*(cell_ptr + yobelow + xoleft) += 0x02;
	*(cell_ptr + yobelow) += 0x02;
	*(cell_ptr + yobelow + xoright) += 0x02;
}

void CellMap::ClearCell(unsigned int x, unsigned int y)
{
	int w = width, h = height;
	int xoleft, xoright, yoabove, yobelow;
	unsigned char* cell_ptr = cells + (y * w) + x;

	// racunanje offseta susjednih celija,
	// setovanje celija da bi se dobio efekat beskonacnog polja, (kao u zmijici)
	xoleft = (x == 0) ? w - 1 : -1;
	xoright = (x == (w - 1)) ? -(w - 1) : 1;
	yoabove = (y == 0) ? length_in_bytes - w : -w;
	yobelow = (y == (h - 1)) ? -(length_in_bytes - w) : w;


	*(cell_ptr) &= ~0x01; // setovanje prvog bita na 0

	// setovanje sukcesivnih bita za brojanje susjeda
	*(cell_ptr + yoabove + xoleft) -= 0x02;
	*(cell_ptr + yoabove) -= 0x02;
	*(cell_ptr + yoabove + xoright) -= 0x02;
	*(cell_ptr + xoleft) -= 0x02;
	*(cell_ptr + xoright) -= 0x02;
	*(cell_ptr + yobelow + xoleft) -= 0x02;
	*(cell_ptr + yobelow) -= 0x02;
	*(cell_ptr + yobelow + xoright) -= 0x02;
}

int CellMap::CellState(unsigned int x,unsigned int y)
{
	unsigned char* cell_ptr =
		cells + (y * width) + x;

	// Vraca prvi bit (LSB)
	return *cell_ptr & 0x01;
}

void CellMap::NextGen()
{
	unsigned int x, y, count;
	unsigned int h = height, w = width;
	unsigned char* cell_ptr;

	// Copy to temp map to keep an unaltered version
	memcpy(temp_cells, cells, length_in_bytes);

	// Process all cells in the current cell map
	cell_ptr = temp_cells;
	for (y = 0; y < h; y++) {

		x = 0;
		do {

			// Zero bytes are off and have no neighbours so skip them...
			while (*cell_ptr == 0) {
				cell_ptr++; // Advance to the next cell
				// If all cells in row are off with no neighbours go to next row
				if (++x >= w)
				{
					flag = true;
					break;
				}
			}

			if (flag) break;
			// Remaining cells are either on or have neighbours
			count = *cell_ptr >> 1; // # of neighboring on-cells
			if (*cell_ptr & 0x01) {

				// On cell must turn off if not 2 or 3 neighbours
				if ((count != 2) && (count != 3)) {
					ClearCell(x, y);
					DrawCell(x, y, OFF_COLOUR);
				}
			}
			else {

				// Off cell must turn on if 3 neighbours
				if (count == 3) {
					SetCell(x, y);
					DrawCell(x, y, ON_COLOUR);
				}
			}

			// Advance to the next cell byte
			cell_ptr++;

		} while (++x < w);
		flag = false;
	}
}



void CellMap::Init()
{
	unsigned int x, y, init_length;

	// Get random seed
	seed = (unsigned)time(NULL);

	// Random inicijalizuje mapu celija sa ~50% zivih celija
	std::cout << "Initializing" << std::endl;

	srand(seed);
	init_length = (width * height) / 2;
	do
	{
		x = rand() % (width - 1);
		y = rand() % (height - 1);
		if (CellState(x, y) == 0)
			SetCell(x, y);
	} while (--init_length);
}

void CellMap::DrawCell(unsigned int x, unsigned int y, unsigned int colour)
{
	//mnozim sa 4 jer pixel_ptr cuva 4 dijela pixela tj RGB i alpha
	Uint8* pixel_ptr = (Uint8*)surface->pixels + (y * cell_size * s_width + x * cell_size) * 4;

	for (unsigned int i = 0; i < cell_size; i++)
	{
		for (unsigned int j = 0; j < cell_size; j++)
		{
			//setovanje RGB komponenti sa colour 
			*(pixel_ptr + j * 4) = colour;
			*(pixel_ptr + j * 4 + 1) = colour;
			*(pixel_ptr + j * 4 + 2) = colour;
		}
		pixel_ptr += s_width * 4;//prelazak u novi red pixela
	}
}