/*
 * Framework for sudoku solver by Anselm Blumer, 26 February, 2015
 * Should be able to handle 9x9, 16x16, and 25x25 (but you may not
 * want to wait for the larger sizes to finish)
 *
 * Modified by Matthew Eads, March 12, 2015
 */

#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <cassert>
#include <cstring>

using namespace std;

#define MAXSIZE 25

// Pick which heuristic you want
#define ARC3 1
#define MNRV 1
#define MXRV 0
#define LCV 0
#define MCV 1

// Position selectors: take a size-2 array as arg, and
// assign the next position to be assigned to the array
// according to the heuristic
void naiveNextPos(int*);
void minimumRemainingValues(int*);
void maximumRemainingValues(int*);

// Value selectors: take a list of vals, and a pos as args,
// and return the next value to try according to heuristic,
// removing that val from the val list.
int  mostConstrainingValue(int*, int, int);
int  leastConstrainingValue(int*, int, int);
int  naiveNextVal(int*, int, int);
int  arc3Check(int, int);
int  naiveArcCheck(int i, int j) {(void) i; (void) j; return 1;}

// Assign next pos according to chosen heurisitc
#if (MNRV)
void (*nextPos)(int*) = &minimumRemainingValues;
#elif (MXRV)
void (*nextPos)(int*) = &maximumRemainingValues;
#else 
void (*nextPos)(int*) = &naiveNextPos;
#endif

#if (ARC3)
int (*arcConsistent)(int, int) = &arc3Check;
#else
int (*arcConsistent)(int, int) = &naiveArcCheck;
#endif

// Assign next val according to chosen heurisitc
#if (LCV)
int (*nextVal)(int*, int, int) = &leastConstrainingValue;
#elif (MCV)
int (*nextVal)(int*, int, int) = &mostConstrainingValue;
#else
int (*nextVal)(int*, int, int) = &naiveNextVal;
#endif

/*
 * Globals:
 * grid is the sudoku grid
 * legal[i][j][k] is 1 if k+1 is a legal value for grid[i][j], 0 if illegal
 *    k+1 since arrays are 0-based
 * size is the edge length of the sudoku grid, 9, 16, or 25
 *    specified on the first line of the input file
 * blocksize is the size of the sub-blocks, sqrt( size )
 */

int grid[MAXSIZE][MAXSIZE];
int legal[MAXSIZE][MAXSIZE][MAXSIZE];
int size, blocksize;

/*
 * GetGrid reads the sudoku grid values from the file specified by its parameter
 * Empty cells are zeroes in the file, givens are 1-9 in 9x9 grids, a-p in 16x16,
 * and a-y in 25x25.  Entries can be separated by blanks.
 */

void GetGrid( char * filename ) {

     ifstream inFile( filename );
     if ( !inFile ) {
	  cerr << "Cannot open " << filename << "\n";
	  exit( 1 );
     }
     inFile >> size;
     blocksize = round( sqrt( size ) );
     cout << "size = " << size << '\n';
     int lowchar = '1';
     if (size > 9) { lowchar = 'a'; }
     char c = ' ';
     while (c != '\n') { c = inFile.get(); }
     for (int i=0; i<size; i++) {
	  for (int j=0; j<size; j++) {
	       c = inFile.get();
	       if (c == ' ') { c = inFile.get(); }
	       if (c == '0') { grid[i][j] = 0; }
	       else {	
		    if ((c < lowchar) || (c > (lowchar+size-1))) {
			 cerr << c << " :Invalid char in file\n";
			 exit( 1 );
		    }
		    grid[i][j] = 1 + c - lowchar;
	       }
	  }
	  while (c != '\n') { c = inFile.get(); }
     }
}

/*
 * PrintGrid prints the sudoku grid.  Empty cells print as dots.
 */
void PrintGrid( void ) {
     int lowchar = '1';
     if (size > 9) { lowchar = 'a'; }
     for (int i=0; i<size; i++) {
	  for (int j=0; j<size; j++) {
	       if (grid[i][j] == 0) { cout << ". "; }
	       else {
		    char c = lowchar + grid[i][j] - 1;
		    cout << c << " ";
	       }
	  }
	  cout << "\n";
     }
}

/*
 * RemoveLegal sets entries in "legal" to 0 (false) for
 * values that occur in the same row, column, or sub-block
 */

void RemoveLegal(int r, int c ) {
     int v = grid[r][c] - 1;
     for (int i=0; i<size; i++) {
	  legal[r][i][v] = 0;
	  legal[i][c][v] = 0;
     }
     r = blocksize * (r/blocksize);
     c = blocksize * (c/blocksize);
     for (int i=r; i<(r+blocksize); i++) {
	  for (int j=c; j<(c+blocksize); j++) {
	       legal[i][j][v] = 0;
	  }
     }
     legal[r][c][grid[r][c] - 1] = 1;
}

int domainSize(int r, int c) {
     int domain_size = 0;
     for (int i = 0; i < size; i++)
	  domain_size += legal[r][c][i];
     return domain_size;
}

