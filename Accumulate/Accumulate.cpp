#pragma pack(show)

#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <nmmintrin.h>
#include <direct.h>
#include <io.h>

const int STEP_X = 1;
const int STEP_Y = 2;
const int MAX_GRID = 64;
const int MAX_SOLUTIONS = 0x300000;
const unsigned long long UNIT = 0x8000000000000000ull;
const unsigned long long MASK = 0xF100000000000000ull;
const char controlFile[] = "Solutions\\accumulation.txt";
const char solutionFolder[] = "C:\\Users\\compu\\Documents\\My Projects\\Knights\\Placing\\Accumulate\\Solutions";
const char solutionFile[] = "\\solutions.dat";
const char maximisedFile[] = "\\maximised.dat";
const char tidiedFile[] = "\\tidied.dat";
const char duplicatesFile[] = "\\duplicates.dat";
const int STATE_KEEP = 0;
const int STATE_SCRAP = 1;
const int STATE_DUPLICATE = 2;
const int HASH_BORDER = 5;
int multiplier[] = { 1,59,61,67,71,73,79,83,89,97,101,103,107,109,113,127,131,137,139,149,151,157,163,167,173,179,181,191,193,197,199,211,223,227,229,233,239,241,251,257,263,269,271,277,281,283,293,307,311,313,317,331,337,347,349,353,359,367,373,379,383,389,397,401,409 };

struct compressedSolution
{
	long knightLines;
	long coverageLines;
};

struct solution {
	unsigned long long knights[MAX_GRID];
	unsigned long long coverage[MAX_GRID];
};

struct solutionLeaf {
	unsigned long long hash;
	solutionLeaf* bigger;
	solutionLeaf* lesser;
	solutionLeaf* equal;
	long long solutionOffset;
};

struct knight {
	unsigned long long placement[MAX_GRID];
	unsigned long long coverage[MAX_GRID];
};

int knights[MAX_GRID][MAX_GRID];
int knightsYX[MAX_GRID * MAX_GRID][2];
knight* placements[MAX_GRID * MAX_GRID];
solutionLeaf* solutionTree;

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

FILE* mkSolDir(int attack, int coverage, int knight, const char* file, bool read, bool append)
{
	char folderName[1024];
	char folderName2[1024];
	sprintf(folderName, "%s\\K%d", solutionFolder, knight);
	struct stat st;
	if (stat(folderName, &st) != 0)
	{
		_mkdir(folderName);
	}
	sprintf(folderName2, "%s\\A%d", folderName, attack);
	if (stat(folderName2, &st) != 0)
	{
		_mkdir(folderName2);
	}
	sprintf(folderName, "%s\\C%d", folderName2, coverage);
	if (stat(folderName, &st) != 0)
	{
		_mkdir(folderName);
	}
	strcat(folderName, file);
	if (read)
	{
		return fopen(folderName, "rb");
	}
	else
	{
		if (stat(folderName, &st) == 0) {
			if (append)
			{
				return fopen(folderName, "ab");
			}
			else
			{
				printf("File exists already:%s\n", folderName);
				exit(0);
			}
		}
		else
		{
			return fopen(folderName, "wb");
		}
	}

}

