#include <stdlib.h>
#include <string.h>
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