/*
 * InitLegal initializes "legal" using RemoveLegal
 */

void InitLegal( void ) {
     for (int i=0; i<size; i++) {
	  for (int j=0; j<size; j++) {
	       for (int k=0; k<size; k++) {
		    legal[i][j][k] = 1;
	       }
	  }
     }
     for (int i=0; i<size; i++) {
	  for (int j=0; j<size; j++) {
	       if (grid[i][j] > 0) {
		    RemoveLegal( i, j );
	       }
	  }
     }
}

/*
 * CopyState saves or restores a copy of the grid when backtracking
 */

void CopyState( int g[MAXSIZE][MAXSIZE], int s[MAXSIZE][MAXSIZE] ) {

     for (int i=0; i<size; i++) {
	  for (int j=0; j<size; j++) {
	       s[i][j] = g[i][j];
	  }
     }
}

/*
 * CopyLegal saves or restores a copy of the grid when backtracking
 * if v is the sentinel -1, it copies all values, otherwise it only copies
 * legal values for index v
 */
void CopyLegal( int g[MAXSIZE][MAXSIZE][MAXSIZE], int s[MAXSIZE][MAXSIZE][MAXSIZE], int v) {
     if (v == -1) {
	  for (int i=0; i<size; i++) {
	       for (int j=0; j<size; j++) {
		    for (int k = 0; k < size; k++) {
			 s[i][j][k] = g[i][j][k];
		    }
	       }
	  }
     }
     else {
	  for (int i=0; i<size; i++) {
	       for (int j=0; j<size; j++) {
		    s[i][j][v] = g[i][j][v];
	       }
	  }
     }
}

/* naiveNextPos picks first unassigned cell, no heuristic is used */
void naiveNextPos(int *pos) {
     for (int i = 0; i < size; i++) {
     	  for (int j = 0; j < size; j++) {
	       if (!grid[i][j]) {
		    pos[0] = i;
		    pos[1] = j;
		    return;
	       }
	  }
     }
}

/* naiveNextVal picks first val from list, no heuristic is used */
int naiveNextVal(int* vals, int, int) {
     int returnVal = vals[0];
     for (int i = 0; i < size - 1 && vals[i]; i++)
	  vals[i] = vals[i+1];
     return returnVal;
}

/* minimumRemainingValues assigns a position to given pos arr, according to the
 * MRV heuristic: the position with the fewest legal values remaining is picked.
 * If a position with 0 legal vals is found, it quits searching and returns */
void minimumRemainingValues(int *pos) {
     int min_domain_size = size;
     int domain_size = 0;
     for (int i = 0; i < size; i++) {
	  for (int j = 0; j < size; j++) {
	       if (!grid[i][j]) {
		    domain_size = domainSize(i, j);
		    if (domain_size < min_domain_size) {
			 pos[0] = i;
			 pos[1] = j;
			 min_domain_size = domain_size;
		    }
		    if (domain_size == 0) {
			 return;
		    }
	       }
	  }
     }
}

/* maximumRemainingValues assigns a position to given pos arr, according to the
 * FRV heuristic: the position with the maximum legal values remaining is picked.
 * If a position with size legal vals is found, it quits searching and returns */
void maximumRemainingValues(int *pos) {
     int max_domain_size = 0;
     int domain_size = 0;
     for (int i = 0; i < size; i++) {
	  for (int j = 0; j < size; j++) {
	       if (!grid[i][j]) {
		    domain_size = domainSize(i, j);
		    if (domain_size >= max_domain_size) {
			 pos[0] = i;
			 pos[1] = j;
			 max_domain_size = domain_size;
		    }
		    if (domain_size == size) {
			 return;
		    }
	       }
	  }
     }
}

/* mostConstrainingValue picks a value from a list of legal values for the given
 * position, according to the MCV heuristic: the value which constrains the most
 * other values is chosen. */
int mostConstrainingValue (int *vals, int r, int c) {
     if (vals[0] == 0)
	  return 0; // No values to pick from
     int sum_domains = 0;
     int v = vals[0] - 1;
     int max_domain_sum = 0;
     int val = v;
     
     // Sum domains for each position affected by given position
     // pick value which maximizes this (constrains most other vals)
     for (int i = 0; i < size; i++) {
	  for (int j = 0; j < size; j++) {
	       sum_domains += legal[j][c][vals[i]];
	       sum_domains += legal[r][j][vals[i]];
	  }

	  if (sum_domains >= max_domain_sum) {
	       max_domain_sum = sum_domains;
	       val = vals[i];
	  }

	  // End if no more vals to pick from
	  if (i == size-1 || vals[i+1] == 0)
	       break;

	  sum_domains = 0;
     }

     // Remove value from list, shift over values by one.
     for (int i = 0; i < MAXSIZE; i++) {
	  if (vals[i] == val) {
	       for (int j = i; j < MAXSIZE; j++) {
		    vals[j] = vals[j+1];
		    // Finished copying over, return val
		    if (vals[j] == 0) {
			 return val;
		    }
	       }
	  }
     }
     // Note: this function will (should) always return before this.
     return val;
}


