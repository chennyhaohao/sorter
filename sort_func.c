#include <stdlib.h>
#include "./record.c"
#include "./compare.h"


int quicksort(tax_rec* arr, int nitem, int sort_by) {
	switch(sort_by) {
		case 0:
			qsort(arr, nitem, sizeof(tax_rec), cmp_by_id);
			break;

		case 1:
			qsort(arr, nitem, sizeof(tax_rec), cmp_by_fname);
			break;

		case 2:
			qsort(arr, nitem, sizeof(tax_rec), cmp_by_lname);
			break;

		case 3:
			qsort(arr, nitem, sizeof(tax_rec), cmp_by_income);
			break;

		default:
			return -1;
		return 0;
	}
}

int cmp_rec(tax_rec* rec1, tax_rec* rec2, int sort_by) {
	switch(sort_by) {
		case 0:
			return cmp_by_id(rec1, rec2);

		case 1:
			return cmp_by_fname(rec1, rec2);

		case 2:
			return cmp_by_lname(rec1, rec2);

		case 3:
			return cmp_by_income(rec1, rec2);

		default:
			return -1;
	}
}

int shellsort(tax_rec* arr, int nitems, int sort_by) {
	if (sort_by < 0 || sort_by > 3) return -1;

	for (int gap = nitems/2; gap > 0; gap /= 2)
    {
        // Do a gapped insertion sort for this gap size.
        // The first gap elements a[0..gap-1] are already in gapped order
        // keep adding one more element until the entire array is
        // gap sorted 
        for (int i = gap; i < nitems; i += 1)
        {
            // add a[i] to the elements that have been gap sorted
            // save a[i] in temp and make a hole at position i
            tax_rec temp = arr[i];
 
            // shift earlier gap-sorted elements up until the correct 
            // location for a[i] is found
            int j;            
            for (j = i; j >= gap && cmp_rec(&arr[j-gap], &temp, sort_by) > 0; j -= gap) {
                arr[j] = arr[j - gap];
            }
             
            //  put temp (the original a[i]) in its correct location
            arr[j] = temp;
        }
    }
    return 0;
}

void swap(tax_rec *xp, tax_rec *yp)
{
    tax_rec temp = *xp;
    *xp = *yp;
    *yp = temp;
}

// A function to implement bubble sort
void bubblesort(tax_rec * arr, int nitems, int sort_by)
{
   int i, j;
   for (i = 0; i < nitems-1; i++)      

       // Last i elements are already in place   
       for (j = 0; j < nitems-i-1; j++) 
           //if (arr[j] > arr[j+1])
       		if (cmp_rec(&arr[j], &arr[j+1], sort_by) > 0)
              swap(&arr[j], &arr[j+1]);
}