void addKnight(solution *sol, int attackSquare, int* coverage) {
	knight* knightPrint = placements[attackSquare];
	if (knightPrint == NULL)
	{
		knightPrint = (knight*)calloc(1, sizeof(knight));
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
	long long cover = 0;
	for (int i = sizeof(solution) / sizeof(long long); i > 0; i--, solPtr++, knightPtr++)
	{
		*solPtr |= *knightPtr;
		if (i <= 64)
		{
			cover += _mm_popcnt_u64(*solPtr);
		}
	}
	*coverage = (int)cover;
}

void initialiseXYs()
{
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
}

void addKnights(int currentAttack, int currentKnight, int currentCoverage)
{
	solution sol;
	FILE* outFile[MAX_GRID * MAX_GRID];
	FILE* inFile = NULL;
	int knightsAttacks[10];
	int knightsAttacksCount = 0;

	memset(outFile, 0, sizeof(outFile));
	memset(placements, 0, sizeof(placements));

	initialiseXYs();

	int y = knightsYX[currentAttack][0];
	int x = knightsYX[currentAttack][1];

	knightsAttacks[knightsAttacksCount] = currentAttack;
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
	if (currentAttack == 0)
	{
		memset(&sol, 0, sizeof(solution));
	}
	else
	{
		inFile = mkSolDir(currentAttack, currentCoverage, currentKnight, tidiedFile, true, false);
		loop = readSolution(&sol, inFile);
	}

	while (loop)
	{
		solution newSol;
		for (int i = 0; i < knightsAttacksCount; i++)
		{
			int coverage = 0;
			_memccpy(&newSol, &sol, 1, sizeof(solution));
			addKnight(&newSol, knightsAttacks[i], &coverage);
			if (outFile[coverage] == NULL)
			{
				outFile[coverage] = mkSolDir(currentAttack, coverage, currentKnight + 1, solutionFile, false, false);
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

void maximiseSolutions(int attack, int knightSoFar, int coverage, bool append)
{
	FILE* inFile;
	FILE* outFile[MAX_GRID * MAX_GRID];
	solution sol;
	solution flippedSol;
	solution* assessedSol;

	memset(outFile, 0, sizeof(outFile));

	if (!append)
	{
		//Clear ALL existing maximum.dat in leaf folders
	}

	inFile = mkSolDir(attack, coverage, knightSoFar, solutionFile, true, false);

	size_t loop = readSolution(&sol, inFile);
	while (loop) {
		bool flip = false;
		unsigned long long column = UNIT;
		for (int row = 0; row < MAX_GRID; row++)
		{
			unsigned long long columnImpact = UNIT;
			unsigned long long columnVal = 0;
			for (int i = 0; i < MAX_GRID; i++)
			{
				if (sol.knights[i] & column)
				{
					columnVal |= columnImpact;
				}
				columnImpact = _rotr64(columnImpact, 1);
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
			column = _rotr64(column, 1);
		}

		if (flip)
		{
			assessedSol = &flippedSol;
			unsigned long long column = UNIT;
			for (int row = 0; row < MAX_GRID; row++)
			{
				unsigned long long columnImpact = UNIT;
				unsigned long long columnVal = 0;
				for (int i = 0; i < MAX_GRID; i++)
				{
					if (sol.coverage[i] & column)
					{
						columnVal |= columnImpact;
					}
					columnImpact = _rotr64(columnImpact, 1);
				}
				flippedSol.coverage[row] = columnVal;
				column = _rotr64(column, 1);
			}
		}
		else
		{
			assessedSol = &sol;
		}

		// find next square to attack
		// In folder

		int newAttack = 0;
		bool exit = false;
		for (int maxLine = 0; (maxLine < MAX_GRID) && !exit; maxLine++)
		{
			unsigned long long test = UNIT;
			for (int x = 0; x <= maxLine; x++)
			{
				if ((assessedSol->coverage[maxLine] & test) == 0)
				{
					exit = true;
					break;
				}
				test = _rotr64(test, 1);
				newAttack++;
			}
			if ((maxLine < MAX_GRID - 1) && !exit)
			{
				for (int y = 0; y <= maxLine; y++)
				{
					if ((assessedSol->coverage[y] & test) == 0)
					{
						exit = true;
						break;
					}
					newAttack++;
				}
			}
		}

		if (outFile[newAttack] == NULL)
		{
			outFile[newAttack] = mkSolDir(newAttack, coverage, knightSoFar, maximisedFile, false, append);
		}

		writeSolution(assessedSol, outFile[newAttack]);

		loop = readSolution(&sol, inFile);
	}

	fclose(inFile);
	for (int i = 0; i < MAX_GRID * MAX_GRID; i++)
	{
		if (outFile[i] != NULL)
		{
			fclose(outFile[i]);
		}
	}
}

void tidySolutions(int attack, int knightNumber, int coverage)
{
	FILE* inFile = mkSolDir(attack, coverage, knightNumber, maximisedFile, true, false);
	FILE* duplicatesOutFile = NULL;
	FILE* tidiedOutFile = NULL;
	solution sol;
	solutionTree = (solutionLeaf *)malloc(sizeof(solutionLeaf) * MAX_SOLUTIONS);
	solutionLeaf* slot = solutionTree;
	unsigned long long count = 0;
	bool error = false;
	initialiseXYs();

	int pos = knightsYX[attack][1] > knightsYX[attack][0] ? knightsYX[attack][1] : knightsYX[attack][0];
	if (pos + HASH_BORDER >= MAX_GRID)
	{
		printf("Cannot hash more than %d rows", (MAX_GRID - HASH_BORDER));
		return;
	}

	long long currentPosition = _ftelli64(inFile);
	size_t loop = readSolution(&sol, inFile);
	unsigned long long mask = _rotr64(MASK, pos);
	while (loop)
	{
		int offset = 0;
		int multiplierIndex = 0;
		unsigned long long rowHash = 0;
		unsigned long long hash = 0;
		for (int row = 0; row < pos; row++)
		{
			unsigned long long part = (sol.coverage[row] & mask);
			if (offset < pos)
			{
				part = _rotl64(part, pos - offset);
			}
			else if (offset > pos)
			{
				part = _rotr64(part, offset - pos);
			}

			rowHash |= part;

			offset += HASH_BORDER;
			if (offset > MAX_GRID)
			{
				offset -= MAX_GRID;
				if (multiplierIndex == 0)
				{
					hash |= rowHash;
				}
				else
				{
					hash |= (rowHash + 1) * multiplier[multiplierIndex];
					multiplierIndex++;
				}
			}
		}

		for (int row = pos; row < pos + HASH_BORDER; row++)
		{
			hash |= (_rotr64(sol.coverage[row], MAX_GRID - HASH_BORDER - pos) + 1) * multiplier[multiplierIndex];
			multiplierIndex++;
		}

		slot->hash = hash;
		slot->bigger = NULL;
		slot->equal = NULL;
		slot->lesser = NULL;
		slot->solutionOffset = currentPosition;

		if (count > 0)
		{
			solutionLeaf* root = solutionTree;
			while (true)
			{
				if (hash == root->hash)
				{
					if (root->equal == NULL)
					{
						root->equal = slot;
						break;
					}
					else
					{
						root = root->equal;
					}
				}
				else if (hash > root->hash)
				{
					if (root->bigger == NULL)
					{
						root->bigger = slot;
						break;
					}
					else
					{
						root = root->bigger;
					}
				}
				else if (hash < root->hash)
				{
					if (root->lesser == NULL)
					{
						root->lesser = slot;
						break;
					}
					else
					{
						root = root->lesser;
					}

				}
			}
		}

		slot++;
		count++;
		currentPosition = _ftelli64(inFile);
		loop = readSolution(&sol, inFile);
		if ((count == MAX_SOLUTIONS) && loop)
		{
			printf("Too many solutions to manage.");
			error = true;
			break;
		}
	}
	fclose(inFile);

	if (error)
	{
		return;
	}

	inFile = mkSolDir(attack, coverage, knightNumber, maximisedFile, true, false);
	tidiedOutFile = mkSolDir(attack, coverage, knightNumber, tidiedFile, false, false);
	slot = solutionTree;
	readSolution(&sol, inFile);
	solution compare;
	while (count)
	{
		if (slot->equal == NULL)
		{
			writeSolution(&sol, tidiedOutFile);
			slot++;
		}
		else
		{
			solutionLeaf* test = slot->equal;
			int state = STATE_KEEP;
			while (test != NULL)
			{
				_fseeki64(inFile, test->solutionOffset, SEEK_SET);
				readSolution(&compare, inFile);
				int cmp = memcmp(sol.coverage, compare.coverage, sizeof(unsigned long long) * MAX_GRID);
				if (cmp == 0)
				{
					cmp = memcmp(sol.knights, compare.knights, sizeof(unsigned long long) * MAX_GRID);
					if (cmp == 0)
					{
						state = STATE_SCRAP;
						break;
					}
					else
					{
						state = STATE_DUPLICATE;
					}
				}
				test = test->equal;
			}

			if (state == STATE_KEEP)
			{
				writeSolution(&sol, tidiedOutFile);
			}
			else if (state == STATE_DUPLICATE)
			{
				if (duplicatesOutFile == NULL)
				{
					duplicatesOutFile = mkSolDir(attack, coverage, knightNumber, duplicatesFile, false, false);
				}
				writeSolution(&sol, duplicatesOutFile);
			}

			slot++;
			_fseeki64(inFile, slot->solutionOffset, SEEK_SET);
		}
		readSolution(&sol, inFile);
		count--;
	}
	fclose(inFile);
	fclose(tidiedOutFile);
	if (duplicatesOutFile != NULL)
	{
		fclose(duplicatesOutFile);
	}

}

void findSolutions()
{
}

int main(int argc, char* argv[]) {

	addKnights(0, 0, 0);
	maximiseSolutions(0, 1, 3, false);
	maximiseSolutions(0, 1, 7, true);
	tidySolutions(1, 1, 3);
	tidySolutions(1, 1, 7);
	addKnights(1, 1, 3);
	addKnights(1, 1, 7);
	maximiseSolutions(1, 2, 7, false);
	maximiseSolutions(1, 2, 9, true);
	maximiseSolutions(1, 2, 10, true);
	maximiseSolutions(1, 2, 12, true);
	maximiseSolutions(1, 2, 14, true);
	maximiseSolutions(1, 2, 16, true);
	tidySolutions(1, 2, 7);
	tidySolutions(1, 2, 10);
	tidySolutions(2, 2, 7);
	tidySolutions(2, 2, 9);
	tidySolutions(2, 2, 10);
	tidySolutions(2, 2, 14);
	tidySolutions(3, 2, 12);
	tidySolutions(3, 2, 16);

	int attack = 0;
	int knightSoFar = 0;
	int coverage = 0;
	if (argc > 4)
	{
		attack = atoi(argv[2]);
		coverage = atoi(argv[3]);
		knightSoFar = atoi(argv[4]);

		if (strcmp(argv[1], "-a") == 0)
		{
			addKnights(attack, knightSoFar, coverage);
		}
		else if (strcmp(argv[1], "-m") == 0)
		{
			maximiseSolutions(attack, knightSoFar, coverage, false);
		}
		else if (strcmp(argv[1], "-f") == 0)
		{
			findSolutions();
		}
	}

	return 0;
}