/* mostConstrainingValue picks a value from a list of legal values for the given
 * position, according to the MCV heuristic: the value which constrains the most
 * other values is chosen. */
int leastConstrainingValue(int *vals, int r, int c) {
     if (vals[0] == 0)
	  return 0; // No vals to choose from
     int sum_domains = 0;
     int v = vals[0] - 1;
     int min_domain_sum = size * 2 + 3;
     int val = v;

     // Sum domains to find how much each val affects other positions
     // Choose the val which minimizes this number
     for (int i = 0; i < size; i++) {
	  for (int j = 0; j < size; j++) {
	       sum_domains += legal[j][c][vals[i]];
	       sum_domains += legal[r][j][vals[i]];
	  }
	  if (sum_domains < min_domain_sum) {
	       min_domain_sum = sum_domains;
	       val = vals[i];
	  }
	  // No more vals to choose from
	  if (i == size - 1 || vals[i+1] == 0)
	       break;

	  sum_domains = 0;
     }

     // Copy back values and return val
     for (int i = 0; i < MAXSIZE; i++) {
	  if (vals[i] == val) {
	       for (int j = i; j < MAXSIZE; j++) {
		    vals[j] = vals[j+1];
		    if (vals[j] == 0) {
			 return val;
		    }
	       }
	  }
     }
     return val;
}


// Determines arc consistent of current grid.
int arc3Check(int r, int c) {
     if (domainSize(r, c) > 1)
	  return 1;

     for (int i = 0; i < size; i++) {
	  for (int k = 0; k < size; k++) {
	       // Break and check next arc if there exists a legal val
	       if (legal[i][c][k]) {
		    break;
	       } else if (i != r && k == size - 1 && !grid[i][c]) {
		    return 0; // No legal values for this arc
	       }
	  }
     }

     // Repeat previous process for cols
     for (int j = 0; j < size; j++) {
	  for (int k = 0; k < size; k++) {
	       if (legal[r][j][k]) {
		    break;
	       } else if (j != c && k == size - 1 && !grid[r][j]) {
		    return 0;
	       }
	  }
     }

     // Success if all arcs consistent
     return 1;
}

/*
 * Outline for the backtracking solver
 * A return value of 0 means a successful solution, 1 means
 * backtracking is necessary
 */

// Counters for program analysis
int how_deep = 0;
int max_deepness = 0;
int total = 0;

int Backtrack( void ) {
     total++;
     how_deep++;
     int savelegal[MAXSIZE][MAXSIZE][MAXSIZE];
     int vals[MAXSIZE];  // legal values for the current cell

     // Grab next positions from nextPos according to heuristic
     int pos[] = {-1, -1}; // Sentinel values
     
     // Pick next pos according to assigned heuristic
     nextPos(pos);
     int i = pos[0];
     int j = pos[1];


     if (pos[0] == -1 || pos[1] == -1) {
	  return 0; // No empty cell was found; puzzle is solved
     }
     for (int k=0; k<size; k++) {
	  vals[k] = 0; // Initialize legal vals to 0
     }

     // Create list of legal vals according to legal matrix
     int v = 0;
     for (int k=0; k<size; k++) {
	  if (legal[i][j][k]) {
	       vals[v] = k+1;
	       v++;
	  }
     }

     if (v == 0) {
	  if (how_deep >= max_deepness) {
	       max_deepness = how_deep;
	  }
	  how_deep--;
	  return 1; // no legal values
     }


     // Backtracking below this
     // Alternative to copystate used
     
     // Copy legal vals to preserve state
     CopyLegal( legal, savelegal, -1);
     for (int k=0; k<v; k++) {
	  // Pick next val according to assigned heuristic
	  int val = nextVal(vals, i, j);
	  if (val == 0) {
	       if (how_deep >= max_deepness) {
		    max_deepness = how_deep;
	       }
	       how_deep--;
	       return 1;  // ran out of values to try - need to backtrack
	  } else {  // Need to do inference and recursion here
	       grid[i][j] = val; // assign val to grid
	       RemoveLegal(i, j); // adjust legal values given new assignment
	       if (!arcConsistent(i, j) || Backtrack()) { // failed
		    CopyLegal(savelegal, legal, val - 1); // reset legals
		    grid[i][j] = 0; // Reset position
	       } else {
		    return 0; // success!
	       }
	  }
     }
     if (how_deep >= max_deepness) {
	  max_deepness = how_deep;
     }
     how_deep--;
     return 1; // all values tried and failed - no soln - fail
}

int main (int argc, char * const argv[]) {
        (void) argc;
        GetGrid( argv[1] );
        cout << "Sudoku grid\n";
        PrintGrid();
        InitLegal();
// You may want to call a function here to make easy inferences
// In some cases this may solve it without backtracking
        Backtrack();
        PrintGrid();
}
