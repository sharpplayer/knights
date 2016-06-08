#pragma pack(show)

#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <nmmintrin.h>
#include <direct.h>

const int STEP_X = 1;
const int STEP_Y = 2;
const int MAX_GRID = 64;
const long long UNIT = 0x8000000000000000ull;
const char controlFile[] = "Solutions\\accumulation.txt";
const char solutionFolder[] = "C:\\Users\\compu\\Documents\\My Projects\\Knights\\Placing\\Accumulate\\Solutions\\%d";
const char solutionFile[] = "\\solutions.dat";
const char tidiedFile[] = "\\tidied.dat";

struct compressedSolution 
{
	long knightLines;
	long coverageLines;
};

struct solution {
	unsigned long long knights[MAX_GRID];
	unsigned long long coverage[MAX_GRID];
};

struct knight {
	unsigned long long placement[MAX_GRID];
	unsigned long long coverage[MAX_GRID];
};

int knights[MAX_GRID][MAX_GRID];
int knightsYX[MAX_GRID * MAX_GRID][2];
knight* placements[MAX_GRID * MAX_GRID];

void writeSolution(solution* sol, FILE* file)
{
	compressedSolution compressed;

	compressed.knightLines = MAX_GRID;
	unsigned long long* knights = &sol->knights[compressed.knightLines - 1];
	while (*knights == 0)
	{
		knights--;
		compressed.knightLines--;
	}
	compressed.coverageLines = (compressed.knightLines + STEP_Y) > MAX_GRID ? MAX_GRID : (compressed.knightLines + STEP_Y);
	unsigned long long* coverage = &sol->coverage[compressed.coverageLines - 1];
	while (*coverage == 0)
	{
		coverage--;
		compressed.coverageLines--;
	}
	fwrite(&compressed, sizeof(compressedSolution), 1, file);
	fwrite(&sol->knights, sizeof(unsigned long long), compressed.knightLines, file);
	fwrite(&sol->coverage, sizeof(unsigned long long), compressed.coverageLines, file);
}

size_t readSolution(solution* sol, FILE* file)
{
	compressedSolution compressed;
	memset(sol, 0, sizeof(solution));
	size_t ret = fread(&compressed, sizeof(compressedSolution), 1, file);
	if (ret)
	{
		fread(&sol->knights, sizeof(unsigned long long), compressed.knightLines, file);
		fread(&sol->coverage, sizeof(unsigned long long), compressed.coverageLines, file);
	}
	return ret;
}

void addKnight(solution *sol, int attackSquare, long long* coverage) {
	knight* knightPrint = placements[attackSquare];
	if (knightPrint == NULL)
	{
		knightPrint = (knight*) calloc(1, sizeof(knight));
		int y = knightsYX[attackSquare][0];
		int x = knightsYX[attackSquare][1];
		knightPrint->placement[y] |= _rotr64(UNIT, x);
		knightPrint->coverage[y] |= _rotr64(UNIT, x);
		if ((y >= STEP_X) && (x >= STEP_Y))
		{
			knightPrint->coverage[y - STEP_X] |= _rotr64(UNIT, (x - STEP_Y));
		}
		if (y >= STEP_X) {
			knightPrint->coverage[y - STEP_X] |= _rotr64(UNIT, (x + STEP_Y));
		}
		if (x >= STEP_Y) {
			knightPrint->coverage[y + STEP_X] |= _rotr64(UNIT, (x - STEP_Y));
		}
		knightPrint->coverage[y + STEP_X] |= _rotr64(UNIT, (x + STEP_Y));

		if ((y >= STEP_Y) && (x >= STEP_X))
		{
			knightPrint->coverage[y - STEP_Y] |= _rotr64(UNIT, (x - STEP_X));
		}
		if (y >= STEP_Y)
		{
			knightPrint->coverage[y - STEP_Y] |= _rotr64(UNIT, (x + STEP_X));
		}
		if (x >= STEP_X)
		{
			knightPrint->coverage[y + STEP_Y] |= _rotr64(UNIT, (x - STEP_X));
		}
		knightPrint->coverage[y + STEP_Y] |= _rotr64(UNIT, (x + STEP_X));

		placements[attackSquare] = knightPrint;
	}
	long long* solPtr = (long long*)sol;
	long long* knightPtr = (long long*)knightPrint;
	for (int i = sizeof(solution) / sizeof(long long); i > 0; i--, solPtr++, knightPtr++)
	{
		*solPtr |= *knightPtr;
		if (i <= 64)
		{
			*coverage += _mm_popcnt_u64(*solPtr);
		}
	}
}

