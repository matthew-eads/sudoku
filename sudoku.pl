#!/usr/bin/perl -w

# Try to solve a sudoku using local search 
# Author:  Anselm Blumer, Tufts University
# Date: 21 February, 2011

# Global variables
# grid: 9 x 9 array giving the current state of the sudoku
#   positive values are numbers that have been filled in
#   zeroes are blank squares
#   negative values are numbers that can be changed
# legal: 9 x 9 x 9 array giving remaining legal values
#   legal[i][j] is an array of Booleans

our @grid;
our @legal;

# ReadFile reads a sudoku grid from a file named "sudokufile"

sub ReadFile {
   our @grid;

open STATEFILE, "<sudokufile";
for my $i (0..8) {
   my $line = <STATEFILE>;
   chomp($line);
   my @tmp = split / /, $line;
   for my $j (0..8) {
      $grid[$i][$j] = $tmp[$j];
   }
}
}

# PrintGrid prints the current grid

sub PrintGrid {
   our @grid;

for my $i (0..8) {
   for my $j (0..8) {
       print abs $grid[$i][$j];
       print " ";
   }
   print "\n";
}
print "-----\n";
}

# RemoveLegal is given a cell location and sets the Booleans
# in the "legal" array in the same row, column, and 3 x 3
# box corresponding to that value to 0

sub RemoveLegal {
   our @grid;
   our @legal;

my $r = $_[0];
my $c = $_[1];
my $v = $grid[$r][$c] - 1;
for my $i (0..8) {
   $legal[$r][$i][$v] = 0;
   $legal[$i][$c][$v] = 0;
}
$r = 3 * int( $r/3 );
$c = 3 * int( $c/3 );
for my $i ($r..($r+2)) {
   for my $j ($c..($c+2)) {
      $legal[$i][$j][$v] = 0;
   }
}
}

# Init initializes the "legal" array

sub Init {
   our @grid;
   our @legal;

for my $i (0..8) {
   for my $j (0..8) {
      for my $k (0..8) {
         $legal[$i][$j][$k] = 1;
      }
   }
}
for my $i (0..8) {
   for my $j (0..8) {
      if ($grid[$i][$j] > 0) {
         RemoveLegal( $i, $j );
      }
   }
}
}

# SolveEasy fills in cells that have only one legal
# value, updating the "legal" array, continuing until
# all cells are filled in or all remaining cells have
# more than one legal value.

sub SolveEasy {
   our @grid;
   our @legal;

my $update = 1;
while ($update) {
   $update = 0;
   for my $i (0..8) {
      for my $j (0..8) {
         if ($grid[$i][$j] == 0) {
            my $val = 0;
            my $count = 0;
            for my $k (0..8) {
               if ($legal[$i][$j][$k]) {
                  $count++;
                  $val = $k + 1;
                }
            }
            if ($count == 1) {
               $grid[$i][$j] = $val;
               RemoveLegal( $i, $j );
               $update = 1;
            }
         }
      }
   }
}
}

# FillRandRow fills the empty spaces in the specified
# row with random values so that the row contains 1-9

sub FillRandRow {
   our @grid;

my $r = $_[0];
my %rowvals = ();
for my $j (0..8) {
   if ($grid[$r][$j] != 0) {
      $rowvals{ $grid[$r][$j] } = 1;
   }
}
my %newvals = ();
for my $v (1..9) {
   if (!exists $rowvals{ $v }) {
      $newvals{ $v } = 1;
   }
}
my @tmp = ();
for my $j (0..8) {
   if ($grid[$r][$j] == 0) {
      my @tmp = keys %newvals;
      if (@tmp == 1) { $grid[$r][$j] = 0 - $tmp[0]; }
      else {
         $randindex = int rand @tmp;
         $grid[$r][$j] = 0 - $tmp[ $randindex ];
         delete $newvals{ $tmp[ $randindex ] };
      }
   }
}
}

# FillRand fills each row with random consistent values
# ignoring column and block conflicts

sub FillRand {
for my $i (0..8) {
   FillRandRow( $i );
}
}

# ColConflicts counts the total number of vertical conflicts
# in a specified column

sub ColConflicts {
   our @grid;

my $c = $_[0];
my $count = 0;
for my $i (0..7) {
   for my $j (($i+1)..8) {
      if (abs($grid[$i][$c]) == abs($grid[$j][$c])) { $count++; }
   }
}
return $count;
}

# BlockConflicts counts the total number of conflicts
# in a specified 3 x 3 block

sub BlockConflicts {
   our @grid;

my $r = $_[0];
my $c = $_[1];
my @vals = ();
for my $i ($r..($r+2)) {
   for my $j ($c..($c+2)) {
      push @vals, abs($grid[$i][$j]);
   }
}
my $count = 0;
for my $i (0..7) {
   for my $j (($i+1)..8) {
      if ($vals[$i] == $vals[$j]) { $count++; }
   }
}
return $count;
}

# TotalConflicts counts the total number of conflicts

sub TotalConflicts {
my $conflicts = 0;
for my $i (0..8) { $conflicts += ColConflicts( $i ); }
for my $i (0..2) {
   for my $j (0..2) {
      $conflicts += BlockConflicts( 3*$i, 3*$j );
   }
}
return $conflicts;
}

# BestSwap tries out every possible swap of pairs
# of changeable values in the same row, picking the
# one that results in the fewest conflicts

sub BestSwap {
   our @grid;

my $bestv = 1000;
my $bestr = 0;
my $besti = 0;
my $bestj = 0;
for my $r (0..8) {
   for my $i (0..7) {
      if ($grid[$r][$i] < 0) {
         for my $j (($i+1)..8) {
            if ($grid[$r][$j] < 0) {
               my $tmp = $grid[$r][$i];
               $grid[$r][$i] = $grid[$r][$j];
               $grid[$r][$j] = $tmp;
               $tmp = TotalConflicts();
               if ($tmp < $bestv) {
                  $bestv = $tmp;
                  $bestr = $r;
                  $besti = $i;
                  $bestj = $j;
               }
               $tmp = $grid[$r][$i];
               $grid[$r][$i] = $grid[$r][$j];
               $grid[$r][$j] = $tmp;
            }
         }
      }
   }
}
return ($bestv, $bestr, $besti, $bestj);
}

# main program

my ($conflicts, $newconflicts, $r, $i, $j);
ReadFile();
PrintGrid();
Init();
SolveEasy();
$conflicts = TotalConflicts();
if ($conflicts == 0) {
   PrintGrid();
   exit 0;
}
FillRand();
PrintGrid();
($newconflicts, $r, $i, $j) = BestSwap();
while ($newconflicts < $conflicts) {
   if ($newconflicts == 0) {
       PrintGrid();
       exit 0;
   }
   $conflicts = $newconflicts;
   my $tmp = $grid[$r][$i];
   $grid[$r][$i] = $grid[$r][$j];
   $grid[$r][$j] = $tmp;
   print $newconflicts, " conflicts\n";
   ($newconflicts, $r, $i, $j) = BestSwap();
}
PrintGrid();