void addKnights(int attack)
{
	int knight = 0;
	solution sol;
	FILE* outFile[MAX_GRID * MAX_GRID];
	FILE* inFile = NULL;
	char folderName[1024];
	char fileName[1024];
	int knightsAttacks[10];
	int knightsAttacksCount = 0;

	memset(outFile, 0, sizeof(outFile));
	memset(placements, 0, sizeof(placements));

	sprintf(folderName, solutionFolder, attack);
	struct stat st;
	if (stat(folderName, &st) != 0)
	{
		printf("Making solution directory\n");
		_mkdir(folderName);
	}

	int square = 0;
	for (int maxLine = 0; maxLine < MAX_GRID; maxLine++)
	{
		for (int x = 0; x <= maxLine; x++)
		{
			knightsYX[square][0] = maxLine;
			knightsYX[square][1] = x;
			knights[maxLine][x] = square;
			square++;
		}
		if (maxLine < MAX_GRID - 1)
		{
			for (int y = 0; y <= maxLine; y++)
			{
				knightsYX[square][0] = y;
				knightsYX[square][1] = maxLine + 1;
				knights[y][maxLine + 1] = square;
				square++;
			}
		}
	}

	int x = knightsYX[attack][0];
	int y = knightsYX[attack][1];

	knightsAttacks[knightsAttacksCount] = attack;
	knightsAttacksCount++;
	if ((y >= STEP_X) && (x >= STEP_Y))
	{
		knightsAttacks[knightsAttacksCount] = knights[y - STEP_X][x - STEP_Y];
		knightsAttacksCount++;
	}
	if (y >= STEP_X) {
		knightsAttacks[knightsAttacksCount] = knights[y - STEP_X][x + STEP_Y];
		knightsAttacksCount++;
	}
	if (x >= STEP_Y) {
		knightsAttacks[knightsAttacksCount] = knights[y + STEP_X][x - STEP_Y];
		knightsAttacksCount++;
	}
	knightsAttacks[knightsAttacksCount] = knights[y + STEP_X][x + STEP_Y];
	knightsAttacksCount++;

	if ((y >= STEP_Y) && (x >= STEP_X))
	{
		knightsAttacks[knightsAttacksCount] = knights[y - STEP_Y][x - STEP_X];
		knightsAttacksCount++;
	}
	if (y >= STEP_Y)
	{
		knightsAttacks[knightsAttacksCount] = knights[y - STEP_Y][x + STEP_X];
		knightsAttacksCount++;
	}
	if (x >= STEP_X)
	{
		knightsAttacks[knightsAttacksCount] = knights[y + STEP_Y][x - STEP_X];
		knightsAttacksCount++;
	}
	knightsAttacks[knightsAttacksCount] = knights[y + STEP_Y][x + STEP_X];
	knightsAttacksCount++;



	size_t loop = 1;
	if (attack == 0)
	{
		memset(&sol, 0, sizeof(solution));
	}
	else
	{
		//sprintf(fileName, solutionFolder, attack);
		//strcat(fileName, tidiedFile);
		loop = readSolution(&sol, inFile);
	}

	while (loop)
	{
		solution newSol;
		for (int i = 0; i < knightsAttacksCount; i++)
		{
			long long coverage = 0;
			_memccpy(&newSol, &sol, 1, sizeof(solution));
			addKnight(&newSol, knightsAttacks[i], &coverage);
			if (outFile[coverage] == NULL)
			{
				sprintf(fileName, "%s\\%llu", folderName, coverage);
				if (stat(fileName, &st) != 0)
				{
					_mkdir(fileName);
				}

				strcat(fileName, solutionFile);
				outFile[coverage] = fopen(fileName, "wb");

			}
			writeSolution(&newSol, outFile[coverage]);
		}
		if (inFile)
		{
			loop = readSolution(&sol, inFile);
		}
		else
		{
			loop = 0;
		}
	}

	for (int i = 0; i < MAX_GRID * MAX_GRID; i++)
	{
		if (outFile[i] != NULL)
		{
			fclose(outFile[i]);
		}
	}

}

void tidySolutions(int attack)
{
	char fileName[1024];
	FILE* inFile;
	FILE* outFile;
	solution sol;
	solution flippedSol;
	solution* assessedSol;

	sprintf(fileName, solutionFolder, attack);
	strcat(fileName, solutionFile);
	inFile = fopen(fileName, "wb");

	size_t loop = fread(&sol, sizeof(solution), 1, inFile);
	while(loop) {
		bool flip = false;
		for (int row = 0; row < MAX_GRID; row++)
		{
			unsigned long long column = UNIT;
			unsigned long long columnImpact = 1;
			unsigned long long columnVal = 0;
			for (int i = 0; i < MAX_GRID; i++)
			{
				if (sol.knights[i] & column)
				{
					columnVal |= columnImpact;
					columnImpact = _rotr64(column, 1);
				}
			}
			if (!flip)
			{
				if (sol.knights[row] > columnVal) 
				{
					break;
				}
				else if (sol.knights[row] < columnVal)
				{
					flip = true;
				}
			}
			flippedSol.knights[row] = columnVal;
		}

		if (flip)
		{
			assessedSol = &flippedSol;
			for (int row = 0; row < MAX_GRID; row++)
			{
				unsigned long long column = UNIT;
				unsigned long long columnImpact = 1;
				unsigned long long columnVal = 0;
				for (int i = 0; i < MAX_GRID; i++)
				{
					if (sol.coverage[i] & column)
					{
						columnVal |= columnImpact;
						columnImpact = _rotr64(column, 1);
					}
				}
				flippedSol.coverage[row] = columnVal;
			}
		}
		else
		{
			assessedSol = &sol;
		}




		loop = fread(&sol, sizeof(solution), 1, inFile);
	}
	
	// Are there any duplicate solutions, if so save duplicates
	// Save in new solutions.dat for "next" square
	// Save in accumulation.txt (plus marker to state processed???)



	fclose(inFile);
}

void findSolutions()
{
}


int main(int argc, char* argv[]) {

	addKnights(0);

	if (argc > 1)
	{
		if (strcmp(argv[1], "-a") == 0)
		{
			int attack = 0;
			if (argc > 2)
			{
				attack = atoi(argv[2]);
			}
			addKnights(attack);
		}
		else if (strcmp(argv[1], "-t") == 0)
		{
			int attack = 0;
			if (argc > 2)
			{
				attack = atoi(argv[2]);
			}
			addKnights(attack);
			tidySolutions(attack);
		}
		else if (strcmp(argv[1], "-f") == 0)
		{
			findSolutions();
		}
	}

	return 0;
}

